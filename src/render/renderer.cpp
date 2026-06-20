#include "render/renderer.h"
#include "core/frame.h"
#include "core/stats.h"
#include "material/material.h"
#include "core/random.h"
#include "light/light.h"
#include "material/bsdf_sample.h"
#include "render/guiding_debug.h"
#include "guiding/guiding_trainer.h"
#include "guiding/local_hemisphere_histogram.h"
#include "guiding/openpgl_guiding.h"


#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    constexpr double PI = 3.14159265358979323846;

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

    bool isBadNumber(double x) {
        return std::isnan(x) || std::isinf(x);
    }

    Color applyHitBaseColor(const HitRecord& rec, const Color& f) {
        if (!rec.hasBaseColor || !rec.material) {
            return f;
        }

        if (!dynamic_cast<const Lambertian*>(rec.material.get())) {
            return f;
        }

        return rec.baseColor / PI;
    }

    void recordPdfStats(double pdfBsdf,double pdfGuided,double pdfFinal) {
        if (pdfBsdf > 0.0 && !isBadNumber(pdfBsdf)) {
            gStats.minBsdfPdf = std::min(gStats.minBsdfPdf, pdfBsdf);
            gStats.maxBsdfPdf = std::max(gStats.maxBsdfPdf, pdfBsdf);
            gStats.sumBsdfPdf += pdfBsdf;
        }

        if (pdfGuided > 0.0 && !isBadNumber(pdfGuided)) {
            gStats.minGuidedPdf = std::min(gStats.minGuidedPdf, pdfGuided);
            gStats.maxGuidedPdf = std::max(gStats.maxGuidedPdf, pdfGuided);
            gStats.sumGuidedPdf += pdfGuided;
        }

        if (pdfFinal > 0.0 && !isBadNumber(pdfFinal)) {
            gStats.minFinalPdf = std::min(gStats.minFinalPdf, pdfFinal);
            gStats.maxFinalPdf = std::max(gStats.maxFinalPdf, pdfFinal);
            gStats.sumFinalPdf += pdfFinal;
        }

        if (pdfGuided <= 1e-12) {
            ++gStats.guidedPdfZeroSamples;
        }

        if (isBadNumber(pdfGuided)) {
            ++gStats.guidedPdfBadSamples;
        }

        if (pdfFinal <= 1e-12) {
            ++gStats.finalPdfZeroSamples;
        }

        if (isBadNumber(pdfFinal)) {
            ++gStats.finalPdfBadSamples;
        }
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

        Color f = applyHitBaseColor(
            rec,
            rec.material->eval(wo, rec.shadingNormal, wi)
        );

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
    OpenPGLGuiding gOpenPGLGuiding;

    constexpr int kMinSpatialGuidingSamplesPerCell = 16;

    bool useOpenPGLGuiding(GuidingMode mode) {
        if (mode != GuidingMode::OpenPGL) {
            return false;
        }

        if (!gOpenPGLGuiding.enabled()) {
            return false;
        }

        return gOpenPGLGuiding.stats().openPGLUpdateSucceeded;
    }

    bool useSpatialGuidingAt(const Point3& p, GuidingMode mode) {
        if (mode != GuidingMode::Spatial && mode != GuidingMode::OpenPGL) {
            return false;
        }

        const SpatialGuidingCell* cell = gGuidingTrainer.spatialDistribution().cellAt(p);

        if (!cell) {
            return false;
        }

        return cell->isReady(kMinSpatialGuidingSamplesPerCell);
    }

    bool canUseGuidingAt(const Point3& p, GuidingMode mode) {
        if (useOpenPGLGuiding(mode)) {
            return true;
        }

        if (useSpatialGuidingAt(p, mode)) {
            return true;
        }

        return gGuidingTrainer.distribution().getTotalWeight() > 0.0;
    }

    LocalGuidedSample sampleGuidedLocalAt(const Point3& p, const Vec3& normal, const Vec3& wo, GuidingMode mode) {
        if (useOpenPGLGuiding(mode)) {
            OpenPGLGuidedSample openPGLSample = gOpenPGLGuiding.sample(p, normal, wo);

            if (openPGLSample.valid && openPGLSample.pdf > 1e-12 && !isBadNumber(openPGLSample.pdf)) {
                Vec3 worldWi = openPGLSample.wi.normalized();
                double cosCheck = std::max(0.0, dot(normal.normalized(), worldWi));

                if (cosCheck > 0.0) {
                    Frame frame(normal);
                    Vec3 localWi = frame.toLocal(worldWi).normalized();

                    if (localWi.z > 0.0) {
                        LocalGuidedSample result;
                        result.localWi = localWi;
                        result.pdf = openPGLSample.pdf;
                        result.valid = true;
                        return result;
                    }
                }
            }
        }

        if (useSpatialGuidingAt(p, mode)) {
            return gGuidingTrainer.spatialDistribution().sampleLocal(p, kMinSpatialGuidingSamplesPerCell);
        }

        return gGuidingTrainer.distribution().sample();
    }

    double guidedPdfLocalAt(const Point3& p, const Vec3& normal, const Vec3& localWi, GuidingMode mode) {
        if (useOpenPGLGuiding(mode)) {
            Frame frame(normal);
            Vec3 worldWi = frame.toWorld(localWi.normalized()).normalized();
            double openPGLPdf = gOpenPGLGuiding.pdf(p, normal, worldWi);

            if (openPGLPdf > 1e-12 && !isBadNumber(openPGLPdf)) {
                return openPGLPdf;
            }
        }

        if (useSpatialGuidingAt(p, mode)) {
            return gGuidingTrainer.spatialDistribution().pdfLocal(p, localWi, kMinSpatialGuidingSamplesPerCell);
        }

        return gGuidingTrainer.distribution().pdf(localWi);
    }
}

Renderer::Renderer(int spp, int depth)
    : samplesPerPixel(spp), maxDepth(depth) {
}

void printGuidingDistributionSummary() {
    std::cout << "Global histogram total weight: " << gGuidingTrainer.distribution().getTotalWeight() << "\n";
    std::cout << "Spatial grid total samples: " << gGuidingTrainer.spatialDistribution().totalSamples() << "\n";
    std::cout << "Spatial grid active cells: " << gGuidingTrainer.spatialDistribution().activeCells() << "\n";
}

void Renderer::setEnableGuidingRecord(bool enabled) {
    enableGuidingRecord = enabled;
}

void Renderer::setEnableGuidedSampling(bool enabled) {
    enableGuidedSampling = enabled;
}

void Renderer::setGuidingProbability(double probability) {
    guidingProbability = std::max(
        0.0,
        std::min(1.0, probability)
    );
}

void Renderer::setGuidingMode(GuidingMode mode) {
    guidingMode = mode;
}

/*
trace() 执行逻辑：
1. 初始化 L、beta、ray 和 previous* 状态。
2. 每个 bounce 先用 ray 和场景求交。
3. 如果没命中，说明看到 environment：
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

        /*BSDFSample bsdfSample = rec.material->sample(wo, rec.shadingNormal);

        if (!bsdfSample.valid || bsdfSample.pdf <= 1e-12) {
            break;
        }

        Vec3 wi = bsdfSample.wi.normalized();
        Color f = bsdfSample.f;
        double pdfBsdf = bsdfSample.pdf;
        */
        BSDFSample bsdfSample;

        bool usedGuidedSampling = false;

        Vec3 wi;
        Color f;
        double pdfBsdf = 0.0;
        double pdfGuided = 0.0;
        double pdfFinal = 0.0;

        bool canUseGuiding = enableGuidedSampling && canUseGuidingAt(rec.p, guidingMode);

        double pGuiding = canUseGuiding ? guidingProbability : 0.0;

        double pBsdf = 1.0 - pGuiding;

        // -----------------------------------------------------
        // 先判断材质是否可能是 delta
        // 第一版做法：先让材质 sample 一次。
        // 如果 sample 结果是 delta，就不使用 guiding。
        // -----------------------------------------------------

        BSDFSample initialSample =
            rec.material->sample( wo, rec.shadingNormal);

        if (!initialSample.valid || initialSample.pdf <= 1e-12) {
            break;
        }

        if (initialSample.isDelta) {
            bsdfSample = initialSample;

            wi = bsdfSample.wi.normalized();
            f = applyHitBaseColor(rec, bsdfSample.f);
            pdfBsdf = bsdfSample.pdf;
            pdfGuided = 0.0;
            pdfFinal = pdfBsdf;

            usedGuidedSampling = false;
        }
        else {
            // -------------------------------------------------
            // non-delta 材质：可以在 BSDF 和 guiding 之间选策略
            // -------------------------------------------------

            bool chooseGuiding = canUseGuiding && randomDouble() < pGuiding;

            if (chooseGuiding) {
                ++gStats.guidedStrategySamples;

                LocalGuidedSample guidedSample =
                    sampleGuidedLocalAt(rec.p, rec.shadingNormal, wo, guidingMode);

                if (!guidedSample.valid || guidedSample.pdf <= 1e-12) {
                    // guiding 无效时回退到最初的 BSDF sample
                    ++gStats.guidedInvalidSamples;
                    ++gStats.guidedFallbackSamples;
                    ++gStats.bsdfStrategySamples;
                    bsdfSample = initialSample;

                    wi = bsdfSample.wi.normalized();
                    f = applyHitBaseColor(rec, bsdfSample.f);
                    pdfBsdf = rec.material->pdfValue( wo, rec.shadingNormal, wi);

                    Frame frame(rec.shadingNormal);

                    Vec3 localWi =
                        frame.toLocal(wi).normalized();

                    pdfGuided =
                        guidedPdfLocalAt(
                            rec.p,
                            rec.shadingNormal,
                            localWi,
                            guidingMode
                        );

                    usedGuidedSampling = false;
                }
                else {
                    Frame frame(rec.shadingNormal);

                    Vec3 localWi =
                        guidedSample.localWi.normalized();

                    wi =
                        frame.toWorld(localWi).normalized();

                    double cosCheck = std::max(0.0, dot(rec.shadingNormal.normalized(), wi));

                    if (cosCheck <= 0.0) {
                        // guided 方向在表面下方，回退到 BSDF sample
                        ++gStats.guidedBelowSurfaceSamples;
                        ++gStats.guidedFallbackSamples;
                        ++gStats.bsdfStrategySamples;
                        bsdfSample = initialSample;

                        wi = bsdfSample.wi.normalized();
                        f = applyHitBaseColor(rec, bsdfSample.f);
                        pdfBsdf =rec.material->pdfValue(wo, rec.shadingNormal,wi);

                        Vec3 fallbackLocalWi =
                            frame.toLocal(wi).normalized();

                        pdfGuided =
                            guidedPdfLocalAt(
                                rec.p,
                                rec.shadingNormal,
                                fallbackLocalWi,
                                guidingMode
                            );

                        usedGuidedSampling = false;
                    }
                    else {
                        f = applyHitBaseColor(
                            rec,
                            rec.material->eval(wo, rec.shadingNormal, wi)
                        );

                        pdfBsdf =rec.material->pdfValue(wo, rec.shadingNormal, wi);

                        pdfGuided = guidedSample.pdf;

                        bsdfSample.wi = wi;
                        bsdfSample.f = f;
                        bsdfSample.pdf = pdfBsdf;
                        bsdfSample.type = BSDFSampleType::Glossy;
                        bsdfSample.isDelta = false;
                        bsdfSample.valid = true;

                        usedGuidedSampling = true;
                    }
                }
            }
            else {
                ++gStats.bsdfStrategySamples;
                bsdfSample = initialSample;

                wi = bsdfSample.wi.normalized();
                f = applyHitBaseColor(rec, bsdfSample.f);

                pdfBsdf =
                    rec.material->pdfValue(wo, rec.shadingNormal, wi);

                if (canUseGuiding) {
                    Frame frame(rec.shadingNormal);

                    Vec3 localWi =
                        frame.toLocal(wi).normalized();

                    pdfGuided =
                        guidedPdfLocalAt(
                            rec.p,
                            rec.shadingNormal,
                            localWi,
                            guidingMode
                        );
                } else {
                    pdfGuided = 0.0;
                }

                usedGuidedSampling = false;
            }

            pdfFinal = pBsdf * pdfBsdf + pGuiding * pdfGuided;

            recordPdfStats(pdfBsdf,pdfGuided,pdfFinal);

            if (pdfFinal <= 1e-12 || isBadNumber(pdfFinal)) {
                break;
            }

            bsdfSample.pdf = pdfFinal;
        }

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
            vertex.bsdfPdf = bsdfSample.isDelta ? 0.0 : pdfFinal;
            vertex.lightPdf = bsdfSample.isDelta ? 0.0 : scene.lightPdfSum(rec.p, wi);
            vertex.cosTheta = bsdfSample.isDelta ? 1.0 : cosTheta;
            vertex.depth = bounce;
            vertex.eventType = bsdfSample.type;
            vertex.isDelta = bsdfSample.isDelta;
            vertex.valid = true;

            guidingRecord.addVertex(vertex);
            gGuidingDebugCollector.recordVertex(vertex);
            gGuidingTrainer.recordVertex(vertex);
            gOpenPGLGuiding.recordVertex(vertex);

            ++gStats.guidingVertices;
        }

        if (bsdfSample.isDelta) {
            beta = beta * f;
        }
        else {
            beta = beta * f * (cosTheta / pdfFinal);
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
        previousBsdfPdf = bsdfSample.isDelta ? 0.0 : pdfFinal;
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
        gOpenPGLGuiding.reset();
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
        gOpenPGLGuiding.build();

        gGuidingDebugCollector.print();
        gGuidingTrainer.printStats();
        gOpenPGLGuiding.printStats();
    }

    if (enableGuidedSampling && guidingMode == GuidingMode::OpenPGL) {
        gOpenPGLGuiding.printStats();
    }
}
