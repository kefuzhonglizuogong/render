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
    trainingSamples.clear();

#ifdef RENDER_ENABLE_OPENPGL
    // Open PGL device / field / sample storage will be initialized later.
#endif
}

bool OpenPGLGuiding::enabled() const {
#ifdef RENDER_ENABLE_OPENPGL
    return true;
#else
    return false;
#endif
}

//接收一个路径顶点 PathVertex，尝试把它转换成 Open PGL 将来可以使用的训练样本
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

    OpenPGLTrainingSample sample = convertVertexToSample(vertex, weight);

    if (!sample.valid) {
        ++guidingStats.skippedBadWeightVertices;
        return;
    }

    trainingSamples.push_back(sample);

    ++guidingStats.recordedVertices;
    ++guidingStats.storedSamples;

    guidingStats.totalTrainingWeight += weight;
    guidingStats.maxTrainingWeight = std::max(guidingStats.maxTrainingWeight, weight);

#ifdef RENDER_ENABLE_OPENPGL
    // Later: convert OpenPGLTrainingSample to real Open PGL SampleData here.
#endif
}

void OpenPGLGuiding::build() {
    ++guidingStats.buildCount;

#ifdef RENDER_ENABLE_OPENPGL
    // Later: build / update Open PGL Field from trainingSamples.
#endif
}

OpenPGLGuidedSample OpenPGLGuiding::sample(const Point3& position, const Vec3& normal, const Vec3& wo) {
    ++guidingStats.sampleRequests;

    OpenPGLGuidedSample result;
    result.wi = normal.normalized();
    result.pdf = 0.0;
    result.valid = false;

#ifdef RENDER_ENABLE_OPENPGL
    // Later: query Open PGL surface distribution and sample direction.
    ++guidingStats.failedSamples;
#else
    ++guidingStats.failedSamples;
#endif

    return result;
}

double OpenPGLGuiding::pdf(const Point3& position, const Vec3& normal, const Vec3& wi) const {
#ifdef RENDER_ENABLE_OPENPGL
    // Later: query Open PGL pdf.
    return 0.0;
#else
    return 0.0;
#endif
}

const OpenPGLGuidingStats& OpenPGLGuiding::stats() const {
    return guidingStats;
}

const std::vector<OpenPGLTrainingSample>& OpenPGLGuiding::samples() const {
    return trainingSamples;
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
    std::cout << "Stored samples: " << guidingStats.storedSamples << "\n";
    std::cout << "Skipped invalid vertices: " << guidingStats.skippedInvalidVertices << "\n";
    std::cout << "Skipped delta vertices: " << guidingStats.skippedDeltaVertices << "\n";
    std::cout << "Skipped bad weight vertices: " << guidingStats.skippedBadWeightVertices << "\n";
    std::cout << "Skipped below surface vertices: " << guidingStats.skippedBelowSurfaceVertices << "\n";
    std::cout << "Total training weight: " << guidingStats.totalTrainingWeight << "\n";
    std::cout << "Max training weight: " << guidingStats.maxTrainingWeight << "\n";

    if (guidingStats.recordedVertices > 0) {
        std::cout << "Avg training weight: "
            << guidingStats.totalTrainingWeight / static_cast<double>(guidingStats.recordedVertices)
            << "\n";
    }

    std::cout << "Build count: " << guidingStats.buildCount << "\n";
    std::cout << "Sample requests: " << guidingStats.sampleRequests << "\n";
    std::cout << "Valid samples: " << guidingStats.validSamples << "\n";
    std::cout << "Failed samples: " << guidingStats.failedSamples << "\n";

    std::cout << "==============================\n";
}

//把 PathVertex 转成 OpenPGLTrainingSample
OpenPGLTrainingSample OpenPGLGuiding::convertVertexToSample(const PathVertex& vertex, double weight) const {
    OpenPGLTrainingSample sample;

    if (isBadVector(vertex.position) || isBadVector(vertex.shadingNormal) || isBadVector(vertex.wi) || isBadVector(vertex.wo)) {
        return sample;
    }

    Vec3 normal = vertex.shadingNormal.normalized();
    Vec3 directionIn = vertex.wi.normalized();
    Vec3 directionOut = vertex.wo.normalized();

    double cosTheta = std::max(0.0, dot(normal, directionIn));

    if (cosTheta <= 0.0) {
        return sample;
    }

    sample.position = vertex.position;
    sample.normal = normal;
    sample.directionIn = directionIn;
    sample.directionOut = directionOut;
    sample.throughput = vertex.throughput;
    sample.bsdfPdf = vertex.bsdfPdf;
    sample.cosTheta = cosTheta;
    sample.depth = vertex.depth;
    sample.weight = weight;

    sample.radianceIn = Color(weight, weight, weight);
    sample.valid = true;

    return sample;
}

bool OpenPGLGuiding::isBad(double x) const {
    return std::isnan(x) || std::isinf(x);
}

bool OpenPGLGuiding::isBadVector(const Vec3& v) const {
    return isBad(v.x) || isBad(v.y) || isBad(v.z);
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