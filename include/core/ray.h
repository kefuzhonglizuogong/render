#pragma once

#include "core/vec3.h"

class Ray {
public:
    Point3 origin;
    Vec3 direction;

    Ray() : origin(), direction(0.0, 0.0, 1.0) {}

    Ray(const Point3& o, const Vec3& d)
        : origin(o), direction(d) {
    }

    Point3 at(double t) const {
        return origin + direction * t;
    }
};