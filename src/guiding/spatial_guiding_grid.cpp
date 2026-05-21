#include "guiding/spatial_guiding_grid.h"

#include <algorithm>
#include <cmath>

void SpatialGuidingCell::reset() {
    histogram.reset();
    sampleCount = 0;
}

void SpatialGuidingCell::build() {
    histogram.buildDistribution();
}

bool SpatialGuidingCell::isReady(int minSamples) const {
    return sampleCount >= minSamples && histogram.getTotalWeight() > 0.0;
}

//初始化整个空间网格
//newBounds   整个 guiding grid 覆盖的场景包围盒
//newNx       x 方向切多少格
//newNy       y 方向切多少格
//newNz       z 方向切多少格
void SpatialGuidingGrid::reset(const AABB& newBounds, int newNx, int newNy, int newNz) {
    bounds = newBounds;
    nx = newNx;
    ny = newNy;
    nz = newNz;

    if (nx <= 0 || ny <= 0 || nz <= 0) {
        nx = 0;
        ny = 0;
        nz = 0;
        cells.clear();
        return;
    }

    const int cellCount = nx * ny * nz;

    if (cellCount <= 0) {
        nx = 0;
        ny = 0;
        nz = 0;
        cells.clear();
        return;
    }

    cells.clear();
    cells.resize(static_cast<std::size_t>(cellCount));

    for (auto& cell : cells) {
        cell.reset();
    }
}

bool SpatialGuidingGrid::valid() const {
    return nx > 0 && ny > 0 && nz > 0 && !cells.empty();
}

int SpatialGuidingGrid::cellIndex(const Point3& p) const {
    if (!valid()) {
        return -1;
    }

    const Point3 bmin = bounds.minimum;
    const Point3 bmax = bounds.maximum;
    const Vec3 extent = bmax - bmin;

    if (extent.x <= 0.0 || extent.y <= 0.0 || extent.z <= 0.0) {
        return -1;
    }

    const double fx = (p.x - bmin.x) / extent.x;
    const double fy = (p.y - bmin.y) / extent.y;
    const double fz = (p.z - bmin.z) / extent.z;

    if (fx < 0.0 || fx > 1.0 || fy < 0.0 || fy > 1.0 || fz < 0.0 || fz > 1.0) {
        return -1;
    }

    int ix = static_cast<int>(fx * static_cast<double>(nx));
    int iy = static_cast<int>(fy * static_cast<double>(ny));
    int iz = static_cast<int>(fz * static_cast<double>(nz));

    ix = std::clamp(ix, 0, nx - 1);
    iy = std::clamp(iy, 0, ny - 1);
    iz = std::clamp(iz, 0, nz - 1);

    return flatten(ix, iy, iz);
}

//根据世界空间点 p，找到对应的 cell 指针
SpatialGuidingCell* SpatialGuidingGrid::cellAt(const Point3& p) {
    const int index = cellIndex(p);

    if (index < 0) {
        return nullptr;
    }

    return &cells[static_cast<std::size_t>(index)];
}

const SpatialGuidingCell* SpatialGuidingGrid::cellAt(const Point3& p) const {
    const int index = cellIndex(p);

    if (index < 0) {
        return nullptr;
    }

    return &cells[static_cast<std::size_t>(index)];
}

//训练入口：在位置 p 所属的空间 cell 中，记录一个局部半球方向 localWi 和对应训练权重 weight
void SpatialGuidingGrid::recordLocalDirection(const Point3& p, const Vec3& localWi, double weight) {
    SpatialGuidingCell* cell = cellAt(p);

    if (!cell) {
        return;
    }

    if (localWi.z <= 0.0) {
        return;
    }

    if (weight <= 0.0 || std::isnan(weight) || std::isinf(weight)) {
        return;
    }

    Vec3 wi = localWi.normalized();

    if (wi.z <= 0.0) {
        return;
    }


    //训练
    cell->histogram.update(wi, weight);
    ++cell->sampleCount;
}

void SpatialGuidingGrid::build() {
    for (auto& cell : cells) {
        if (cell.sampleCount > 0) {
            cell.build();
        }
    }
}

LocalGuidedSample SpatialGuidingGrid::sampleLocal(const Point3& p, int minSamples) const {
    const SpatialGuidingCell* cell = cellAt(p);

    if (!cell || !cell->isReady(minSamples)) {
        LocalGuidedSample invalidSample;
        invalidSample.localWi = Vec3(0.0, 0.0, 1.0);
        invalidSample.pdf = 0.0;
        invalidSample.valid = false;
        return invalidSample;
    }

    return cell->histogram.sample();
}

double SpatialGuidingGrid::pdfLocal(const Point3& p, const Vec3& localWi, int minSamples) const {
    const SpatialGuidingCell* cell = cellAt(p);

    if (!cell || !cell->isReady(minSamples)) {
        return 0.0;
    }

    if (localWi.z <= 0.0) {
        return 0.0;
    }

    Vec3 wi = localWi.normalized();

    if (wi.z <= 0.0) {
        return 0.0;
    }

    return cell->histogram.pdf(wi);
}

int SpatialGuidingGrid::totalSamples() const {
    int total = 0;

    for (const auto& cell : cells) {
        total += cell.sampleCount;
    }

    return total;
}

int SpatialGuidingGrid::activeCells() const {
    int count = 0;

    for (const auto& cell : cells) {
        if (cell.sampleCount > 0) {
            ++count;
        }
    }

    return count;
}

int SpatialGuidingGrid::flatten(int ix, int iy, int iz) const {
    return ix + nx * (iy + ny * iz);
}