#pragma once

#include "core/vec3.h"

struct LightSample {
    Point3 position;
    Vec3 normal;
    Vec3 wi;
    double distance;
    double pdf;
    Color emission;
};

class Light {
public:
    virtual ~Light() = default;

    virtual bool sample(
        const Point3& refPoint,
        LightSample& sample
    ) const = 0;

    virtual double pdf(
        const Point3& refPoint,
        const Vec3& wi
    ) const = 0;
};

class SphereLight : public Light {
public:
    Point3 center;
    double radius;
    Color emit;

    SphereLight(
        const Point3& center,
        double radius,
        const Color& emit
    );

    bool sample(
        const Point3& refPoint,
        LightSample& sample
    ) const override;

    double pdf(
        const Point3& refPoint,
        const Vec3& wi
    ) const override;
};

class QuadLight : public Light {
public:
    Point3 corner;
    Vec3 edgeU;
    Vec3 edgeV;
    Vec3 normal;
    double area;
    Color emit;

    QuadLight(
        const Point3& corner,
        const Vec3& edgeU,
        const Vec3& edgeV,
        const Color& emit
    );

    bool sample(
        const Point3& refPoint,
        LightSample& sample
    ) const override;

    double pdf(
        const Point3& refPoint,
        const Vec3& wi
    ) const override;

private:
    bool isInside(double a, double b) const;
};