#include "render/guiding_debug.h"

#include <iostream>
#include <cmath>
#include <algorithm>

bool GuidingDebugCollector::isBad(double x) const {
    return std::isnan(x) || std::isinf(x);
}

double GuidingDebugCollector::maxColorComponent(const Color& c) const {
    return std::max(
        c.x,
        std::max(c.y, c.z)
    );
}

void GuidingDebugCollector::reset() {
    data = GuidingDebugStats();
}

void GuidingDebugCollector::recordVertex(const PathVertex& vertex) {
    if (!vertex.valid) {
        return;
    }

    ++data.totalVertices;

    int depth = vertex.depth;

    if (depth >= 0 &&depth < GuidingDebugStats::maxTrackedDepth) {
        ++data.depthCounts[depth];
    }

    switch (vertex.eventType) {
    case BSDFSampleType::Diffuse:
        ++data.diffuseVertices;
        break;

    case BSDFSampleType::Glossy:
        ++data.glossyVertices;
        break;

    case BSDFSampleType::DeltaReflection:
        ++data.deltaReflectionVertices;
        break;

    case BSDFSampleType::DeltaTransmission:
        ++data.deltaTransmissionVertices;
        break;

    case BSDFSampleType::None:
    default:
        ++data.noneVertices;
        break;
    }

    if (vertex.isDelta) {
        ++data.deltaVertices;
    }
    else {
        ++data.nonDeltaVertices;
    }

    if (vertex.bsdfPdf <= 1e-12) {
        ++data.zeroBsdfPdfVertices;
    }

    if (vertex.lightPdf <= 1e-12) {
        ++data.zeroLightPdfVertices;
    }

    if (isBad(vertex.bsdfPdf) || isBad(vertex.lightPdf) || isBad(vertex.cosTheta)) {
        ++data.badNumberVertices;
    }

    double throughputMax = maxColorComponent(vertex.throughput);

    if (isBad(throughputMax)) {
        ++data.badNumberVertices;
    }

    if (!vertex.isDelta && vertex.bsdfPdf > 0.0) {
        data.minBsdfPdf = std::min(data.minBsdfPdf, vertex.bsdfPdf);

        data.maxBsdfPdf = std::max(data.maxBsdfPdf, vertex.bsdfPdf);

        data.sumBsdfPdf += vertex.bsdfPdf;
    }

    if (!vertex.isDelta && vertex.lightPdf > 0.0) {
        data.minLightPdf = std::min(data.minLightPdf, vertex.lightPdf);

        data.maxLightPdf = std::max(data.maxLightPdf, vertex.lightPdf);

        data.sumLightPdf += vertex.lightPdf;
    }

    data.minCosTheta = std::min(data.minCosTheta, vertex.cosTheta);

    data.maxCosTheta = std::max(data.maxCosTheta, vertex.cosTheta);

    data.sumCosTheta += vertex.cosTheta;

    data.maxThroughput = std::max(data.maxThroughput, throughputMax);

    data.sumThroughput += throughputMax;
}

void GuidingDebugCollector::print() const {
    std::cout << "\n=== Guiding Debug Stats ===\n";

    std::cout << "Total vertices:              " << data.totalVertices << "\n";

    std::cout << "Diffuse vertices:            " << data.diffuseVertices << "\n";

    std::cout << "Glossy vertices:             " << data.glossyVertices << "\n";

    std::cout << "Delta reflection vertices:   " << data.deltaReflectionVertices << "\n";

    std::cout << "Delta transmission vertices: " << data.deltaTransmissionVertices << "\n";

    std::cout << "None vertices:               " << data.noneVertices << "\n";

    std::cout << "Delta vertices:              " << data.deltaVertices << "\n";

    std::cout << "Non-delta vertices:          " << data.nonDeltaVertices << "\n";

    std::cout << "Zero bsdf pdf vertices:      " << data.zeroBsdfPdfVertices << "\n";

    std::cout << "Zero light pdf vertices:     " << data.zeroLightPdfVertices << "\n";

    std::cout << "Bad number vertices:         " << data.badNumberVertices << "\n";

    double avgBsdfPdf = 0.0;
    double avgLightPdf = 0.0;
    double avgCosTheta = 0.0;
    double avgThroughput = 0.0;

    if (data.totalVertices > 0) {
        avgCosTheta =
            data.sumCosTheta /
            static_cast<double>(data.totalVertices);

        avgThroughput =
            data.sumThroughput /
            static_cast<double>(data.totalVertices);
    }

    if (data.nonDeltaVertices > 0) {
        avgBsdfPdf =
            data.sumBsdfPdf /
            static_cast<double>(data.nonDeltaVertices);

        avgLightPdf =
            data.sumLightPdf /
            static_cast<double>(data.nonDeltaVertices);
    }

    std::cout << "Min bsdf pdf:                " << data.minBsdfPdf << "\n";

    std::cout << "Max bsdf pdf:                " << data.maxBsdfPdf << "\n";

    std::cout << "Avg bsdf pdf:                " << avgBsdfPdf << "\n";

    std::cout << "Min light pdf:               " << data.minLightPdf << "\n";

    std::cout << "Max light pdf:               " << data.maxLightPdf << "\n";

    std::cout << "Avg light pdf:               " << avgLightPdf << "\n";

    std::cout << "Min cos theta:               " << data.minCosTheta << "\n";

    std::cout << "Max cos theta:               " << data.maxCosTheta << "\n";

    std::cout << "Avg cos theta:               " << avgCosTheta << "\n";

    std::cout << "Max throughput component:    " << data.maxThroughput << "\n";

    std::cout << "Avg throughput component:    " << avgThroughput << "\n";

    std::cout << "\nDepth counts:\n";

    for (int i = 0; i < GuidingDebugStats::maxTrackedDepth; ++i ) {
        if (data.depthCounts[i] == 0) {
            continue;
        }

        std::cout << "  depth " << i << ": " << data.depthCounts[i] << "\n";
    }

    std::cout << "===========================\n";
}