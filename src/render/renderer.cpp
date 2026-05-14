#include "render/renderer.h"
#include "material/material.h"
#include "core/random.h"
#include "light/light.h"
#include "material/bsdf_sample.h"

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

        LightSelectionSample lightSelection =
            scene.sampleLight();

        if (!lightSelection.valid || !lightSelection.light) {
            return Color(0.0, 0.0, 0.0);
        }

        const auto& light = lightSelection.light;
        double lightSelectionPdf =
            lightSelection.selectionPdf;

        LightSample lightSample;

        if (!light->sample(rec.p, lightSample)) {
            return Color(0.0, 0.0, 0.0);
        }

        Vec3 wi = lightSample.wi.normalized();

        double cosSurface = std::max(
            0.0,
            dot(rec.shadingNormal.normalized(), wi)
        );

        if (cosSurface <= 0.0) {
            return Color(0.0, 0.0, 0.0);
        }

        double pdfLight =
            lightSelectionPdf * lightSample.pdf;

        if (pdfLight <= 1e-12) {
            return Color(0.0, 0.0, 0.0);
        }

        Ray shadowRay(rec.p + rec.geometricNormal * 1e-4, wi);

        double shadowTMax =
            lightSample.isInfinite ? 1e30 : lightSample.distance - 1e-4;

        HitRecord shadowRec;
        if (scene.intersect(shadowRay, 1e-4, shadowTMax, shadowRec)) {
            return Color(0.0, 0.0, 0.0);
        }

        Color f = rec.material->eval(wo, rec.shadingNormal, wi);

        if (isBlack(f)) {
            return Color(0.0, 0.0, 0.0);
        }

        double pdfBsdf =
            rec.material->pdfValue(wo, rec.shadingNormal, wi);

        double misWeight =
            powerHeuristic(pdfLight, pdfBsdf);

        return f * lightSample.emission * (cosSurface / pdfLight) * misWeight;
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
    bool previousWasDelta = false;

    for (int bounce = 0; bounce < depth; ++bounce) {
        HitRecord rec;

        if (!scene.intersect(ray, 1e-4, 1e30, rec)) {
            Vec3 unitDir = ray.direction.normalized();

            if (scene.environment) {
                Color envRadiance =
                    scene.environment->eval(unitDir);

                if (bounce == 0) {
                    L += beta * envRadiance;
                }
                else if (previousWasDelta) {
                    L += beta * envRadiance;
                }
                else if (previousWasBsdfSample) {
                    double pdfLight = scene.lightPdfSum(
                        previousPoint,
                        previousWi
                    );

                    double misWeight =
                        powerHeuristic(previousBsdfPdf, pdfLight);

                    L += beta * envRadiance * misWeight;
                }
                else {
                    L += beta * envRadiance;
                }
            }
            else {
                double t = 0.5 * (unitDir.y + 1.0);

                Color background =
                    (1.0 - t) * Color(1.0, 1.0, 1.0) +
                    t * Color(0.5, 0.7, 1.0);

                L += beta * background;
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
            else if (previousWasDelta) {
                L += beta * emitted;
            }
            else if (previousWasBsdfSample) {
                double pdfLight = scene.lightPdfSum(
                    previousPoint,
                    previousWi
                );

                double misWeight =
                    powerHeuristic(previousBsdfPdf, pdfLight);

                L += beta * emitted * misWeight;
            }

            break;
        }

        Vec3 wo = (-ray.direction).normalized();

        Color directLight = estimateDirectLightMIS(rec, wo, scene);

        L += beta * directLight;

        BSDFSample bsdfSample =
            rec.material->sample(wo, rec.shadingNormal);

        if (!bsdfSample.valid || bsdfSample.pdf <= 1e-12) {
            break;
        }

        Vec3 wi = bsdfSample.wi.normalized();
        Color f = bsdfSample.f;
        double pdfBsdf = bsdfSample.pdf;

        if (bsdfSample.isDelta) {
            beta = beta * f;
        }
        else {
            double cosTheta = std::max(
                0.0,
                dot(rec.shadingNormal.normalized(), wi)
            );

            if (cosTheta <= 0.0) {
                break;
            }

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
