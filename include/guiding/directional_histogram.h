#pragma once

#include "core/vec3.h"

#include <vector>

/*
这个类表示一个全局球面方向分布。
它先不考虑位置，也不考虑法线坐标系：
整个场景共用一个方向直方图
这不是最终 Path Guiding，但它是第一步。
*/

struct DirectionalSample {
    Vec3 wi;
    double pdf = 0.0;
    bool valid = false;
};

class DirectionalHistogram {
public:
    DirectionalHistogram(
        int thetaBins = 16,
        int phiBins = 32
    );

    void reset();

    void update(const Vec3& direction, double weight);

    void buildDistribution();

    DirectionalSample sample() const;

    double pdf(const Vec3& direction) const;

    int getThetaBins() const {
        return thetaBins;
    }

    int getPhiBins() const {
        return phiBins;
    }

    double getTotalWeight() const {
        return totalWeight;
    }

private:
    int thetaBins = 16;
    int phiBins = 32;
    int binCount = 512;

    std::vector<double> weights;// 每个格子的权重（光的强度）
    std::vector<double> cdf;// 累积分布（用来采样）

    double totalWeight = 0.0;
    bool distributionBuilt = false;

    int directionToBin(const Vec3& direction) const;

    Vec3 binToDirection(int bin,double u1,double u2) const;

    double binSolidAngle(int thetaIndex) const;
};