#pragma once

#include <cstdint>

struct RenderStats {
    std::uint64_t sceneIntersectCalls = 0;
    std::uint64_t guidingVertices = 0;

    std::uint64_t bsdfStrategySamples = 0;
    std::uint64_t guidedStrategySamples = 0;
    std::uint64_t guidedFallbackSamples = 0;
    std::uint64_t guidedBelowSurfaceSamples = 0;
    std::uint64_t guidedInvalidSamples = 0;

    std::uint64_t guidedPdfZeroSamples = 0;
    std::uint64_t guidedPdfBadSamples = 0;
    std::uint64_t finalPdfZeroSamples = 0;
    std::uint64_t finalPdfBadSamples = 0;

    double minBsdfPdf = 1e30;
    double maxBsdfPdf = 0.0;
    double sumBsdfPdf = 0.0;

    double minGuidedPdf = 1e30;
    double maxGuidedPdf = 0.0;
    double sumGuidedPdf = 0.0;

    double minFinalPdf = 1e30;
    double maxFinalPdf = 0.0;
    double sumFinalPdf = 0.0;

    std::uint64_t bvhNodeIntersectCalls = 0;
    std::uint64_t aabbHitCalls = 0;

    std::uint64_t sphereIntersectCalls = 0;
    std::uint64_t quadIntersectCalls = 0;
    std::uint64_t triangleIntersectCalls = 0;
    std::uint64_t meshIntersectCalls = 0;

    void reset() {
        sceneIntersectCalls = 0;
        guidingVertices = 0;

        bsdfStrategySamples = 0;
        guidedStrategySamples = 0;
        guidedFallbackSamples = 0;
        guidedBelowSurfaceSamples = 0;
        guidedInvalidSamples = 0;

        guidedPdfZeroSamples = 0;
        guidedPdfBadSamples = 0;
        finalPdfZeroSamples = 0;
        finalPdfBadSamples = 0;

        minBsdfPdf = 1e30;
        maxBsdfPdf = 0.0;
        sumBsdfPdf = 0.0;

        minGuidedPdf = 1e30;
        maxGuidedPdf = 0.0;
        sumGuidedPdf = 0.0;

        minFinalPdf = 1e30;
        maxFinalPdf = 0.0;
        sumFinalPdf = 0.0;

        bvhNodeIntersectCalls = 0;
        aabbHitCalls = 0;

        sphereIntersectCalls = 0;
        quadIntersectCalls = 0;
        triangleIntersectCalls = 0;
        meshIntersectCalls = 0;
    }
};

extern RenderStats gStats;
