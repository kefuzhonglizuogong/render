#include "guiding/guiding_trainer.h"

#include "core/frame.h"

#include <algorithm>
#include <cmath>
#include <iostream>


/*
Receive PathVertex samples        接收路径采样顶点
Compute training weights          计算训练权重
Update global LocalHemisphereHistogram 更新全局半球直方图
Update spatial LocalHemisphereHistogram grid 更新空间网格直方图
Build distributions after rendering 训练完构建分布
Print training stats              打印训练统计
*/


GuidingTrainer::GuidingTrainer()
    : histogram(16, 32),
    spatialGridBounds(Point3(-10.0, -2.0, -10.0), Point3(10.0, 5.0, 5.0)) {
    resetSpatialGrid();
}

void GuidingTrainer::reset() {
    histogram.reset();
    resetSpatialGrid();
    trainerStats = GuidingTrainerStats();
}

void GuidingTrainer::configureSpatialGrid(const AABB& bounds, int nx, int ny, int nz) {
    spatialGridBounds = bounds;
    spatialGridNx = nx;
    spatialGridNy = ny;
    spatialGridNz = nz;
    resetSpatialGrid();
}

void GuidingTrainer::resetSpatialGrid() {
    spatialGrid.reset(spatialGridBounds, spatialGridNx, spatialGridNy, spatialGridNz);
}

bool GuidingTrainer::isBad(double x) const {
    return std::isnan(x) || std::isinf(x);
}

double GuidingTrainer::maxColorComponent(const Color& c) const {
    return std::max(c.x, std::max(c.y, c.z));
}

double GuidingTrainer::computeTrainingWeight(const PathVertex& vertex) const {
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

//训练入口
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
    Vec3 localWi = frame.toLocal(vertex.wi).normalized();

    if (localWi.z <= 0.0) {
        ++trainerStats.skippedBadWeightVertices;
        return;
    }

    histogram.update(localWi, weight);
    spatialGrid.recordLocalDirection(vertex.position, localWi, weight);

    ++trainerStats.trainedVertices;

    trainerStats.totalTrainingWeight += weight;
    trainerStats.maxTrainingWeight = std::max(trainerStats.maxTrainingWeight, weight);
}

void GuidingTrainer::build() {
    histogram.buildDistribution();
    spatialGrid.build();
}

void GuidingTrainer::printStats() const {
    std::cout << "\n=== Guiding Trainer Stats ===\n";

    std::cout << "Received vertices: " << trainerStats.receivedVertices << "\n";
    std::cout << "Trained vertices: " << trainerStats.trainedVertices << "\n";
    std::cout << "Skipped delta vertices: " << trainerStats.skippedDeltaVertices << "\n";
    std::cout << "Skipped invalid vertices: " << trainerStats.skippedInvalidVertices << "\n";
    std::cout << "Skipped bad weights: " << trainerStats.skippedBadWeightVertices << "\n";

    std::cout << "Total training weight: " << trainerStats.totalTrainingWeight << "\n";
    std::cout << "Max training weight: " << trainerStats.maxTrainingWeight << "\n";
    std::cout << "Histogram total weight: " << histogram.getTotalWeight() << "\n";

    std::cout << "Spatial grid total samples: " << spatialGrid.totalSamples() << "\n";
    std::cout << "Spatial grid active cells: " << spatialGrid.activeCells() << "\n";

    if (trainerStats.trainedVertices > 0) {
        std::cout << "Avg training weight: "
            << trainerStats.totalTrainingWeight / static_cast<double>(trainerStats.trainedVertices)
            << "\n";

        std::cout << "Spatial grid record ratio: "
            << static_cast<double>(spatialGrid.totalSamples()) / static_cast<double>(trainerStats.trainedVertices)
            << "\n";
    }

    std::cout << "=============================\n";
}