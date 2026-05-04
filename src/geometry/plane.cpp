#include "geometry/plane.h"
#include <cmath>

bool Plane::intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {
    const double eps = 1e-8;
    double denom = dot(normal, ray.direction);

    if (std::abs(denom) < eps) {
        return false;
    }

    double t = dot(point - ray.origin, normal) / denom;
    if (t < tMin || t > tMax) {
        return false;
    }

    rec.t = t;
    rec.p = ray.at(t);
    rec.setFaceNormal(ray, normal);
    rec.material = material;

    return true;
}