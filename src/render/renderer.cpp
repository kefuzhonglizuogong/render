#include "render/renderer.h"
#include "material/material.h"
#include "core/random.h"
#include "light/light.h"

#include <iostream>
#include <algorithm>
#include <cmath>


/*
powerHeuristic()          计算 MIS 权重
isBlack()                 判断颜色是否接近黑色
lightPdfSum()             计算某个方向被光源采样到的概率
estimateDirectLightMIS()  用 Light sampling 估计直接光
trace()                   追踪整条路径
render()                  遍历像素，多次采样，输出图像
*/

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

    int randomLightIndex(int lightCount) {
        int index = static_cast<int>(randomDouble() * lightCount);

        if (index < 0) {
            index = 0;
        }

        if (index >= lightCount) {
            index = lightCount - 1;
        }

        return index;
    }

    double lightPdfSum(
        const Scene& scene,
        const Point3& refPoint,
        const Vec3& wi
    ) {
        if (scene.lights.empty()) {
            return 0.0;
        }

        double pdf = 0.0;
        double selectPdf = 1.0 / static_cast<double>(scene.lights.size());

        for (const auto& light : scene.lights) {
            pdf += selectPdf * light->pdf(refPoint, wi);
        }

        return pdf;
    }

    //在当前交点 rec.p，随机选择一个光源，在这个光源上采样一个点，发 shadow ray 判断是否可见，如果可见，就计算直接光贡献，并乘上 MIS 权重。
    Color estimateDirectLightMIS(
        const HitRecord& rec,
        const Vec3& wo,
        const Scene& scene
    ) {
        if (!rec.material) {
            return Color(0.0, 0.0, 0.0);
        }

        if (scene.lights.empty()) {
            return Color(0.0, 0.0, 0.0);
        }

        int lightCount = static_cast<int>(scene.lights.size());
        int lightIndex = randomLightIndex(lightCount);

        const auto& light = scene.lights[lightIndex];

        double selectPdf = 1.0 / static_cast<double>(lightCount);

        LightSample lightSample;//在已经选中这个光源之后，这个光源内部自己的采样方法，采到方向 wi 的概率密度。

        if (!light->sample(rec.p, lightSample)) {
            return Color(0.0, 0.0, 0.0);
        }

        Vec3 wi = lightSample.wi.normalized();

        double cosSurface = std::max(
            0.0,
            dot(rec.normal.normalized(), wi)
        );

        if (cosSurface <= 0.0) {
            return Color(0.0, 0.0, 0.0);
        }

        //“按照整个光源采样策略，最终采到当前方向 wi 的总概率密度。”
        double pdfLight = selectPdf * lightSample.pdf;

        if (pdfLight <= 1e-12) {
            return Color(0.0, 0.0, 0.0);
        }

        Ray shadowRay(rec.p + rec.normal * 1e-4, wi);

        HitRecord shadowRec;
        if (scene.intersect(
            shadowRay,
            1e-4,
            lightSample.distance - 1e-4,
            shadowRec
        )) {
            return Color(0.0, 0.0, 0.0);
        }

        Color f = rec.material->eval(
            wo,
            rec.normal,
            wi
        );

        if (isBlack(f)) {
            return Color(0.0, 0.0, 0.0);
        }

        double pdfBsdf = rec.material->pdfValue(
            wo,
            rec.normal,
            wi
        );

        double misWeight = powerHeuristic(
            pdfLight,
            pdfBsdf
        );

        return f* lightSample.emission* (cosSurface / pdfLight)* misWeight;
    }
}

Renderer::Renderer(int spp, int depth)
    : samplesPerPixel(spp), maxDepth(depth) {
}

Color Renderer::trace(const Ray& rayIn, const Scene& scene, int depth) const {
    Color L(0.0, 0.0, 0.0);
    Color beta(1.0, 1.0, 1.0);

    Ray ray = rayIn;

    bool previousWasBsdfSample = false;
    double previousBsdfPdf = 0.0;
    Point3 previousPoint;
    Vec3 previousWi;

    for (int bounce = 0; bounce < depth; ++bounce) {
        HitRecord rec;

        if (!scene.intersect(ray, 1e-4, 1e30, rec)) {
            Vec3 unitDir = ray.direction.normalized();
            double t = 0.5 * (unitDir.y + 1.0);

            Color background =
                (1.0 - t) * Color(1.0, 1.0, 1.0) +
                t * Color(0.5, 0.7, 1.0);

            L += beta * background;
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
                double pdfLight = lightPdfSum(
                    scene,
                    previousPoint,
                    previousWi
                );

                double misWeight = powerHeuristic(
                    previousBsdfPdf,
                    pdfLight
                );

                L += beta * emitted * misWeight;
            }

            break;
        }

        Vec3 wo = (-ray.direction).normalized();

        Color directLight = estimateDirectLightMIS(rec,wo,scene);

        L += beta * directLight;

        Vec3 wi;
        Color f;
        double pdfBsdf = 0.0;

        bool ok = rec.material->sample(
            wo,
            rec.normal,
            wi,
            f,
            pdfBsdf
        );

        if (!ok || pdfBsdf <= 1e-12) {
            break;
        }

        wi = wi.normalized();

        double cosTheta = std::max(
            0.0,
            dot(rec.normal.normalized(), wi)
        );

        if (cosTheta <= 0.0) {
            break;
        }

        beta = beta * f * (cosTheta / pdfBsdf);

        if (bounce >= 3) {
            double p = std::min(maxComponent(beta), 0.95);

            if (randomDouble() > p) {
                break;
            }

            beta = beta / p;
        }

        previousWasBsdfSample = true;
        previousBsdfPdf = pdfBsdf;
        previousPoint = rec.p;
        previousWi = wi;

        ray = Ray(rec.p + rec.normal * 1e-4, wi);
    }

    return L;
}

void Renderer::render(const Scene& scene, const Camera& camera, Film& film) const {
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
}