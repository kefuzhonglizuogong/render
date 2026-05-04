#include "material/material.h"
#include "core/random.h"
#include "core/frame.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr double PI = 3.14159265358979323846;
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

    double cosTheta = std::max(0.0, dot(normal.normalized(), wi.normalized()));
    if (cosTheta <= 0.0) {
        return Color(0.0, 0.0, 0.0);
    }

    return albedo / PI;//albedo·´ÕÕÂÊ
}

double Lambertian::pdfValue(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    (void)wo;

    double cosTheta = std::max(0.0, dot(normal.normalized(), wi.normalized()));
    return cosTheta / PI;
}

bool Lambertian::sample(
    const Vec3& wo,
    const Vec3& normal,
    Vec3& wi,
    Color& f,
    double& pdf
) const {
    (void)wo;

    Frame frame(normal);
    Vec3 localWi = randomCosineDirection();
    wi = frame.toWorld(localWi).normalized();

    f = eval(wo, normal, wi);
    pdf = pdfValue(wo, normal, wi);

    return pdf > 0.0;
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

bool DiffuseLight::sample(
    const Vec3& wo,
    const Vec3& normal,
    Vec3& wi,
    Color& f,
    double& pdf
) const {
    (void)wo;
    (void)normal;
    (void)wi;
    f = Color(0.0, 0.0, 0.0);
    pdf = 0.0;
    return false;
}