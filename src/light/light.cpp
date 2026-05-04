#include "light/light.h"
#include "core/random.h"
#include "core/ray.h"

#include <cmath>
#include <algorithm>

namespace {
    constexpr double PI = 3.14159265358979323846;
}

SphereLight::SphereLight(
    const Point3& c,
    double r,
    const Color& e
)
    : center(c), radius(r), emit(e) {
}

//在球光源表面采样一个点，并返回方向 pdf
bool SphereLight::sample(const Point3& refPoint, LightSample& sample) const {
    Vec3 localNormal = randomUnitVector();

    Point3 lightPoint = center + localNormal * radius;
    Vec3 lightNormal = localNormal.normalized();

    Vec3 toLight = lightPoint - refPoint;
    double distanceSquared = toLight.lengthSquared();
    double distance = std::sqrt(distanceSquared);

    if (distance <= 1e-8) {
        return false;
    }

    Vec3 wi = toLight / distance;

    double cosLight = std::max(0.0, dot(lightNormal, -wi));

    if (cosLight <= 1e-8) {
        return false;
    }

    double area = 4.0 * PI * radius * radius;
    double pdfArea = 1.0 / area;

    double pdfSolidAngle = pdfArea * distanceSquared / cosLight;

    if (pdfSolidAngle <= 1e-12) {
        return false;
    }

    sample.position = lightPoint;
    sample.normal = lightNormal;
    sample.wi = wi;
    sample.distance = distance;
    sample.pdf = pdfSolidAngle;
    sample.emission = emit;

    return true;
}

//给定一个方向，计算这个方向在球光源采样策略下的 pdf
double SphereLight::pdf(const Point3& refPoint,const Vec3& wi) const {
    Ray ray(refPoint, wi.normalized());

    Vec3 oc = ray.origin - center;

    double a = ray.direction.lengthSquared();
    double halfB = dot(oc, ray.direction);
    double c = oc.lengthSquared() - radius * radius;

    double discriminant = halfB * halfB - a * c;

    if (discriminant < 0.0) {
        return 0.0;
    }

    double sqrtD = std::sqrt(discriminant);

    double root = (-halfB - sqrtD) / a;
    if (root <= 1e-4) {
        root = (-halfB + sqrtD) / a;
        if (root <= 1e-4) {
            return 0.0;
        }
    }

    Point3 lightPoint = ray.at(root);
    Vec3 lightNormal = (lightPoint - center) / radius;

    double distanceSquared = (lightPoint - refPoint).lengthSquared();

    double cosLight = std::max(0.0, dot(lightNormal.normalized(), -ray.direction.normalized()));

    if (cosLight <= 1e-8) {
        return 0.0;
    }

    double area = 4.0 * PI * radius * radius;
    double pdfArea = 1.0 / area;

    return pdfArea * distanceSquared / cosLight;
}

QuadLight::QuadLight(
    const Point3& c,
    const Vec3& u,
    const Vec3& v,
    const Color& e
)
    : corner(c), edgeU(u), edgeV(v), emit(e) {
    Vec3 n = cross(edgeU, edgeV);
    area = n.length();
    normal = n.normalized();
}

bool QuadLight::isInside(double a, double b) const {
    return a >= 0.0 && a <= 1.0 && b >= 0.0 && b <= 1.0;
}

bool QuadLight::sample(const Point3& refPoint,LightSample& sample) const {
    if (area <= 1e-12) {
        return false;
    }

    double a = randomDouble();
    double b = randomDouble();

    Point3 lightPoint = corner + a * edgeU + b * edgeV;

    Vec3 toLight = lightPoint - refPoint;
    double distanceSquared = toLight.lengthSquared();
    double distance = std::sqrt(distanceSquared);

    if (distance <= 1e-8) {
        return false;
    }

    Vec3 wi = toLight / distance;

    double cosLight = std::max(
        0.0,
        dot(normal, -wi)
    );

    if (cosLight <= 1e-8) {
        return false;
    }

    double pdfArea = 1.0 / area;
    double pdfSolidAngle = pdfArea * distanceSquared / cosLight;

    if (pdfSolidAngle <= 1e-12) {
        return false;
    }

    sample.position = lightPoint;
    sample.normal = normal;
    sample.wi = wi;
    sample.distance = distance;
    sample.pdf = pdfSolidAngle;
    sample.emission = emit;

    return true;
}

double QuadLight::pdf(const Point3& refPoint,const Vec3& wi) const {
    if (area <= 1e-12) {
        return 0.0;
    }

    Ray ray(refPoint, wi.normalized());

    double denom = dot(normal, ray.direction);

    if (std::fabs(denom) < 1e-8) {
        return 0.0;
    }

    double t = dot(corner - ray.origin, normal) / denom;

    if (t <= 1e-4) {
        return 0.0;
    }

    Point3 p = ray.at(t);
    Vec3 planarHit = p - corner;

    double uu = dot(edgeU, edgeU);
    double uv = dot(edgeU, edgeV);
    double vv = dot(edgeV, edgeV);

    double wu = dot(planarHit, edgeU);
    double wv = dot(planarHit, edgeV);

    double determinant = uv * uv - uu * vv;

    if (std::fabs(determinant) < 1e-12) {
        return 0.0;
    }

    double a = (uv * wv - vv * wu) / determinant;
    double b = (uv * wu - uu * wv) / determinant;

    if (!isInside(a, b)) {
        return 0.0;
    }

    double distanceSquared = (p - refPoint).lengthSquared();

    double cosLight = std::max(
        0.0,
        dot(normal, -ray.direction.normalized())
    );

    if (cosLight <= 1e-8) {
        return 0.0;
    }

    double pdfArea = 1.0 / area;
    return pdfArea * distanceSquared / cosLight;
}