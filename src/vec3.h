#pragma once
#include <cmath>
#include <algorithm>

struct Vec3 {
    double x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

    Vec3 operator+(const Vec3& b) const { return Vec3(x + b.x, y + b.y, z + b.z); }
    Vec3 operator-(const Vec3& b) const { return Vec3(x - b.x, y - b.y, z - b.z); }
    Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(double s) const { return Vec3(x / s, y / s, z / s); }

    Vec3& operator+=(const Vec3& b) {
        x += b.x; y += b.y; z += b.z;
        return *this;
    }
};

inline double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

//归一化
inline Vec3 normalize(const Vec3& v) {
    double len = length(v);
    if (len == 0.0) return Vec3(0, 0, 0);
    return v / len;
}

//把颜色限制在 [0, 1]
inline Vec3 clamp01(const Vec3& c) {
    return Vec3(
        std::clamp(c.x, 0.0, 1.0),
        std::clamp(c.y, 0.0, 1.0),
        std::clamp(c.z, 0.0, 1.0)
    );
}
