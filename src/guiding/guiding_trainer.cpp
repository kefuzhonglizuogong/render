#include "guiding/guiding_trainer.h"

#include <iostream>
#include <cmath>
#include <algorithm>
/*
接收 PathVertex
计算训练权重
更新 DirectionalHistogram
渲染结束后 buildDistribution
打印训练统计
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

//接收一个路径顶点 PathVertex，然后算出这个方向应该给 histogram 加多少权重
double GuidingTrainer::computeTrainingWeight(const PathVertex& vertex) const {
    // 第一版训练权重先用一个简单、安全的估计：
    //
    // throughput * bsdfValue * cosTheta / bsdfPdf
    //
    // 这近似代表当前方向对路径贡献的潜力。
    // 后面可以替换成更严格的 incident radiance / path contribution。

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
    // 防止极端 firefly 把 histogram 冲坏。
    // 第一版先做一个保守 clamp。
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

    histogram.update(vertex.wi, weight);

    ++trainerStats.trainedVertices;

    trainerStats.totalTrainingWeight += weight;

    trainerStats.maxTrainingWeight = std::max( trainerStats.maxTrainingWeight, weight);
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
        std::cout << "Avg training weight:     " << trainerStats.totalTrainingWeight / static_cast<double>(trainerStats.trainedVertices) << "\n";
    }

    std::cout << "=============================\n";
}