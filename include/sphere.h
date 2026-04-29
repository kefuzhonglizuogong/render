#pragma once

#include "hittable.h"
#include "material.h"

class Sphere : public Hittable {
public:
    Point3 center;
    double radius;
    Material* material;

    Sphere(const Point3& c, double r, Material* m)
        : center(c), radius(r), material(m) {
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override;
};