#pragma once

#include "vec3.h"

class Ray {
public:
    Point3 origin;//射线起点
    Vec3 direction;//射线方向

    Ray() : origin(), direction(0.0, 0.0, 1.0) {}

    Ray(const Point3& o, const Vec3& d)
        : origin(o), direction(d) {
    }

    //在这条射线上，离起点距离参数为 t 的点在哪里。
    Point3 at(double t) const {
        return origin + direction * t;
    }
};