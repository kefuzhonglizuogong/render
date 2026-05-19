#include "guiding/local_hemisphere_histogram.h"

#include "core/random.h"

#include <cmath>
#include <algorithm>

/*
----------------训练阶段----------------

路径追踪产生世界方向 vertex.wi
        ↓
用 vertex.shadingNormal 建 Frame
        ↓
world wi 转 local wi
        ↓
localWi.z > 0 才训练
        ↓
histogram.update(localWi, weight)

----------------采样阶段----------------

histogram.sample()
        ↓
得到 localWi
        ↓
用当前交点 rec.shadingNormal 建 Frame
        ↓
localWi 转 world wi
        ↓
用 world wi 继续追踪光线

----------------pdf 查询阶段----------------

无论 wi 来自 BSDF 还是 guiding
        ↓
只要要查 guiding pdf
        ↓
world wi 转 local wi
        ↓
histogram.pdf(localWi)
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

LocalHemisphereHistogram::LocalHemisphereHistogram(
    int tBins,
    int pBins
)
    : thetaBins(tBins),
    phiBins(pBins),
    binCount(tBins* pBins),
    weights(static_cast<size_t>(binCount), 0.0),
    cdf(static_cast<size_t>(binCount + 1), 0.0) {
}

void LocalHemisphereHistogram::reset() {
    std::fill(weights.begin(), weights.end(), 0.0);
    std::fill(cdf.begin(), cdf.end(), 0.0);

    totalWeight = 0.0;
    distributionBuilt = false;
}

int LocalHemisphereHistogram::directionToBin(
    const Vec3& localDirection
) const {
    Vec3 d = localDirection.normalized();

    if (d.z <= 0.0) {
        return -1;
    }

    double theta = std::acos(
        clampDouble(d.z, 0.0, 1.0)
    );

    double phi = std::atan2(d.y, d.x);

    if (phi < 0.0) {
        phi += 2.0 * PI;
    }

    double thetaNorm =
        theta / (0.5 * PI);

    double phiNorm =
        phi / (2.0 * PI);

    int thetaIndex =
        static_cast<int>(thetaNorm * thetaBins);

    int phiIndex =
        static_cast<int>(phiNorm * phiBins);

    thetaIndex = std::max(
        0,
        std::min(thetaBins - 1, thetaIndex)
    );

    phiIndex = std::max(
        0,
        std::min(phiBins - 1, phiIndex)
    );

    return thetaIndex * phiBins + phiIndex;
}

double LocalHemisphereHistogram::binSolidAngle(
    int thetaIndex
) const {
    double theta0 =
        0.5 * PI *
        static_cast<double>(thetaIndex) /
        static_cast<double>(thetaBins);

    double theta1 =
        0.5 * PI *
        static_cast<double>(thetaIndex + 1) /
        static_cast<double>(thetaBins);

    double dPhi =
        2.0 * PI /
        static_cast<double>(phiBins);

    double solidAngle =
        (std::cos(theta0) - std::cos(theta1)) *
        dPhi;

    return std::max(1e-12, solidAngle);
}

Vec3 LocalHemisphereHistogram::binToDirection(
    int bin,
    double u1,
    double u2
) const {
    int thetaIndex = bin / phiBins;
    int phiIndex = bin % phiBins;

    double theta0 =
        0.5 * PI *
        static_cast<double>(thetaIndex) /
        static_cast<double>(thetaBins);

    double theta1 =
        0.5 * PI *
        static_cast<double>(thetaIndex + 1) /
        static_cast<double>(thetaBins);

    double phi0 =
        2.0 * PI *
        static_cast<double>(phiIndex) /
        static_cast<double>(phiBins);

    double phi1 =
        2.0 * PI *
        static_cast<double>(phiIndex + 1) /
        static_cast<double>(phiBins);

    double cosTheta0 = std::cos(theta0);
    double cosTheta1 = std::cos(theta1);

    double cosTheta =
        (1.0 - u1) * cosTheta0 +
        u1 * cosTheta1;

    cosTheta =
        clampDouble(cosTheta, 0.0, 1.0);

    double phi =
        (1.0 - u2) * phi0 +
        u2 * phi1;

    double sinTheta =
        std::sqrt(
            std::max(
                0.0,
                1.0 - cosTheta * cosTheta
            )
        );

    double x = sinTheta * std::cos(phi);
    double y = sinTheta * std::sin(phi);
    double z = cosTheta;

    return Vec3(x, y, z).normalized();
}

void LocalHemisphereHistogram::update(
    const Vec3& localDirection,
    double weight
) {
    if (weight <= 0.0) {
        return;
    }

    if (std::isnan(weight) || std::isinf(weight)) {
        return;
    }

    int bin =
        directionToBin(localDirection);

    if (bin < 0) {
        return;
    }

    weights[static_cast<size_t>(bin)] += weight;
    distributionBuilt = false;
}

void LocalHemisphereHistogram::buildDistribution() {
    cdf[0] = 0.0;
    totalWeight = 0.0;

    for (int i = 0; i < binCount; ++i) {
        totalWeight +=
            std::max(
                0.0,
                weights[static_cast<size_t>(i)]
            );

        cdf[static_cast<size_t>(i + 1)] =
            totalWeight;
    }

    if (totalWeight > 0.0) {
        for (int i = 1; i <= binCount; ++i) {
            cdf[static_cast<size_t>(i)] /=
                totalWeight;
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

LocalGuidedSample LocalHemisphereHistogram::sample() const {
    LocalGuidedSample result;

    if (!distributionBuilt) {
        return result;
    }

    double u = randomDouble();

    auto it = std::upper_bound(
        cdf.begin(),
        cdf.end(),
        u
    );

    int bin =
        static_cast<int>(
            std::distance(cdf.begin(), it)
            ) - 1;

    bin = std::max(
        0,
        std::min(binCount - 1, bin)
    );

    Vec3 localWi =
        binToDirection(
            bin,
            randomDouble(),
            randomDouble()
        );

    result.localWi = localWi;
    result.pdf = pdf(localWi);
    result.valid = result.pdf > 1e-12;

    return result;
}

double LocalHemisphereHistogram::pdf(
    const Vec3& localDirection
) const {
    if (!distributionBuilt) {
        return 0.0;
    }

    int bin =
        directionToBin(localDirection);

    if (bin < 0) {
        return 0.0;
    }

    double pBin = 0.0;

    if (totalWeight > 0.0) {
        pBin =
            std::max(
                0.0,
                weights[static_cast<size_t>(bin)]
            ) / totalWeight;
    }
    else {
        pBin =
            1.0 /
            static_cast<double>(binCount);
    }

    int thetaIndex = bin / phiBins;

    double solidAngle =
        binSolidAngle(thetaIndex);

    return pBin / solidAngle;
}