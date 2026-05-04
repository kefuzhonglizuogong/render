#pragma once

#include "geometry/hittable.h"
#include "material/material.h"

class Plane : public Hittable {
public:
    Point3 point;
    Vec3 normal;
    Material* material;

    Plane(const Point3& p, const Vec3& n, Material* m)
        : point(p), normal(n.normalized()), material(m) {
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override;
};