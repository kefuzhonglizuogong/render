#pragma once

#include <cmath>
#include "vec3.h"

class Frame {
public:
    Vec3 s;//切线方向,局部 x 轴
    Vec3 t;//副切线方向,局部 y 轴
    Vec3 n;//法线方向,局部 z 轴

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