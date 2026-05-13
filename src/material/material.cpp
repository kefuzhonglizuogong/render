#include "material/material.h"

#include "core/random.h"
#include "core/frame.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr double PI = 3.14159265358979323846;

    Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - 2.0 * dot(v, n) * n;
    }
}

Color Material::emitted() const {
    return Color(0.0, 0.0, 0.0);
}

Lambertian::Lambertian(const Color& a)
    : albedo(a) {
}

Color Lambertian::eval(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;

    double cosTheta = std::max(
        0.0,
        dot(normal.normalized(), wi.normalized())
    );

    if (cosTheta <= 0.0) {
        return Color(0.0, 0.0, 0.0);
    }

    return albedo / PI;
}

double Lambertian::pdfValue(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;

    double cosTheta = std::max(
        0.0,
        dot(normal.normalized(), wi.normalized())
    );

    return cosTheta / PI;
}

BSDFSample Lambertian::sample(
    const Vec3& wo,
    const Vec3& normal
) const {
    BSDFSample result;

    Frame frame(normal);

    Vec3 localWi = randomCosineDirection();
    Vec3 wi = frame.toWorld(localWi).normalized();

    Color f = eval(wo, normal, wi);
    double pdf = pdfValue(wo, normal, wi);

    if (pdf <= 1e-12) {
        result.valid = false;
        return result;
    }

    result.wi = wi;
    result.f = f;
    result.pdf = pdf;
    result.type = BSDFSampleType::Diffuse;
    result.isDelta = false;
    result.valid = true;

    return result;
}

DiffuseLight::DiffuseLight(const Color& e)
    : emission(e) {
}

Color DiffuseLight::emitted() const {
    return emission;
}

Color DiffuseLight::eval(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;
    (void)normal;
    (void)wi;

    return Color(0.0, 0.0, 0.0);
}

double DiffuseLight::pdfValue(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;
    (void)normal;
    (void)wi;

    return 0.0;
}

BSDFSample DiffuseLight::sample(
    const Vec3& wo,
    const Vec3& normal
) const {
    (void)wo;
    (void)normal;

    BSDFSample result;
    result.valid = false;
    result.type = BSDFSampleType::None;
    result.isDelta = false;
    result.pdf = 0.0;
    result.f = Color(0.0, 0.0, 0.0);

    return result;
}

Mirror::Mirror(const Color& a)
    : albedo(a) {
}

Color Mirror::eval(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;
    (void)normal;
    (void)wi;

    // 理想镜面是 delta distribution，
    // 不能用普通连续 BRDF eval 表示。
    return Color(0.0, 0.0, 0.0);
}

double Mirror::pdfValue(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;
    (void)normal;
    (void)wi;

    // delta 材质没有普通 solid-angle pdf。
    return 0.0;
}

BSDFSample Mirror::sample(
    const Vec3& wo,
    const Vec3& normal
) const {
    BSDFSample result;

    Vec3 n = normal.normalized();

    // wo 是从交点指向上一段路径来源的方向。
    // 入射方向可以理解为 -wo。
    // 镜面反射方向就是 reflect(-wo, n)。
    Vec3 wi = reflect(-wo.normalized(), n).normalized();

    double cosTheta = dot(n, wi);

    if (cosTheta <= 0.0) {
        result.valid = false;
        return result;
    }

    result.wi = wi;

    // 对 delta sample，这里把 f 作为 throughput factor 使用。
    // renderer 里会对 isDelta 特殊处理，不再乘 cos / pdf。
    result.f = albedo;

    result.pdf = 1.0;
    result.type = BSDFSampleType::DeltaReflection;
    result.isDelta = true;
    result.valid = true;

    return result;
}