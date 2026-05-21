#pragma once

#include "core/aabb.h"
#include "guiding/local_hemisphere_histogram.h"
#include "guiding/spatial_guiding_grid.h"
#include "render/path_vertex.h"

#include <cstdint>

struct GuidingTrainerStats {
    std::uint64_t receivedVertices = 0;
    std::uint64_t trainedVertices = 0;
    std::uint64_t skippedDeltaVertices = 0;
    std::uint64_t skippedInvalidVertices = 0;
    std::uint64_t skippedBadWeightVertices = 0;

    double totalTrainingWeight = 0.0;
    double maxTrainingWeight = 0.0;
};

class GuidingTrainer {
public:
    GuidingTrainer();

    void reset();
    void configureSpatialGrid(const AABB& bounds, int nx, int ny, int nz);

    void recordVertex(const PathVertex& vertex);
    void build();

    LocalHemisphereHistogram& distribution() { return histogram; }
    const LocalHemisphereHistogram& distribution() const { return histogram; }

    SpatialGuidingGrid& spatialDistribution() { return spatialGrid; }
    const SpatialGuidingGrid& spatialDistribution() const { return spatialGrid; }

    const GuidingTrainerStats& stats() const { return trainerStats; }

    void printStats() const;

private:
    LocalHemisphereHistogram histogram;
    SpatialGuidingGrid spatialGrid;

    AABB spatialGridBounds;
    int spatialGridNx = 4;
    int spatialGridNy = 2;
    int spatialGridNz = 4;

    GuidingTrainerStats trainerStats;

    void resetSpatialGrid();

    double computeTrainingWeight(const PathVertex& vertex) const;
    double maxColorComponent(const Color& c) const;
    bool isBad(double x) const;
};