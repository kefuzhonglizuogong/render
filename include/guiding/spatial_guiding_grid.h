#pragma once

#include "core/aabb.h"
#include "core/vec3.h"
#include "guiding/local_hemisphere_histogram.h"

#include <vector>

struct SpatialGuidingCell {
    LocalHemisphereHistogram histogram;//这个空间格子的局部半球方向分布
    int sampleCount = 0;//这个格子收集过多少训练样本

    void reset();
    void build();
    bool isReady(int minSamples) const;
};

class SpatialGuidingGrid {
public:
    SpatialGuidingGrid() = default;

    void reset(const AABB& bounds, int nx, int ny, int nz);
    bool valid() const;

    int cellIndex(const Point3& p) const;

    SpatialGuidingCell* cellAt(const Point3& p);
    const SpatialGuidingCell* cellAt(const Point3& p) const;

    void recordLocalDirection(const Point3& p, const Vec3& localWi, double weight);
    void build();

    LocalGuidedSample sampleLocal(const Point3& p, int minSamples = 16) const;
    double pdfLocal(const Point3& p, const Vec3& localWi, int minSamples = 16) const;

    int totalSamples() const;
    int activeCells() const;

private:
    AABB bounds;
    int nx = 0;
    int ny = 0;
    int nz = 0;

    std::vector<SpatialGuidingCell> cells;

    int flatten(int ix, int iy, int iz) const;
};