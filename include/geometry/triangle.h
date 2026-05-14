#pragma once

#include "geometry/hittable.h"
#include "core/vec3.h"
#include "material/material.h"

#include <memory>

class Triangle : public Hittable {
public:
    Point3 v0;
    Point3 v1;
    Point3 v2;

    Vec3 normal;
    Vec3 n0;
    Vec3 n1;
    Vec3 n2;
    bool hasVertexNormals = false;

    std::shared_ptr<Material> material;

    Triangle(
        const Point3& v0,
        const Point3& v1,
        const Point3& v2,
        const std::shared_ptr<Material>& material
    );

    Triangle(
        const Point3& v0,
        const Point3& v1,
        const Point3& v2,
        const Vec3& n0,
        const Vec3& n1,
        const Vec3& n2,
        const std::shared_ptr<Material>& material
    );

    bool intersect(const Ray& ray,double tMin, double tMax,HitRecord& rec) const override;

    bool boundingBox(AABB& outputBox) const override;
};
