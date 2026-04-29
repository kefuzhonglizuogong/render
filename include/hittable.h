#pragma once

#include "ray.h"
#include "hit.h"

class Hittable {
public:
    virtual bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const = 0;
    virtual ~Hittable() = default;
};