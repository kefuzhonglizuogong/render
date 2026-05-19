#include "guiding/guiding_trainer.h"
#include "core/frame.h"

#include <algorithm>
#include <cmath>
#include <iostream>

/*
Receive PathVertex samples
Compute training weights
Update LocalHemisphereHistogram
Build distribution after rendering
Print training stats
*/

GuidingTrainer::GuidingTrainer() : histogram(16, 32) {
}

void GuidingTrainer::reset() {
    histogram.reset();
    trainerStats = GuidingTrainerStats();
}

bool GuidingTrainer::isBad(double x) const {
    return std::isnan(x) || std::isinf(x);
}

double GuidingTrainer::maxColorComponent(const Color& c) const {
    return std::max(c.x, std::max(c.y, c.z));
}

// Convert a recorded vertex into a safe training weight for the histogram.
double GuidingTrainer::computeTrainingWeight(const PathVertex& vertex) const {
    // First-pass estimate:
    //
    // throughput * bsdfValue * cosTheta / bsdfPdf
    //
    // This approximates how valuable the sampled direction is.

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

void GuidingTrainer::recordVertex(const PathVertex& vertex) {
    ++trainerStats.receivedVertices;

    if (!vertex.valid) {
        ++trainerStats.skippedInvalidVertices;
        return;
    }

    if (vertex.isDelta) {
        ++trainerStats.skippedDeltaVertices;
        return;
    }

    double weight = computeTrainingWeight(vertex);

    if (weight <= 0.0 || isBad(weight)) {
        ++trainerStats.skippedBadWeightVertices;
        return;
    }

    Frame frame(vertex.shadingNormal);

    Vec3 localWi =
        frame.toLocal(vertex.wi).normalized();

    if (localWi.z <= 0.0) {
        ++trainerStats.skippedBadWeightVertices;
        return;
    }

    histogram.update(
        localWi,
        weight
    );

    ++trainerStats.trainedVertices;
    trainerStats.totalTrainingWeight += weight;
    trainerStats.maxTrainingWeight =
        std::max(trainerStats.maxTrainingWeight, weight);
}

void GuidingTrainer::build() {
    histogram.buildDistribution();
}

void GuidingTrainer::printStats() const {
    std::cout << "\n=== Guiding Trainer Stats ===\n";
    std::cout << "Received vertices:       " << trainerStats.receivedVertices << "\n";
    std::cout << "Trained vertices:        " << trainerStats.trainedVertices << "\n";
    std::cout << "Skipped delta vertices:  " << trainerStats.skippedDeltaVertices << "\n";
    std::cout << "Skipped invalid vertices:" << trainerStats.skippedInvalidVertices << "\n";
    std::cout << "Skipped bad weights:     " << trainerStats.skippedBadWeightVertices << "\n";
    std::cout << "Total training weight:   " << trainerStats.totalTrainingWeight << "\n";
    std::cout << "Max training weight:     " << trainerStats.maxTrainingWeight << "\n";
    std::cout << "Histogram total weight:  " << histogram.getTotalWeight() << "\n";

    if (trainerStats.trainedVertices > 0) {
        std::cout << "Avg training weight:     "
                  << trainerStats.totalTrainingWeight /
                  static_cast<double>(trainerStats.trainedVertices)
                  << "\n";
    }

    std::cout << "=============================\n";
}
