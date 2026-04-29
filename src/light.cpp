#include "light.h"
#include "random.h"
#include "ray.h"

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