#pragma once

#include "core/vec3.h"
#include "material/bsdf_sample.h"

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

    virtual BSDFSample sample(
        const Vec3& wo,
        const Vec3& normal
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

    BSDFSample sample(
        const Vec3& wo,
        const Vec3& normal
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

    BSDFSample sample(
        const Vec3& wo,
        const Vec3& normal
    ) const override;
};

class Mirror : public Material {
public:
    Color albedo;

    explicit Mirror(const Color& a);

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

    BSDFSample sample(
        const Vec3& wo,
        const Vec3& normal
    ) const override;
};

class GGXMetal : public Material {
public:
    Color albedo;
    double roughness;

    GGXMetal(
        const Color& albedo,
        double roughness
    );

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

    BSDFSample sample(
        const Vec3& wo,
        const Vec3& normal
    ) const override;
};