#include "geometry/triangle.h"
#include "core/stats.h"

#include <cmath>

Triangle::Triangle(const Point3& a, const Point3& b, const Point3& c, Material* m): v0(a), v1(b), v2(c), material(m) {
    normal = cross(v1 - v0, v2 - v0).normalized();
}

bool Triangle::intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {
    ++gStats.triangleIntersectCalls;

    const double EPSILON = 1e-8;

    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;

    Vec3 h = cross(ray.direction, edge2);
    double a = dot(edge1, h);

    if (std::fabs(a) < EPSILON) {
        return false;
    }

    double f = 1.0 / a;

    Vec3 s = ray.origin - v0;
    double u = f * dot(s, h);

    if (u < 0.0 || u > 1.0) {
        return false;
    }

    Vec3 q = cross(s, edge1);
    double v = f * dot(ray.direction, q);

    if (v < 0.0 || u + v > 1.0) {
        return false;
    }

    double t = f * dot(edge2, q);

    if (t < tMin || t > tMax) {
        return false;
    }

    rec.t = t;
    rec.p = ray.at(t);
    rec.normal = normal;

    if (dot(ray.direction, rec.normal) > 0.0) {
        rec.normal = -rec.normal;
    }

    rec.material = material;

    return true;
}

bool Triangle::boundingBox(AABB& outputBox) const {
    double minX = std::min(v0.x, std::min(v1.x, v2.x));
    double minY = std::min(v0.y, std::min(v1.y, v2.y));
    double minZ = std::min(v0.z, std::min(v1.z, v2.z));

    double maxX = std::max(v0.x, std::max(v1.x, v2.x));
    double maxY = std::max(v0.y, std::max(v1.y, v2.y));
    double maxZ = std::max(v0.z, std::max(v1.z, v2.z));

    const double padding = 1e-4;

    outputBox = AABB(
        Point3(minX - padding, minY - padding, minZ - padding),
        Point3(maxX + padding, maxY + padding, maxZ + padding)
    );

    return true;
}