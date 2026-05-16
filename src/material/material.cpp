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

    //计算方向 v 关于法线 n 的镜面反射方向。
    Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - 2.0 * dot(v, n) * n;
    }

    //判断两个方向 a、b 是否在法线 n 的同一侧半球。
    bool sameHemisphere(const Vec3& a,const Vec3& b,const Vec3& n) {
        return dot(a, n) * dot(b, n) > 0.0;
    }

    //计算 GGX 的法线分布函数 D(h)。表示微表面法线 h 出现的密度。
    double ggxD(const Vec3& n,const Vec3& h,double alpha) {
        double NoH = std::max(0.0, dot(n, h));
        double a2 = alpha * alpha;

        double denom =NoH * NoH * (a2 - 1.0) + 1.0;

        return a2 / (PI * denom * denom);
    }

    //从方向 v 观察时，有多少微表面没有被遮挡/掩蔽。
    double smithG1(const Vec3& n,const Vec3& v,double alpha) {
        double NoV = std::max(0.0, dot(n, v));

        if (NoV <= 0.0) {
            return 0.0;
        }

        double a2 = alpha * alpha;
        double NoV2 = NoV * NoV;

        return 2.0 * NoV /(NoV + std::sqrt(a2 + (1.0 - a2) * NoV2));
    }

    //把两边几何项合起来，得到双边几何项 G(wo, wi)。
    double smithG(const Vec3& n,const Vec3& wo,const Vec3& wi,double alpha) {
        return smithG1(n, wo, alpha) * smithG1(n, wi, alpha);
    }

    //入射角越掠射，反射越强。
    Color fresnelSchlick(double cosTheta,const Color& F0) {
        double x = clampDouble(1.0 - cosTheta, 0.0, 1.0);
        double x2 = x * x;
        double x5 = x2 * x2 * x;

        return F0 + (Color(1.0, 1.0, 1.0) - F0) * x5;
    }

    //旧版 GGX 采样方法。
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

    //GGX VNDF 采样方法
    Vec3 sampleGGXVNDF(const Vec3& wo,const Vec3& normal,double alpha) {
        // n 是当前表面的单位法线。
        // v 是当前交点指向上一跳 / 相机方向的单位向量。
        Vec3 n = normal.normalized();
        Vec3 v = wo.normalized();

        // 建立以 normal 为 z 轴的局部坐标系。
        // 在这个局部空间里，表面法线等价于 (0, 0, 1)。
        Frame frame(n);

        // 把观察方向 wo 从世界坐标转换到局部坐标。
        Vec3 V = frame.toLocal(v).normalized();

        // 根据 GGX 粗糙度 alpha 拉伸观察方向。
        // x/y 方向乘 alpha，z 方向保持不变。
        // 这是 VNDF 采样算法的核心步骤之一。
        Vec3 Vh = Vec3(alpha * V.x, alpha * V.y, V.z).normalized();

        // 计算 Vh 在 xy 平面投影长度的平方。
        // 用于判断能否稳定构造切线方向 T1。
        double lensq = Vh.x * Vh.x + Vh.y * Vh.y;

        Vec3 T1;

        // 如果 Vh 在 xy 平面上有足够长的投影，
        // 就构造一个和 Vh 投影垂直的切线方向。
        if (lensq > 1e-12) {
            T1 = Vec3(
                -Vh.y,
                Vh.x,
                0.0
            ) / std::sqrt(lensq);
        }
        // 如果 Vh 几乎正对 z 轴，投影长度接近 0，
        // 直接使用默认切线方向，避免除以 0。
        else {
            T1 = Vec3(1.0, 0.0, 0.0);
        }

        // 构造第二个切线方向。
        // T1、T2、Vh 共同组成围绕 Vh 的局部坐标系。
        Vec3 T2 = cross(Vh, T1);

        // 两个随机数，用于采样单位圆盘。
        double u1 = randomDouble();
        double u2 = randomDouble();

        // 在单位圆盘上做均匀面积采样。
        // r = sqrt(u1) 是为了保证面积均匀，而不是半径均匀。
        double r = std::sqrt(u1);
        double phi = 2.0 * PI * u2;

        // 圆盘上的二维采样点。
        double t1 = r * std::cos(phi);
        double t2 = r * std::sin(phi);

        // 根据观察方向 Vh.z 计算混合系数。
        // Vh 越接近法线方向，s 越接近 1。
        double s = 0.5 * (1.0 + Vh.z);

        // 根据可见法线分布对圆盘采样点进行变形。
        // 这一步让采样结果更偏向当前观察方向可见的微表面法线。
        t2 = (1.0 - s) * std::sqrt(
            std::max(0.0, 1.0 - t1 * t1)
        ) + s * t2;

        // 把变形后的圆盘点投影到以 Vh 为法线的半球上。
        // Nh 是拉伸空间中的法线方向。
        Vec3 Nh =
            t1 * T1 +
            t2 * T2 +
            std::sqrt(
                std::max(
                    0.0,
                    1.0 - t1 * t1 - t2 * t2
                )
            ) * Vh;

        // 从拉伸空间变换回真实 GGX 空间。
        // 得到局部坐标下的可见微表面法线 Ne。
        Vec3 Ne = Vec3(
            alpha * Nh.x,
            alpha * Nh.y,
            std::max(0.0, Nh.z)
        ).normalized();

        // 把局部坐标下的微表面法线转换回世界坐标并返回。
        // 返回值是微表面法线 m，不是最终反射方向 wi。
        return frame.toWorld(Ne).normalized();
    }

    //计算 VNDF 采样下 half-vector h 的 pdf。
    double ggxVisibleNormalPdf(const Vec3& n, const Vec3& wo, const Vec3& h, double alpha) {
        double NoV = std::max(0.0, dot(n, wo));
        double NoH = std::max(0.0, dot(n, h));
        double VoH = std::max(0.0, dot(wo, h));

        if (NoV <= 1e-12 || NoH <= 0.0 || VoH <= 1e-12) {
            return 0.0;
        }

        double D = ggxD(n, h, alpha);
        double G1 = smithG1(n, wo, alpha);


        //VNDF 概率 = 微法线本身的概率（D）× 这个微法线能被视线看见的概率（G1）× 视线与微法线的夹角修正（VoH）÷ 视线与宏观法线的夹角修正（NoV）
        return D * G1 * VoH / NoV;
    }

    //用来决定玻璃这次是反射还是折射。
    //cosTheta,   视线与表面法线的夹角余弦
    //etaI,       入射侧折射率（空气≈1.0）
    //etaT        物体折射率（玻璃≈1.5，金属≈复杂值）
    double schlickFresnel(double cosTheta,double etaI,double etaT) {
        double r0 =(etaI - etaT) /(etaI + etaT);

        r0 = r0 * r0;

        double x = clampDouble(1.0 - cosTheta,0.0,1.0);

        double x2 = x * x;
        double x5 = x2 * x2 * x;

        //最终反射率 =正对时的反光（r0）+ 斜角时增强的反光（(1 - r0) * (1 - cosθ) ^ 5）
        return r0 + (1.0 - r0) * x5;
    }

    /*
        根据 Snell 定律计算折射方向。
        
        true：成功折射
        false：发生全反射，折射失败

        这是玻璃材质的核心几何函数。
        如果全反射，就只能走反射分支。
    */
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
    double r,
    bool useVNDF
)
    : albedo(a),
    roughness(r),
    useVNDFSampling(useVNDF) {
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

    if (NoV <= 0.0 || NoL <= 0.0 || !sameHemisphere(v, l, n)) {
        return 0.0;
    }

    Vec3 h = (v + l).normalized();

    double VoH = std::max(0.0, dot(v, h));
    double NoH = std::max(0.0, dot(n, h));

    if (VoH <= 1e-12 || NoH <= 0.0) {
        return 0.0;
    }

    double alpha = std::max(0.001, roughness * roughness);

    double pdfH = 0.0;

    if (useVNDFSampling) {
        pdfH =
            ggxVisibleNormalPdf(
                n,
                v,
                h,
                alpha
            );
    }
    else {
        double D = ggxD(n, h, alpha);
        pdfH = D * NoH;
    }

    double pdfWi = pdfH / (4.0 * VoH);

    return pdfWi;
}

BSDFSample GGXMetal::sample(const Vec3& wo,const Vec3& normal) const {
    BSDFSample result;

    Vec3 n = normal.normalized();
    Vec3 v = wo.normalized();

    double NoV = std::max(0.0, dot(n, v));

    if (NoV <= 0.0) {
        result.valid = false;
        return result;
    }

    double alpha = std::max(0.001, roughness * roughness);

    Vec3 h;

    if (useVNDFSampling) {
        h =
            sampleGGXVNDF(
                v,
                n,
                alpha
            );
    }
    else {
        h = sampleGGXHalfVector(n, alpha);
    }

    double VoH =
        std::max(0.0, dot(v, h));

    if (VoH <= 1e-12) {
        result.valid = false;
        return result;
    }

    Vec3 wi = reflect(-v, h).normalized();

    double NoL = std::max(0.0, dot(n, wi));

    if (NoL <= 0.0 || !sameHemisphere(v, wi, n)) {
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
