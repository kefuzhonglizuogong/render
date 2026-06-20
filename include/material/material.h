#pragma once

#include "core/vec3.h"
#include "material/bsdf_sample.h"
#include "render/texture/texture.h"

#include <memory>

/*
eval()：这个方向的 BRDF 值是多少
pdfValue()：采到这个方向的概率是多少
sample()：怎么按重要性采样方向
*/
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
    std::shared_ptr<Texture> texture = nullptr;

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
    bool useVNDFSampling;

    GGXMetal(
        const Color& albedo,
        double roughness,
        bool useVNDFSampling = true
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

class Dielectric : public Material {
public:
    Color albedo;
    double ior;//ior = index of refraction，折射率
    /*
    air     1.0
    water   1.33
    glass   1.5
    diamond 2.4
    */

    Dielectric(
        const Color& albedo,
        double ior
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
