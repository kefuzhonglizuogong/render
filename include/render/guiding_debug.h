#pragma once

#include "render/path_vertex.h"

#include <vector>
#include <cstdint>

//路径引导调试收集
//不改变采样策略，只把已经记录的 PathVertex 汇总成统计数据，确认 Path Guiding 数据流是可信的
struct GuidingDebugStats {
    std::uint64_t totalVertices = 0;

    std::uint64_t diffuseVertices = 0;
    std::uint64_t glossyVertices = 0;
    std::uint64_t deltaReflectionVertices = 0;
    std::uint64_t deltaTransmissionVertices = 0;
    std::uint64_t noneVertices = 0;

    std::uint64_t deltaVertices = 0;
    std::uint64_t nonDeltaVertices = 0;

    std::uint64_t zeroBsdfPdfVertices = 0;
    std::uint64_t zeroLightPdfVertices = 0;

    std::uint64_t badNumberVertices = 0;

    double minBsdfPdf = 1e30;
    double maxBsdfPdf = 0.0;
    double sumBsdfPdf = 0.0;

    double minLightPdf = 1e30;
    double maxLightPdf = 0.0;
    double sumLightPdf = 0.0;

    double minCosTheta = 1e30;
    double maxCosTheta = 0.0;
    double sumCosTheta = 0.0;

    double maxThroughput = 0.0;
    double sumThroughput = 0.0;

    static constexpr int maxTrackedDepth = 32;
    std::uint64_t depthCounts[maxTrackedDepth] = {};
};

class GuidingDebugCollector {
public:
    void reset();

    void recordVertex(const PathVertex& vertex);

    void print() const;

    const GuidingDebugStats& stats() const {
        return data;
    }

private:
    GuidingDebugStats data;

    bool isBad(double x) const;
    double maxColorComponent(const Color& c) const;
};