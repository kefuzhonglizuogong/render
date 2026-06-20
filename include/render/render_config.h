#pragma once

#include <string>

struct RenderConfig {
    // =====================================================
    // Image
    // =====================================================

    int imageWidth = 400;
    double aspectRatio = 16.0 / 9.0;

    int samplesPerPixel = 200;//200
    int maxDepth = 12;

    int trainingSamplesPerPixel = 128;
    double guidingProbability = 0.8;

    // =====================================================
    // Acceleration
    // =====================================================

    bool enableBVH = true;

    // =====================================================
    // Assets
    // =====================================================

    std::string objPath =  "models/bunny.obj";

    std::string environmentPath = "models/test_env.ppm";

    std::string outputPath;

    // =====================================================
    // Environment
    // =====================================================

    double environmentIntensity = 8.0;

    // =====================================================
    // Mesh placement
    // =====================================================

    double meshTargetSize = 0.65;

    // =====================================================
    // Guiding comparison
    // =====================================================

    bool runGuidingComparison = true;
    std::string baselineOutputPath;
    std::string guidedOutputPath;

    std::string globalGuidedOutputPath;
    std::string spatialGuidedOutputPath;
    std::string openPGLGuidedOutputPath;


    double meshCenterX = -0.3;
    double meshCenterY = 0.0;
    double meshCenterZ = -1.0;
};
