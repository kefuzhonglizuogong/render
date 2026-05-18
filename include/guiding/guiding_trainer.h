#pragma once

#include "guiding/directional_histogram.h"
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

    void recordVertex(const PathVertex& vertex);

    void build();

    DirectionalHistogram& distribution() {
        return histogram;
    }

    const DirectionalHistogram& distribution() const {
        return histogram;
    }

    const GuidingTrainerStats& stats() const {
        return trainerStats;
    }

    void printStats() const;

private:
    DirectionalHistogram histogram;
    GuidingTrainerStats trainerStats;

    double computeTrainingWeight(const PathVertex& vertex) const;

    double maxColorComponent(const Color& c) const;

    bool isBad(double x) const;
};