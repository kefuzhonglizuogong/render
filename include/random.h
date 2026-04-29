#pragma once

#include <random>
#include <cmath>
#include "vec3.h"

inline double randomDouble() {
    static thread_local std::mt19937 generator(std::random_device{}());
    static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline double randomDouble(double min, double max) {
    return min + (max - min) * randomDouble();
}

//这是拒绝采样：
//先在立方体[-1, 1] ^ 3 里随机扔点
//如果落在单位球外，就丢弃
//落在单位球内，就接受
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

//表示在随机方向上取一个单位向量。
inline Vec3 randomUnitVector() {
    return randomInUnitSphere().normalized();
}

//返回局部空间里的方向
inline Vec3 randomCosineDirection() {
    constexpr double PI = 3.14159265358979323846;

    double r1 = randomDouble();
    double r2 = randomDouble();

    double phi = 2.0 * PI * r1;//在 [0, 2π) 里均匀随机
    double x = std::cos(phi) * std::sqrt(r2);
    double y = std::sin(phi) * std::sqrt(r2);
    double z = std::sqrt(1.0 - r2);

    return Vec3(x, y, z);
}