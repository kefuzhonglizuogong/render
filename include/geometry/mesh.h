#pragma once

#include "geometry/hittable.h"
#include "geometry/triangle.h"

#include <memory>
#include <vector>

class Mesh : public Hittable {
public:
    std::vector<std::shared_ptr<Hittable>> triangles;

    std::shared_ptr<Hittable> bvhRoot;
    AABB box;
    bool hasBVH = false;
    bool hasBox = false;

    Mesh() = default;

    void addTriangle(
        const Point3& v0,
        const Point3& v1,
        const Point3& v2,
        Material* material
    );

    void addTriangle(
        const std::shared_ptr<Triangle>& triangle
    );

    void buildBVH();

    bool intersect(
        const Ray& ray,
        double tMin,
        double tMax,
        HitRecord& rec
    ) const override;

    bool boundingBox(AABB& outputBox) const override;
};