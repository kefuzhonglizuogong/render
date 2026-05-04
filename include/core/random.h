#pragma once

#include <random>
#include <cmath>
#include "core/vec3.h"

inline double randomDouble() {
    static thread_local std::mt19937 generator(std::random_device{}());
    static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline double randomDouble(double min, double max) {
    return min + (max - min) * randomDouble();
}

inline Vec3 randomInUnitSphere() {
    while (true) {
        Vec3 p(
            randomDouble(-1.0, 1.0),
            randomDouble(-1.0, 1.0),
            randomDouble(-1.0, 1.0)
        );

        if (p.lengthSquared() >= 1.0) {
            continue;
        }
        return p;
    }
}

inline Vec3 randomUnitVector() {
    return randomInUnitSphere().normalized();
}

inline Vec3 randomCosineDirection() {
    constexpr double PI = 3.14159265358979323846;

    double r1 = randomDouble();
    double r2 = randomDouble();

    double phi = 2.0 * PI * r1;
    double x = std::cos(phi) * std::sqrt(r2);
    double y = std::sin(phi) * std::sqrt(r2);
    double z = std::sqrt(1.0 - r2);

    return Vec3(x, y, z);
}