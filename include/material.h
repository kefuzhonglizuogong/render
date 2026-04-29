#pragma once

#include "vec3.h"


//eval：如果方向 wo 和 wi 已经给定，这个 BRDF 的数值是多少？
//pdfValue：如果我用这套采样策略，要采到这个 wi，概率密度是多少？
//sample：按策略，真正采一个 wi 出来，同时返回对应的 f 和 pdf 。
class Material {
public:
    virtual ~Material() = default;

    virtual Color emitted() const;

    virtual Color eval(
        const Vec3& wo,
        const Vec3& normal,
        const Vec3& wi
    ) const = 0;

    virtual double pdfValue(
        const Vec3& wo,
        const Vec3& normal,
        const Vec3& wi
    ) const = 0;

    virtual bool sample(
        const Vec3& wo,
        const Vec3& normal,
        Vec3& wi,
        Color& f,
        double& pdf
    ) const = 0;
};

class Lambertian : public Material {
public:
    Color albedo;

    explicit Lambertian(const Color& a);

    Color eval(
        const Vec3& wo,
        const Vec3& normal,
        const Vec3& wi
    ) const override;

    double pdfValue(
        const Vec3& wo,
        const Vec3& normal,
        const Vec3& wi
    ) const override;

    bool sample(
        const Vec3& wo,
        const Vec3& normal,
        Vec3& wi,
        Color& f,
        double& pdf
    ) const override;
};

class DiffuseLight : public Material {
public:
    Color emission;

    explicit DiffuseLight(const Color& e);

    Color emitted() const override;

    Color eval(
        const Vec3& wo,
        const Vec3& normal,
        const Vec3& wi
    ) const override;

    double pdfValue(
        const Vec3& wo,
        const Vec3& normal,
        const Vec3& wi
    ) const override;

    bool sample(
        const Vec3& wo,
        const Vec3& normal,
        Vec3& wi,
        Color& f,
        double& pdf
    ) const override;
};