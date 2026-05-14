#include "material/material.h"

#include "core/random.h"
#include "core/frame.h"

#include <algorithm>
#include <cmath>

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

    Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - 2.0 * dot(v, n) * n;
    }

    double ggxD(const Vec3& n,const Vec3& h,double alpha) {
        double NoH = std::max(0.0, dot(n, h));
        double a2 = alpha * alpha;

        double denom =
            NoH * NoH * (a2 - 1.0) + 1.0;

        return a2 / (PI * denom * denom);
    }

    double smithG1(const Vec3& n,const Vec3& v,double alpha) {
        double NoV = std::max(0.0, dot(n, v));

        if (NoV <= 0.0) {
            return 0.0;
        }

        double a2 = alpha * alpha;
        double NoV2 = NoV * NoV;

        return 2.0 * NoV /
            (NoV + std::sqrt(a2 + (1.0 - a2) * NoV2));
    }

    double smithG(const Vec3& n,const Vec3& wo,const Vec3& wi,double alpha) {
        return smithG1(n, wo, alpha) *
            smithG1(n, wi, alpha);
    }

    Color fresnelSchlick(double cosTheta,const Color& F0) {
        double x = clampDouble(1.0 - cosTheta, 0.0, 1.0);
        double x2 = x * x;
        double x5 = x2 * x2 * x;

        return F0 + (Color(1.0, 1.0, 1.0) - F0) * x5;
    }

    Vec3 sampleGGXHalfVector(const Vec3& normal,double alpha) {
        double u1 = randomDouble();
        double u2 = randomDouble();

        double a2 = alpha * alpha;

        double phi = 2.0 * PI * u1;

        double cosTheta =
            std::sqrt(
                (1.0 - u2) /
                (1.0 + (a2 - 1.0) * u2)
            );

        double sinTheta =std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));

        Vec3 localH(std::cos(phi) * sinTheta,std::sin(phi) * sinTheta,cosTheta);

        Frame frame(normal);

        return frame.toWorld(localH).normalized();
    }
    double schlickFresnel(double cosTheta,double etaI,double etaT) {
        double r0 =(etaI - etaT) /(etaI + etaT);

        r0 = r0 * r0;

        double x = clampDouble(1.0 - cosTheta,0.0,1.0);

        double x2 = x * x;
        double x5 = x2 * x2 * x;

        return r0 + (1.0 - r0) * x5;
    }

    bool refract(const Vec3& incident,const Vec3& normal,double etaRatio,Vec3& refracted) {
        Vec3 i = incident.normalized();
        Vec3 n = normal.normalized();

        double cosTheta =std::min(dot(-i, n),1.0);

        Vec3 rOutPerp =etaRatio * (i + cosTheta * n);

        double k =1.0 - rOutPerp.lengthSquared();

        if (k < 0.0) {
            return false;
        }

        Vec3 rOutParallel =-std::sqrt(k) * n;

        refracted =(rOutPerp + rOutParallel).normalized();

        return true;
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

GGXMetal::GGXMetal(
    const Color& a,
    double r
)
    : albedo(a),
    roughness(r) {
}

Color GGXMetal::eval(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    Vec3 n = normal.normalized();
    Vec3 v = wo.normalized();
    Vec3 l = wi.normalized();

    double NoV = std::max(0.0, dot(n, v));
    double NoL = std::max(0.0, dot(n, l));

    if (NoV <= 0.0 || NoL <= 0.0) {
        return Color(0.0, 0.0, 0.0);
    }

    Vec3 h = (v + l).normalized();

    double VoH = std::max(0.0, dot(v, h));

    double alpha = std::max(0.001, roughness * roughness);

    double D = ggxD(n, h, alpha);
    double G = smithG(n, v, l, alpha);
    Color F = fresnelSchlick(VoH, albedo);

    double denom = 4.0 * NoV * NoL;

    if (denom <= 1e-12) {
        return Color(0.0, 0.0, 0.0);
    }

    return F * (D * G / denom);
}

double GGXMetal::pdfValue(
    const Vec3& wo,
    const Vec3& normal,
    const Vec3& wi
) const {
    Vec3 n = normal.normalized();
    Vec3 v = wo.normalized();
    Vec3 l = wi.normalized();

    double NoV = std::max(0.0, dot(n, v));
    double NoL = std::max(0.0, dot(n, l));

    if (NoV <= 0.0 || NoL <= 0.0) {
        return 0.0;
    }

    Vec3 h = (v + l).normalized();

    double VoH = std::max(0.0, dot(v, h));
    double NoH = std::max(0.0, dot(n, h));

    if (VoH <= 1e-12 || NoH <= 0.0) {
        return 0.0;
    }

    double alpha = std::max(0.001, roughness * roughness);

    double D = ggxD(n, h, alpha);

    // 先采样 half-vector h：
    // pdf_h = D(h) * cosTheta_h
    //
    // 再由 h 映射到反射方向 wi：
    // pdf_wi = pdf_h / (4 * dot(wo, h))
    double pdfH = D * NoH;
    double pdfWi = pdfH / (4.0 * VoH);

    return pdfWi;
}

BSDFSample GGXMetal::sample(
    const Vec3& wo,
    const Vec3& normal
) const {
    BSDFSample result;

    Vec3 n = normal.normalized();
    Vec3 v = wo.normalized();

    double NoV = std::max(0.0, dot(n, v));

    if (NoV <= 0.0) {
        result.valid = false;
        return result;
    }

    double alpha = std::max(0.001, roughness * roughness);

    Vec3 h = sampleGGXHalfVector(n, alpha);

    if (dot(v, h) <= 0.0) {
        result.valid = false;
        return result;
    }

    Vec3 wi = reflect(-v, h).normalized();

    double NoL = std::max(0.0, dot(n, wi));

    if (NoL <= 0.0) {
        result.valid = false;
        return result;
    }

    Color f = eval(v, n, wi);
    double pdf = pdfValue(v, n, wi);

    if (pdf <= 1e-12) {
        result.valid = false;
        return result;
    }

    result.wi = wi;
    result.f = f;
    result.pdf = pdf;
    result.type = BSDFSampleType::Glossy;
    result.isDelta = false;
    result.valid = true;

    return result;
}

Dielectric::Dielectric(const Color& a,double indexOfRefraction): albedo(a),ior(indexOfRefraction) {
}

Color Dielectric::eval(const Vec3& wo,const Vec3& normal,const Vec3& wi) const {
    (void)wo;
    (void)normal;
    (void)wi;

    // 理想玻璃是 delta BSDF：
    // 反射和折射都不是普通连续 BRDF。
    return Color(0.0, 0.0, 0.0);
}

double Dielectric::pdfValue(const Vec3& wo,const Vec3& normal,const Vec3& wi) const {
    (void)wo;
    (void)normal;
    (void)wi;

    // delta distribution 没有普通 solid-angle pdf。
    return 0.0;
}

BSDFSample Dielectric::sample(const Vec3& wo,const Vec3& normal) const {
    BSDFSample result;

    Vec3 n = normal.normalized();
    Vec3 out = wo.normalized();

    // 当前 ray 的入射方向，也就是路径继续前进前的方向
    Vec3 incident = -out;

    double cosTheta = std::min(dot(out, n),1.0);

    cosTheta = std::max(0.0, cosTheta);

    // 因为 HitRecord 里的 shadingNormal 已经被翻到朝向 ray 来源，
    // 所以 dot(wo, n) 通常为正。
    //
    // 这个判断用于兼容未来没有翻转 normal 的情况。
    bool entering = dot(out, n) > 0.0;

    double etaI = entering ? 1.0 : ior;
    double etaT = entering ? ior : 1.0;

    double etaRatio = etaI / etaT;

    double reflectProbability = schlickFresnel(cosTheta,etaI,etaT);

    Vec3 refractedDirection;
    bool canRefract = refract(incident,n,etaRatio,refractedDirection);

    bool chooseReflection =!canRefract ||randomDouble() < reflectProbability;

    if (chooseReflection) {
        Vec3 reflected =
            reflect(
                incident,
                n
            ).normalized();

        result.wi = reflected;
        result.f = albedo;
        result.pdf = 1.0;
        result.type = BSDFSampleType::DeltaReflection;
        result.isDelta = true;
        result.valid = true;

        return result;
    }

    result.wi = refractedDirection;
    result.f = albedo;
    result.pdf = 1.0;
    result.type = BSDFSampleType::DeltaTransmission;
    result.isDelta = true;
    result.valid = true;

    return result;
}