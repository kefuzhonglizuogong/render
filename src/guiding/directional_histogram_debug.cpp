#include "guiding/directional_histogram_debug.h"

#include "guiding/directional_histogram.h"

#include <iostream>
#include <cmath>
#include <algorithm>

namespace {
    constexpr double PI = 3.14159265358979323846;

    double angleBetween(const Vec3& a,const Vec3& b) {
        double c = dot(a.normalized(), b.normalized());

        c = std::max(-1.0, std::min(1.0, c));

        return std::acos(c);
    }
}

void debugDirectionalHistogram() {
    DirectionalHistogram histogram(16, 32);

    Vec3 brightDir = Vec3(0.0, 1.0, 0.0).normalized();

    Vec3 secondaryDir = Vec3(1.0, 0.2, 0.0).normalized();

    // 给 brightDir 附近很高权重
    for (int i = 0; i < 1000; ++i) {
        histogram.update(
            brightDir,
            10.0
        );
    }

    // 给 secondaryDir 较低权重
    for (int i = 0; i < 1000; ++i) {
        histogram.update(secondaryDir, 2.0);
    }

    // 给一些背景方向小权重
    histogram.update(
        Vec3(0.0, -1.0, 0.0), 1.0
    );

    histogram.buildDistribution();

    int sampleCount = 100000;

    int brightHits = 0;
    int secondaryHits = 0;

    int invalidSamples = 0;
    int badPdfSamples = 0;

    double avgPdf = 0.0;
    double maxPdf = 0.0;

    for (int i = 0; i < sampleCount; ++i) {
        DirectionalSample sample = histogram.sample();

        if (!sample.valid) {
            ++invalidSamples;
            continue;
        }

        if (  sample.pdf <= 0.0 || std::isnan(sample.pdf) || std::isinf(sample.pdf) ) {
            ++badPdfSamples;
            continue;
        }

        avgPdf += sample.pdf;
        maxPdf = std::max(maxPdf, sample.pdf);

        double angleBright = angleBetween(sample.wi, brightDir);

        double angleSecondary = angleBetween( sample.wi, secondaryDir);

        if (angleBright < 0.25) {
            ++brightHits;
        }

        if (angleSecondary < 0.25) {
            ++secondaryHits;
        }
    }

    avgPdf /= static_cast<double>(sampleCount);

    std::cout << "\n=== Directional Histogram Debug ===\n";
    std::cout << "Total weight:       " << histogram.getTotalWeight() << "\n";

    std::cout << "Sample count:       " << sampleCount << "\n";

    std::cout << "Invalid samples:    " << invalidSamples << "\n";

    std::cout << "Bad pdf samples:    " << badPdfSamples << "\n";

    std::cout << "Bright hits:        " << brightHits << "\n";

    std::cout << "Secondary hits:     "  << secondaryHits << "\n";

    std::cout << "Bright hit ratio:   " << static_cast<double>(brightHits) / static_cast<double>(sampleCount) << "\n";

    std::cout << "Secondary hit ratio:" << static_cast<double>(secondaryHits) / static_cast<double>(sampleCount) << "\n";

    std::cout << "Avg pdf:            "  << avgPdf << "\n";

    std::cout << "Max pdf:            "  << maxPdf << "\n";

    std::cout << "===================================\n";
}