#pragma once

#include "core/vec3.h"
#include "render/path_vertex.h"

#include <cstdint>

struct OpenPGLGuidedSample {
    Vec3 wi;
    double pdf = 0.0;
    bool valid = false;
};

struct OpenPGLGuidingStats {
    std::uint64_t receivedVertices = 0;
    std::uint64_t recordedVertices = 0;
    std::uint64_t skippedInvalidVertices = 0;
    std::uint64_t skippedDeltaVertices = 0;
    std::uint64_t skippedBadWeightVertices = 0;
    std::uint64_t buildCount = 0;
    std::uint64_t sampleRequests = 0;
    std::uint64_t validSamples = 0;
    std::uint64_t failedSamples = 0;
};

class OpenPGLGuiding {
public:
    OpenPGLGuiding();

    void reset();
    bool enabled() const;

    void recordVertex(const PathVertex& vertex);
    void build();

    OpenPGLGuidedSample sample(const Point3& position, const Vec3& normal, const Vec3& wo);
    double pdf(const Point3& position, const Vec3& normal, const Vec3& wi) const;

    const OpenPGLGuidingStats& stats() const;
    void printStats() const;

private:
    OpenPGLGuidingStats guidingStats;

    double computeTrainingWeight(const PathVertex& vertex) const;
    double maxColorComponent(const Color& c) const;
    bool isBad(double x) const;
};