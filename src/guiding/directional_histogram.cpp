#include "guiding/directional_histogram.h"

#include "core/random.h"

#include <cmath>
#include <algorithm>
#include <iostream>


/*
    DirectionalHistogram

    这是一个基于球面经纬度划分的方向直方图采样器。

    它把整个球面方向划分成 thetaBins * phiBins 个 bin。
    每个 bin 记录一个权重 weights[bin]。
    权重越大，说明该方向区域越重要，后续采样时越容易被选中。

    主要用途：
        用于 path guiding / directional guiding。
        记录历史采样中哪些方向贡献较大，
        然后根据这些权重构建概率分布，
        后续更倾向于采样高贡献方向。

    坐标约定：
        使用 y 轴作为竖直方向。
        theta = acos(y)，范围 [0, PI]。
        phi = atan2(z, x)，范围 [0, 2PI]。

    核心流程：

        update(direction, weight)
            把某个方向的贡献累加到对应 bin。

        buildDistribution()
            根据 weights 构建 CDF，用于离散采样 bin。

        sample()
            先根据 CDF 按权重选中一个 bin，
            再在该 bin 覆盖的球面区域内均匀采样一个方向。

        pdf(direction)
            返回某个方向在当前直方图分布下的概率密度。
            pdf = pBin / solidAngle。

    注意：
        pBin 是选中某个 bin 的概率。
        solidAngle 是这个 bin 对应的球面立体角。
        路径追踪中需要的是单位立体角上的 pdf，
        所以最终要除以 solidAngle。
*/
namespace {
    constexpr double PI = 3.14159265358979323846;

    double clampDouble(double x, double a, double b) {
        if (x < a) {
            return a;
        }

        if (x > b) {
            return b;
        }

        return x;
    }
}

DirectionalHistogram::DirectionalHistogram(int tBins, int pBins)
    : thetaBins(tBins),
    phiBins(pBins),
    binCount(tBins* pBins),
    weights(static_cast<size_t>(binCount), 0.0),
    cdf(static_cast<size_t>(binCount + 1), 0.0) {
}

void DirectionalHistogram::reset() {
    std::fill(weights.begin(), weights.end(), 0.0);

    std::fill(cdf.begin(), cdf.end(), 0.0);

    totalWeight = 0.0;
    distributionBuilt = false;
}

int DirectionalHistogram::directionToBin(const Vec3& direction) const {
    Vec3 d = direction.normalized();

    double theta = std::acos(clampDouble(d.y, -1.0, 1.0));

    double phi = std::atan2(d.z, d.x);

    if (phi < 0.0) {
        phi += 2.0 * PI;
    }

    double thetaNorm = theta / PI;
    double phiNorm = phi / (2.0 * PI);

    int thetaIndex = static_cast<int>(thetaNorm * thetaBins);

    int phiIndex = static_cast<int>(phiNorm * phiBins);

    thetaIndex = std::max(0, std::min(thetaBins - 1, thetaIndex)
    );

    phiIndex = std::max( 0, std::min(phiBins - 1, phiIndex));

    return thetaIndex * phiBins + phiIndex;
}

//计算某一个 theta 行对应 bin 的立体角大小
double DirectionalHistogram::binSolidAngle(int thetaIndex) const {
    double theta0 =
        PI * static_cast<double>(thetaIndex) /
        static_cast<double>(thetaBins);

    double theta1 =
        PI * static_cast<double>(thetaIndex + 1) /
        static_cast<double>(thetaBins);

    double dPhi = 2.0 * PI / static_cast<double>(phiBins);

    // solid angle of lat-long bin:
    // ∫ sin(theta) dtheta dphi = (cos(theta0) - cos(theta1)) * dPhi
    double solidAngle = (std::cos(theta0) - std::cos(theta1)) * dPhi;

    return std::max(1e-12, solidAngle);
}

//给定一个 bin，再在这个 bin 覆盖的球面区域内部随机采样一个方向
Vec3 DirectionalHistogram::binToDirection(int bin, double u1, double u2) const {
    int thetaIndex = bin / phiBins;
    int phiIndex = bin % phiBins;

    double theta0 =
        PI * static_cast<double>(thetaIndex) /
        static_cast<double>(thetaBins);

    double theta1 =
        PI * static_cast<double>(thetaIndex + 1) /
        static_cast<double>(thetaBins);

    double phi0 =
        2.0 * PI * static_cast<double>(phiIndex) /
        static_cast<double>(phiBins);

    double phi1 =
        2.0 * PI * static_cast<double>(phiIndex + 1) /
        static_cast<double>(phiBins);

    // 在俯仰角区间内实现立体角均匀采样
    // 应当对余弦值 cos(theta) 做线性采样
    double cosTheta0 = std::cos(theta0);
    double cosTheta1 = std::cos(theta1);

    double cosTheta = (1.0 - u1) * cosTheta0 + u1 * cosTheta1;

    cosTheta = clampDouble(cosTheta, -1.0, 1.0);

    double theta = std::acos(cosTheta);

    double phi = (1.0 - u2) * phi0 + u2 *  phi1;

    //球坐标转三维方向
    double sinTheta = std::sqrt( std::max(0.0, 1.0 - cosTheta * cosTheta) );

    double x = sinTheta * std::cos(phi);
    double y = cosTheta;
    double z = sinTheta * std::sin(phi);

    return Vec3(x, y, z).normalized();
}

//用于给某个方向增加权重
void DirectionalHistogram::update(const Vec3& direction, double weight) {
    if (weight <= 0.0) {
        return;
    }

    if (std::isnan(weight) || std::isinf(weight)) {
        return;
    }

    int bin = directionToBin(direction);

    weights[static_cast<size_t>(bin)] += weight;
    distributionBuilt = false;
}

//根据当前 weights 构建 CDF
void DirectionalHistogram::buildDistribution() {
    cdf[0] = 0.0;
    totalWeight = 0.0;

    for (int i = 0; i < binCount; ++i) {
        totalWeight +=std::max( 0.0, weights[static_cast<size_t>(i)]);

        cdf[static_cast<size_t>(i + 1)] = totalWeight;
    }

    if (totalWeight > 0.0) {
        for (int i = 1; i <= binCount; ++i) {
            cdf[static_cast<size_t>(i)] /= totalWeight;
        }
    }
    else {
        for (int i = 1; i <= binCount; ++i) {
            cdf[static_cast<size_t>(i)] =
                static_cast<double>(i) /
                static_cast<double>(binCount);
        }
    }

    distributionBuilt = true;
}

DirectionalSample DirectionalHistogram::sample() const {
    DirectionalSample result;

    if (!distributionBuilt) {
        return result;
    }

    double u = randomDouble();

    auto it = std::upper_bound(cdf.begin(), cdf.end(), u);

    int bin = static_cast<int>(std::distance(cdf.begin(), it)) - 1;

    bin = std::max(0, std::min(binCount - 1, bin));

    double u1 = randomDouble();
    double u2 = randomDouble();

    Vec3 wi = binToDirection(bin, u1, u2);

    result.wi = wi;
    result.pdf = pdf(wi);
    result.valid = result.pdf > 1e-12;

    return result;
}

//返回某个方向在当前直方图分布下的 pdf
double DirectionalHistogram::pdf(const Vec3& direction) const {
    if (!distributionBuilt) {
        return 0.0;
    }

    int bin = directionToBin(direction);

    double pBin = 0.0;

    if (totalWeight > 0.0) {
        pBin = std::max(0.0, weights[static_cast<size_t>(bin)]) / totalWeight;
    }
    else {
        pBin = 1.0 / static_cast<double>(binCount);
    }

    int thetaIndex = bin / phiBins;

    double solidAngle = binSolidAngle(thetaIndex);

    return pBin / solidAngle;
}