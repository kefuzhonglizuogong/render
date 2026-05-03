#include "sphere.h"
#include <cmath>
#include "core/stats.h"

bool Sphere::intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {

    ++gStats.sphereIntersectCalls;

    Vec3 oc = ray.origin - center;

    double a = ray.direction.lengthSquared();
    double halfB = dot(oc, ray.direction);
    double c = oc.lengthSquared() - radius * radius;

    double discriminant = halfB * halfB - a * c;
    if (discriminant < 0.0) {
        return false;
    }

    double sqrtD = std::sqrt(discriminant);

    double root = (-halfB - sqrtD) / a;
    if (root < tMin || root > tMax) {
        root = (-halfB + sqrtD) / a;
        if (root < tMin || root > tMax) {
            return false;
        }
    }

    rec.t = root;
    rec.p = ray.at(rec.t);

    Vec3 outwardNormal = (rec.p - center) / radius;
    rec.setFaceNormal(ray, outwardNormal);

    rec.material = material;
    return true;
}

bool Sphere::boundingBox(AABB& outputBox) const {
    Vec3 r(radius, radius, radius);

    outputBox = AABB(
        center - r,
        center + r
    );

    return true;
}