#pragma once

#include "geometry/hittable.h"
#include "material/material.h"

#include <memory>

class Sphere : public Hittable {
public:
    Point3 center;
    double radius;
    std::shared_ptr<Material> material;

    Sphere(
        const Point3& c,
        double r,
        const std::shared_ptr<Material>& m
    )
        : center(c), radius(r), material(m) {
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override;

    bool boundingBox(AABB& outputBox) const override;
};
