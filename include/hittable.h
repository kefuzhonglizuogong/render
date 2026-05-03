#pragma once

#include "ray.h"
#include "hit.h"
#include "vec3.h"
#include "core/aabb.h"

class Material;

class Hittable {
public:
    virtual bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const = 0;
    virtual ~Hittable() = default;

    virtual bool boundingBox(AABB& outputBox) const {
        return false;
    }
};