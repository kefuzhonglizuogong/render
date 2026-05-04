#pragma once

#include <cmath>
#include <algorithm>

class Vec3 {
public:
    double x, y, z;

    Vec3() : x(0.0), y(0.0), z(0.0) {}
    Vec3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

    Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }

    Vec3 operator-(const Vec3& v) const {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }

    Vec3 operator*(double s) const {
        return Vec3(x * s, y * s, z * s);
    }

    Vec3 operator/(double s) const {
        return Vec3(x / s, y / s, z / s);
    }

    Vec3& operator+=(const Vec3& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    Vec3& operator*=(double s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    Vec3& operator/=(double s) {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    Vec3 operator-() const {
        return Vec3(-x, -y, -z);
    }

    double lengthSquared() const {
        return x * x + y * y + z * z;
    }

    double length() const {
        return std::sqrt(lengthSquared());
    }

    Vec3 normalized() const {
        double len = length();
        if (len <= 0.0) {
            return Vec3(0.0, 0.0, 0.0);
        }
        return *this / len;
    }
};

inline double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline Vec3 operator*(double s, const Vec3& v) {
    return Vec3(v.x * s, v.y * s, v.z * s);
}

inline Vec3 operator*(const Vec3& a, const Vec3& b) {
    return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline double maxComponent(const Vec3& v) {
    return std::max(v.x, std::max(v.y, v.z));
}

using Color = Vec3;
using Point3 = Vec3;