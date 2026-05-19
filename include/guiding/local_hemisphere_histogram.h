#pragma once

#include "core/vec3.h"

#include <vector>

struct LocalGuidedSample {
    Vec3 localWi;
    double pdf = 0.0;
    bool valid = false;
};

class LocalHemisphereHistogram {
public:
    LocalHemisphereHistogram(
        int thetaBins = 8,
        int phiBins = 32
    );

    void reset();

    void update(const Vec3& localDirection, double weight);

    void buildDistribution();

    LocalGuidedSample sample() const;

    double pdf(const Vec3& localDirection) const;

    double getTotalWeight() const {
        return totalWeight;
    }

private:
    int thetaBins = 8;
    int phiBins = 32;
    int binCount = 256;

    std::vector<double> weights;
    std::vector<double> cdf;

    double totalWeight = 0.0;
    bool distributionBuilt = false;

    int directionToBin(const Vec3& localDirection) const;

    Vec3 binToDirection(int bin, double u1, double u2) const;

    double binSolidAngle(int thetaIndex) const;
};