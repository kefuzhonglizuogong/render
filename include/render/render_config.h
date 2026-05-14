#pragma once

#include <string>

struct RenderConfig {
    // =====================================================
    // Image
    // =====================================================

    int imageWidth = 400;
    double aspectRatio = 16.0 / 9.0;

    int samplesPerPixel = 100;
    int maxDepth = 12;

    // =====================================================
    // Acceleration
    // =====================================================

    bool enableBVH = true;

    // =====================================================
    // Assets
    // =====================================================

    std::string objPath =
        "models/bunny.obj";

    std::string environmentPath =
        "models/test_env.ppm";

    std::string outputPath;

    // =====================================================
    // Environment
    // =====================================================

    double environmentIntensity = 8.0;

    // =====================================================
    // Mesh placement
    // =====================================================

    double meshTargetSize = 1.0;

    double meshCenterX = 0.0;
    double meshCenterY = -0.5;
    double meshCenterZ = -2.4;
};