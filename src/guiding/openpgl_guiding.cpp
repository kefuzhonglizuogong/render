#include "guiding/openpgl_guiding.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef RENDER_ENABLE_OPENPGL
#include <openpgl/openpgl.h>
#endif

OpenPGLGuiding::OpenPGLGuiding() {
    reset();
}

void OpenPGLGuiding::reset() {
    guidingStats = OpenPGLGuidingStats();

#ifdef RENDER_ENABLE_OPENPGL
    // Open PGL state will be initialized here in the next step.
#endif
}

bool OpenPGLGuiding::enabled() const {
#ifdef RENDER_ENABLE_OPENPGL
    return true;
#else
    return false;
#endif
}

void OpenPGLGuiding::recordVertex(const PathVertex& vertex) {
    ++guidingStats.receivedVertices;

    if (!vertex.valid) {
        ++guidingStats.skippedInvalidVertices;
        return;
    }

    if (vertex.isDelta) {
        ++guidingStats.skippedDeltaVertices;
        return;
    }

    double weight = computeTrainingWeight(vertex);

    if (weight <= 0.0 || isBad(weight)) {
        ++guidingStats.skippedBadWeightVertices;
        return;
    }

#ifdef RENDER_ENABLE_OPENPGL
    // In the next step, this PathVertex will be converted to Open PGL sample data.
    ++guidingStats.recordedVertices;
#else
    ++guidingStats.recordedVertices;
#endif
}

void OpenPGLGuiding::build() {
    ++guidingStats.buildCount;

#ifdef RENDER_ENABLE_OPENPGL
    // Open PGL field training will be added here in the next step.
#endif
}

OpenPGLGuidedSample OpenPGLGuiding::sample(const Point3& position, const Vec3& normal, const Vec3& wo) {
    ++guidingStats.sampleRequests;

    OpenPGLGuidedSample result;
    result.wi = normal.normalized();
    result.pdf = 0.0;
    result.valid = false;

#ifdef RENDER_ENABLE_OPENPGL
    // Open PGL guided direction sampling will be added later.
    ++guidingStats.failedSamples;
#else
    ++guidingStats.failedSamples;
#endif

    return result;
}

double OpenPGLGuiding::pdf(const Point3& position, const Vec3& normal, const Vec3& wi) const {
#ifdef RENDER_ENABLE_OPENPGL
    // Open PGL pdf query will be added later.
    return 0.0;
#else
    return 0.0;
#endif
}

const OpenPGLGuidingStats& OpenPGLGuiding::stats() const {
    return guidingStats;
}

void OpenPGLGuiding::printStats() const {
    std::cout << "\n=== Open PGL Guiding Stats ===\n";

#ifdef RENDER_ENABLE_OPENPGL
    std::cout << "Open PGL enabled: yes\n";
#else
    std::cout << "Open PGL enabled: no\n";
#endif

    std::cout << "Received vertices: " << guidingStats.receivedVertices << "\n";
    std::cout << "Recorded vertices: " << guidingStats.recordedVertices << "\n";
    std::cout << "Skipped invalid vertices: " << guidingStats.skippedInvalidVertices << "\n";
    std::cout << "Skipped delta vertices: " << guidingStats.skippedDeltaVertices << "\n";
    std::cout << "Skipped bad weight vertices: " << guidingStats.skippedBadWeightVertices << "\n";
    std::cout << "Build count: " << guidingStats.buildCount << "\n";
    std::cout << "Sample requests: " << guidingStats.sampleRequests << "\n";
    std::cout << "Valid samples: " << guidingStats.validSamples << "\n";
    std::cout << "Failed samples: " << guidingStats.failedSamples << "\n";

    std::cout << "==============================\n";
}

bool OpenPGLGuiding::isBad(double x) const {
    return std::isnan(x) || std::isinf(x);
}

double OpenPGLGuiding::maxColorComponent(const Color& c) const {
    return std::max(c.x, std::max(c.y, c.z));
}

double OpenPGLGuiding::computeTrainingWeight(const PathVertex& vertex) const {
    if (vertex.isDelta) {
        return 0.0;
    }

    if (vertex.bsdfPdf <= 1e-12) {
        return 0.0;
    }

    double throughput = maxColorComponent(vertex.throughput);
    double bsdf = maxColorComponent(vertex.bsdfValue);
    double cosTheta = std::max(0.0, vertex.cosTheta);

    double weight = throughput * bsdf * cosTheta / vertex.bsdfPdf;

    if (weight <= 0.0 || isBad(weight)) {
        return 0.0;
    }

    const double maxWeight = 100.0;
    return std::min(weight, maxWeight);
}