#include "render/renderer.h"
#include "core/stats.h"
#include "material/material.h"
#include "core/random.h"
#include "light/light.h"
#include "material/bsdf_sample.h"
#include "render/guiding_debug.h"
#include "guiding/guiding_trainer.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    double powerHeuristic(double pdfA, double pdfB) {
        double a = pdfA * pdfA;
        double b = pdfB * pdfB;

        if (a + b <= 0.0) {
            return 0.0;
        }

        return a / (a + b);
    }

    bool isBlack(const Color& c) {
        return maxComponent(c) <= 1e-12;
    }

    Color estimateDirectLightMIS(const HitRecord& rec,const Vec3& wo,const Scene& scene) {
        if (!rec.material) {
            return Color(0.0, 0.0, 0.0);
        }

        if (scene.lights.empty()) {
            return Color(0.0, 0.0, 0.0);
        }

        LightSelectionSample lightSelection =scene.sampleLight();

        if (!lightSelection.valid || !lightSelection.light) {
            return Color(0.0, 0.0, 0.0);
        }

        const auto& light = lightSelection.light;
        double lightSelectionPdf =lightSelection.selectionPdf;

        LightSample lightSample;

        if (!light->sample(rec.p, lightSample)) {
            return Color(0.0, 0.0, 0.0);
        }

        Vec3 wi = lightSample.wi.normalized();

        double cosSurface = std::max(0.0,dot(rec.shadingNormal.normalized(), wi));

        if (cosSurface <= 0.0) {
            return Color(0.0, 0.0, 0.0);
        }

        double pdfLight =lightSelectionPdf * lightSample.pdf;

        if (pdfLight <= 1e-12) {
            return Color(0.0, 0.0, 0.0);
        }

        Ray shadowRay(rec.p + rec.geometricNormal * 1e-4, wi);

        double shadowTMax =lightSample.isInfinite ? 1e30 : lightSample.distance - 1e-4;

        HitRecord shadowRec;
        if (scene.intersect(shadowRay, 1e-4, shadowTMax, shadowRec)) {
            return Color(0.0, 0.0, 0.0);
        }

        Color f = rec.material->eval(wo, rec.shadingNormal, wi);

        if (isBlack(f)) {
            return Color(0.0, 0.0, 0.0);
        }

        double pdfBsdf =rec.material->pdfValue(wo, rec.shadingNormal, wi);

        double misWeight =powerHeuristic(pdfLight, pdfBsdf);

        //最终颜色 = 材质反射率 × 光颜色 × 余弦衰减 / 采样概率 × MIS 权重
        return f * lightSample.emission * (cosSurface / pdfLight) * misWeight;
    }

    double misWeightForBsdfHitLight(const Scene& scene,const Point3& previousPoint,const Vec3& previousWi,double previousBsdfPdf,bool previousWasDelta) {
        if (previousWasDelta) {
            return 1.0;
        }

        if (previousBsdfPdf <= 1e-12) {
            return 0.0;
        }

        double pdfLight = scene.lightPdfSum(
            previousPoint,
            previousWi
        );

        return powerHeuristic(
            previousBsdfPdf,
            pdfLight
        );
    }

    GuidingDebugCollector gGuidingDebugCollector;
    GuidingTrainer gGuidingTrainer;
}

Renderer::Renderer(int spp, int depth)
    : samplesPerPixel(spp), maxDepth(depth) {
}

void Renderer::setEnableGuidingRecord(bool enabled) {
    enableGuidingRecord = enabled;
}

/*
trace() 执行逻辑：

1. 初始化 L、beta、ray 和 previous* 状态。
2. 每个 bounce 先用 ray 和场景求交。
3. 如果没命中，说明看到了 environment：
   - 主射线直接看到 environment：直接加到 L。
   - BSDF sample 之后看到 environment：用 MIS 加到 L。
4. 如果命中 emitter：
   - 主射线直接命中 emitter：直接加到 L。
   - BSDF sample 之后命中 emitter：用 MIS 加到 L。
5. 如果命中普通表面：
   - 先做 NEE，主动采样光源，得到直接光。
   - 再做 BSDF sample，得到下一跳方向 wi。
   - 根据 delta / non-delta 更新 beta。
6. 深度足够后执行俄罗斯轮盘。
7. 保存 previous*，用于下一跳如果打到 light / environment 时计算 MIS。
8. 发射下一条 ray，继续 bounce。

核心检查点：
- L += ... 表示真正累计光。
- beta *= ... 表示更新路径权重。
- previous* 表示为下一跳的 BSDF-hit-light MIS 保存状态。
*/
Color Renderer::trace(const Ray& rayIn, const Scene& scene, int depth) const {
    GuidingRecord guidingRecord;

    if (enableGuidingRecord) {
        guidingRecord.clear();
    }

    Color L(0.0, 0.0, 0.0);
    Color beta(1.0, 1.0, 1.0);

    Ray ray = rayIn;

    bool previousWasBsdfSample = false;
    double previousBsdfPdf = 0.0;
    Point3 previousPoint;
    Vec3 previousWi;
    bool previousWasDelta = false;

    for (int bounce = 0; bounce < depth; ++bounce) {
        HitRecord rec;

        if (!scene.intersect(ray, 1e-4, 1e30, rec)) {
            Vec3 unitDir = ray.direction.normalized();

            Color environmentRadiance(0.0, 0.0, 0.0);

            if (scene.environment) {
                environmentRadiance =
                    scene.environment->eval(unitDir);
            }
            else {
                double t = 0.5 * (unitDir.y + 1.0);

                environmentRadiance =
                    (1.0 - t) * Color(1.0, 1.0, 1.0) +
                    t * Color(0.5, 0.7, 1.0);
            }

            if (bounce == 0) {
                L += beta * environmentRadiance;
            }
            else if (previousWasBsdfSample) {
                double misWeight = misWeightForBsdfHitLight(
                    scene,
                    previousPoint,
                    previousWi,
                    previousBsdfPdf,
                    previousWasDelta
                );

                L += beta * environmentRadiance * misWeight;
            }
            else {
                L += beta * environmentRadiance;
            }

            break;
        }

        if (!rec.material) {
            break;
        }

        Color emitted = rec.material->emitted();

        if (!isBlack(emitted)) {
            if (bounce == 0) {
                L += beta * emitted;
            }
            else if (previousWasBsdfSample) {
                double misWeight = misWeightForBsdfHitLight(scene, previousPoint, previousWi, previousBsdfPdf, previousWasDelta);

                L += beta * emitted * misWeight;
            }

            break;
        }

        Vec3 wo = (-ray.direction).normalized();

        Color directLight = estimateDirectLightMIS(rec, wo, scene);

        L += beta * directLight;

        BSDFSample bsdfSample =rec.material->sample(wo, rec.shadingNormal);

        if (!bsdfSample.valid || bsdfSample.pdf <= 1e-12) {
            break;
        }

        Vec3 wi = bsdfSample.wi.normalized();
        Color f = bsdfSample.f;
        double pdfBsdf = bsdfSample.pdf;
        double cosTheta = 1.0;

        if (!bsdfSample.isDelta) {
            cosTheta = std::max(0.0,dot(rec.shadingNormal.normalized(), wi));

            if (cosTheta <= 0.0) {
                break;
            }
        }

        if (enableGuidingRecord) {
            PathVertex vertex;
            vertex.position = rec.p;
            vertex.geometricNormal = rec.geometricNormal;
            vertex.shadingNormal = rec.shadingNormal;
            vertex.wo = wo;
            vertex.wi = wi;
            vertex.throughput = beta;
            vertex.bsdfValue = f;
            vertex.bsdfPdf = bsdfSample.isDelta ? 0.0 : pdfBsdf;
            vertex.lightPdf = bsdfSample.isDelta ? 0.0 : scene.lightPdfSum(rec.p, wi);
            vertex.cosTheta = bsdfSample.isDelta ? 1.0 : cosTheta;
            vertex.depth = bounce;
            vertex.eventType = bsdfSample.type;
            vertex.isDelta = bsdfSample.isDelta;
            vertex.valid = true;

            guidingRecord.addVertex(vertex);
            gGuidingDebugCollector.recordVertex(vertex);
            gGuidingTrainer.recordVertex(vertex);

            ++gStats.guidingVertices;
        }

        if (bsdfSample.isDelta) {
            beta = beta * f;
        }
        else {
            beta = beta * f * (cosTheta / pdfBsdf);
        }

        if (bounce >= 3) {
            double p = std::min(maxComponent(beta), 0.95);

            if (randomDouble() > p) {
                break;
            }

            beta = beta / p;
        }

        previousWasBsdfSample = true;
        previousWasDelta = bsdfSample.isDelta;
        previousBsdfPdf = bsdfSample.isDelta ? 0.0 : pdfBsdf;
        previousPoint = rec.p;
        previousWi = wi;

        ray = Ray(rec.p + rec.geometricNormal * 1e-4, wi);
    }

    if (enableGuidingRecord) {
        guidingRecord.finalRadiance = L;
    }

    return L;
}

void Renderer::render(const Scene& scene, const Camera& camera, Film& film) const {
    if (enableGuidingRecord) {
        gGuidingDebugCollector.reset();
        gGuidingTrainer.reset();
    }

    for (int j = 0; j < film.height; ++j) {
        std::cout << "\rRendering line " << (j + 1) << " / " << film.height << std::flush;

        for (int i = 0; i < film.width; ++i) {
            Color pixelColor(0.0, 0.0, 0.0);

            for (int s = 0; s < samplesPerPixel; ++s) {
                double u = (i + randomDouble()) / (film.width - 1);
                double v = (j + randomDouble()) / (film.height - 1);

                Ray ray = camera.generateRay(u, v);
                pixelColor += trace(ray, scene, maxDepth);
            }

            film.setPixel(i, j, pixelColor);
        }
    }

    std::cout << std::endl;

    if (enableGuidingRecord) {
        gGuidingTrainer.build();

        gGuidingDebugCollector.print();
        gGuidingTrainer.printStats();
    }
}
