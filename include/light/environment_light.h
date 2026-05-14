#pragma once

#include "light/light.h"

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