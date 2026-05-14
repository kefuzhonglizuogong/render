#include "light/environment_debug.h"

#include "core/random.h"

#include <iostream>
#include <cmath>
#include <algorithm>

namespace {
    constexpr double PI = 3.14159265358979323846;

    double luminance(const Color& c) {
        return 0.2126 * c.x +
            0.7152 * c.y +
            0.0722 * c.z;
    }

    Vec3 sampleUniformSphere() {
        double u1 = randomDouble();
        double u2 = randomDouble();

        double z = 1.0 - 2.0 * u1;
        double r = std::sqrt(std::max(0.0, 1.0 - z * z));
        double phi = 2.0 * PI * u2;

        double x = r * std::cos(phi);
        double y = z;
        double zz = r * std::sin(phi);

        return Vec3(x, y, zz).normalized();
    }

    bool isBadNumber(double x) {
        return std::isnan(x) || std::isinf(x);
    }
}

void debugEnvironmentSampling(
    const EnvironmentLight& environment,
    int sampleCount
) {
    int validSamples = 0;
    int invalidSamples = 0;
    int zeroPdfSamples = 0;
    int mismatchedPdfSamples = 0;
    int badNumberSamples = 0;

    double minPdf = 1e30;
    double maxPdf = 0.0;
    double avgPdf = 0.0;

    double avgSampledLuminance = 0.0;
    double maxSampledLuminance = 0.0;

    double avgUniformLuminance = 0.0;
    double maxUniformLuminance = 0.0;

    Point3 refPoint(0.0, 0.0, 0.0);

    for (int i = 0; i < sampleCount; ++i) {
        LightSample lightSample;

        bool ok = environment.sample(refPoint, lightSample);

        if (!ok || lightSample.pdf <= 1e-12) {
            ++invalidSamples;

            if (ok && lightSample.pdf <= 1e-12) {
                ++zeroPdfSamples;
            }

            continue;
        }

        ++validSamples;

        Vec3 wi = lightSample.wi.normalized();

        double pdfFromSample = lightSample.pdf;
        double pdfFromQuery = environment.pdf(refPoint, wi);

        if (
            isBadNumber(pdfFromSample) ||
            isBadNumber(pdfFromQuery)
            ) {
            ++badNumberSamples;
            continue;
        }

        double pdfDiff = std::abs(pdfFromSample - pdfFromQuery);
        double pdfRelDiff =
            pdfDiff / std::max(1e-12, pdfFromQuery);

        if (pdfRelDiff > 0.25) {
            ++mismatchedPdfSamples;
        }

        minPdf = std::min(minPdf, pdfFromSample);
        maxPdf = std::max(maxPdf, pdfFromSample);
        avgPdf += pdfFromSample;

        Color sampledRadiance = environment.eval(wi);
        double sampledLum = luminance(sampledRadiance);

        avgSampledLuminance += sampledLum;
        maxSampledLuminance =
            std::max(maxSampledLuminance, sampledLum);

        Vec3 uniformWi = sampleUniformSphere();
        Color uniformRadiance = environment.eval(uniformWi);
        double uniformLum = luminance(uniformRadiance);

        avgUniformLuminance += uniformLum;
        maxUniformLuminance =
            std::max(maxUniformLuminance, uniformLum);
    }

    if (validSamples > 0) {
        avgPdf /= static_cast<double>(validSamples);
        avgSampledLuminance /= static_cast<double>(validSamples);
        avgUniformLuminance /= static_cast<double>(validSamples);
    }

    std::cout << "\n=== Environment Sampling Debug ===\n";
    std::cout << "Sample count:              " << sampleCount << "\n";
    std::cout << "Valid samples:             " << validSamples << "\n";
    std::cout << "Invalid samples:           " << invalidSamples << "\n";
    std::cout << "Zero pdf samples:          " << zeroPdfSamples << "\n";
    std::cout << "Bad number samples:        " << badNumberSamples << "\n";
    std::cout << "Pdf mismatch samples:      " << mismatchedPdfSamples << "\n";

    std::cout << "Min pdf:                   " << minPdf << "\n";
    std::cout << "Max pdf:                   " << maxPdf << "\n";
    std::cout << "Avg pdf:                   " << avgPdf << "\n";

    std::cout << "Avg sampled luminance:     " << avgSampledLuminance << "\n";
    std::cout << "Max sampled luminance:     " << maxSampledLuminance << "\n";

    std::cout << "Avg uniform luminance:     " << avgUniformLuminance << "\n";
    std::cout << "Max uniform luminance:     " << maxUniformLuminance << "\n";

    if (avgUniformLuminance > 1e-12) {
        std::cout << "Sampled / Uniform avg lum: "
            << avgSampledLuminance / avgUniformLuminance
            << "\n";
    }

    std::cout << "==================================\n";
}