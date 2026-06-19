#include "guiding/openpgl_guiding.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef RENDER_ENABLE_OPENPGL
#include <openpgl/cpp/OpenPGL.h>
#endif

#ifdef RENDER_ENABLE_OPENPGL
struct OpenPGLGuiding::OpenPGLRuntime {
    std::unique_ptr<openpgl::cpp::Device> device;
    openpgl::cpp::FieldConfig fieldConfig;
    std::unique_ptr<openpgl::cpp::Field> field;
    std::unique_ptr<openpgl::cpp::SampleStorage> sampleStorage;

    OpenPGLRuntime() {
        device = std::make_unique<openpgl::cpp::Device>(PGL_DEVICE_TYPE_CPU_4);

        fieldConfig.Init(
            PGL_SPATIAL_STRUCTURE_KDTREE,
            PGL_DIRECTIONAL_DISTRIBUTION_PARALLAX_AWARE_VMM,
            true,
            32000
        );

        fieldConfig.SetSpatialStructureArgMaxDepth(16);

        field = std::make_unique<openpgl::cpp::Field>(device.get(), fieldConfig);
        sampleStorage = std::make_unique<openpgl::cpp::SampleStorage>();
    }
};
#else
struct OpenPGLGuiding::OpenPGLRuntime {
};
#endif

OpenPGLGuiding::OpenPGLGuiding()
    : sceneBounds(Point3(-10.0, -2.0, -10.0), Point3(10.0, 5.0, 5.0)) {
    reset();
}

OpenPGLGuiding::~OpenPGLGuiding() = default;

void OpenPGLGuiding::reset() {
    guidingStats = OpenPGLGuidingStats();
    trainingSamples.clear();

#ifdef RENDER_ENABLE_OPENPGL
    runtime.reset();
    initializeRuntime();
#endif
}

bool OpenPGLGuiding::enabled() const {
#ifdef RENDER_ENABLE_OPENPGL
    return true;
#else
    return false;
#endif
}

void OpenPGLGuiding::setSceneBounds(const AABB& bounds) {
    sceneBounds = bounds;
    applySceneBounds();
}

bool OpenPGLGuiding::initializeRuntime() {
#ifdef RENDER_ENABLE_OPENPGL
    if (runtime) {
        return true;
    }

    try {
        runtime = std::make_unique<OpenPGLRuntime>();

        guidingStats.runtimeInitialized = runtime != nullptr;
        guidingStats.fieldInitialized = runtime && runtime->field != nullptr;
        guidingStats.sampleStorageInitialized = runtime && runtime->sampleStorage != nullptr;

        applySceneBounds();

        return guidingStats.runtimeInitialized && guidingStats.fieldInitialized && guidingStats.sampleStorageInitialized;
    }
    catch (const std::exception& e) {
        ++guidingStats.runtimeInitFailures;
        runtime.reset();

        std::cout << "Open PGL initialization failed: " << e.what() << "\n";

        return false;
    }
#else
    return false;
#endif
}

void OpenPGLGuiding::applySceneBounds() {
#ifdef RENDER_ENABLE_OPENPGL
    if (!runtime || !runtime->field) {
        return;
    }

    pgl_box3f bounds;
    pglBox3f(
        bounds,
        static_cast<float>(sceneBounds.minimum.x),
        static_cast<float>(sceneBounds.minimum.y),
        static_cast<float>(sceneBounds.minimum.z),
        static_cast<float>(sceneBounds.maximum.x),
        static_cast<float>(sceneBounds.maximum.y),
        static_cast<float>(sceneBounds.maximum.z)
    );

    runtime->field->SetSceneBounds(bounds);
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
}

void OpenPGLGuiding::build() {
    ++guidingStats.buildCount;

#ifdef RENDER_ENABLE_OPENPGL
    if (!initializeRuntime()) {
        return;
    }

    if (runtime && runtime->sampleStorage) {
        guidingStats.openPGLSurfaceSamples = static_cast<std::uint64_t>(runtime->sampleStorage->GetSizeSurface());
        guidingStats.openPGLVolumeSamples = static_cast<std::uint64_t>(runtime->sampleStorage->GetSizeVolume());
    }
#endif
}

OpenPGLGuidedSample OpenPGLGuiding::sample(const Point3& position, const Vec3& normal, const Vec3& wo) {
    ++guidingStats.sampleRequests;

    OpenPGLGuidedSample result;
    result.wi = normal.normalized();
    result.pdf = 0.0;
    result.valid = false;

#ifdef RENDER_ENABLE_OPENPGL
    ++guidingStats.failedSamples;
#else
    ++guidingStats.failedSamples;
#endif

    return result;
}

double OpenPGLGuiding::pdf(const Point3& position, const Vec3& normal, const Vec3& wi) const {
#ifdef RENDER_ENABLE_OPENPGL
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

    std::cout << "Runtime initialized: " << (guidingStats.runtimeInitialized ? "yes" : "no") << "\n";
    std::cout << "Field initialized: " << (guidingStats.fieldInitialized ? "yes" : "no") << "\n";
    std::cout << "Sample storage initialized: " << (guidingStats.sampleStorageInitialized ? "yes" : "no") << "\n";
    std::cout << "Runtime init failures: " << guidingStats.runtimeInitFailures << "\n";

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

    std::cout << "Open PGL surface samples: " << guidingStats.openPGLSurfaceSamples << "\n";
    std::cout << "Open PGL volume samples: " << guidingStats.openPGLVolumeSamples << "\n";

    std::cout << "Build count: " << guidingStats.buildCount << "\n";
    std::cout << "Sample requests: " << guidingStats.sampleRequests << "\n";
    std::cout << "Valid samples: " << guidingStats.validSamples << "\n";
    std::cout << "Failed samples: " << guidingStats.failedSamples << "\n";

    std::cout << "==============================\n";
}

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