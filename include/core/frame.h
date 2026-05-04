#pragma once

#include <cmath>
#include "core/vec3.h"

class Frame {
public:
    Vec3 s;
    Vec3 t;
    Vec3 n;

    Frame() : s(1.0, 0.0, 0.0), t(0.0, 1.0, 0.0), n(0.0, 0.0, 1.0) {}

    explicit Frame(const Vec3& normal) {
        n = normal.normalized();

        if (std::abs(n.z) < 0.999) {
            s = cross(Vec3(0.0, 0.0, 1.0), n).normalized();
        }
        else {
            s = cross(Vec3(0.0, 1.0, 0.0), n).normalized();
        }

        t = cross(n, s);
    }

    Vec3 toWorld(const Vec3& local) const {
        return local.x * s + local.y * t + local.z * n;
    }
};