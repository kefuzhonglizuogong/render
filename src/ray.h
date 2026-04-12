#pragma once
#include "vec3.h"

struct Ray {
    Vec3 o;//表示射线起点，o 是 origin
    Vec3 d;//表示射线方向，d 是 direction

    Ray() = default;
    Ray(const Vec3& origin, const Vec3& dir) : o(origin), d(dir) {}
};
