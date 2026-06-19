#pragma once

#include "core/vec3.h"
#include "render/path_vertex.h"

#include <cstdint>
#include <vector>

struct OpenPGLGuidedSample {
    Vec3 wi;
    double pdf = 0.0;
    bool valid = false;
};

struct OpenPGLTrainingSample {
    Point3 position;
    Vec3 normal;
    Vec3 directionIn;
    Vec3 directionOut;
    Color radianceIn;
    Color throughput;
    double weight = 0.0;
    double bsdfPdf = 0.0;
    double cosTheta = 0.0;
    int depth = 0;
    bool valid = false;
};

struct OpenPGLGuidingStats {
    std::uint64_t receivedVertices = 0;
    std::uint64_t recordedVertices = 0;
    std::uint64_t skippedInvalidVertices = 0;
    std::uint64_t skippedDeltaVertices = 0;
    std::uint64_t skippedBadWeightVertices = 0;
    std::uint64_t skippedBelowSurfaceVertices = 0;
    std::uint64_t storedSamples = 0;
    std::uint64_t buildCount = 0;
    std::uint64_t sampleRequests = 0;
    std::uint64_t validSamples = 0;
    std::uint64_t failedSamples = 0;

    double totalTrainingWeight = 0.0;
    double maxTrainingWeight = 0.0;
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
    const std::vector<OpenPGLTrainingSample>& samples() const;

    void printStats() const;

private:
    OpenPGLGuidingStats guidingStats;
    std::vector<OpenPGLTrainingSample> trainingSamples;

    OpenPGLTrainingSample convertVertexToSample(const PathVertex& vertex, double weight) const;

    double computeTrainingWeight(const PathVertex& vertex) const;
    double maxColorComponent(const Color& c) const;
    bool isBad(double x) const;
    bool isBadVector(const Vec3& v) const;
};