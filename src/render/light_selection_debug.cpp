#include "render/light_selection_debug.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void debugLightSelection(
    const Scene& scene,
    int sampleCount
) {
    if (scene.lights.empty()) {
        std::cout << "\n=== Light Selection Debug ===\n";
        std::cout << "No lights in scene.\n";
        std::cout << "=============================\n";
        return;
    }

    int lightCount =
        static_cast<int>(scene.lights.size());

    std::vector<int> selectedCounts(
        static_cast<size_t>(lightCount),
        0
    );

    std::vector<double> weights(
        static_cast<size_t>(lightCount),
        0.0
    );

    double totalWeight = 0.0;

    for (int i = 0; i < lightCount; ++i) {
        if (!scene.lights[i]) {
            continue;
        }

        double w =
            std::max(
                0.0,
                scene.lights[i]->selectionWeight()
            );

        weights[static_cast<size_t>(i)] = w;
        totalWeight += w;
    }

    int validSamples = 0;
    int invalidSamples = 0;

    for (int i = 0; i < sampleCount; ++i) {
        LightSelectionSample selected =
            scene.sampleLight();

        if (!selected.valid || !selected.light) {
            ++invalidSamples;
            continue;
        }

        bool found = false;

        for (int j = 0; j < lightCount; ++j) {
            if (scene.lights[j] == selected.light) {
                ++selectedCounts[static_cast<size_t>(j)];
                found = true;
                break;
            }
        }

        if (found) {
            ++validSamples;
        }
        else {
            ++invalidSamples;
        }
    }

    std::cout << "\n=== Light Selection Debug ===\n";
    std::cout << "Sample count:    " << sampleCount << "\n";
    std::cout << "Valid samples:   " << validSamples << "\n";
    std::cout << "Invalid samples: " << invalidSamples << "\n";
    std::cout << "Light count:     " << lightCount << "\n";
    std::cout << "Total weight:    " << totalWeight << "\n\n";

    for (int i = 0; i < lightCount; ++i) {
        double theoreticalPdf = 0.0;

        if (totalWeight > 0.0) {
            theoreticalPdf =
                weights[static_cast<size_t>(i)] /
                totalWeight;
        }
        else {
            theoreticalPdf =
                1.0 / static_cast<double>(lightCount);
        }

        double observedPdf = 0.0;

        if (validSamples > 0) {
            observedPdf =
                static_cast<double>(
                    selectedCounts[static_cast<size_t>(i)]
                ) /
                static_cast<double>(validSamples);
        }

        double absError =
            std::abs(observedPdf - theoreticalPdf);

        std::cout << "Light " << i << "\n";
        std::cout << "  weight:          "
                  << weights[static_cast<size_t>(i)] << "\n";
        std::cout << "  theoretical pdf: "
                  << theoreticalPdf << "\n";
        std::cout << "  observed pdf:    "
                  << observedPdf << "\n";
        std::cout << "  abs error:       "
                  << absError << "\n";
    }

    std::cout << "=============================\n";
}
