#pragma once

#include "light/light.h"
#include "image/float_image.h"

#include <vector>

class EnvironmentLight : public Light {
public:
    virtual ~EnvironmentLight() = default;

    virtual Color eval(const Vec3& direction) const = 0;
};

class ConstantEnvironmentLight : public EnvironmentLight {
public:
    Color radiance;

    explicit ConstantEnvironmentLight(const Color& radiance);

    Color eval(const Vec3& direction) const override;

    bool sample(const Point3& refPoint,LightSample& sample) const override;

    double pdf(const Point3& refPoint,const Vec3& wi) const override;
};

class LatLongEnvironmentLight : public EnvironmentLight {
public:
    FloatImage image;

    std::vector<double> weights;
    std::vector<double> marginalCdf;
    std::vector<double> conditionalCdf;

    double totalWeight = 0.0;

    explicit LatLongEnvironmentLight(const FloatImage& image);

    Color eval(const Vec3& direction) const override;

    bool sample(const Point3& refPoint,LightSample& sample) const override;

    double pdf(const Point3& refPoint,const Vec3& wi) const override;

private:
    void buildDistribution();

    int sampleDiscrete(const std::vector<double>& cdf,double u) const;

    double texelPdfSolidAngle(int x,int y) const;
};