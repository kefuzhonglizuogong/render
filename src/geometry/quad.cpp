#include "geometry/quad.h"

#include <cmath>
#include <algorithm>
#include "core/stats.h"

Quad::Quad(const Point3& c, const Vec3& u, const Vec3& v, Material* m) : corner(c), edgeU(u), edgeV(v), material(m) {
    normal = cross(edgeU, edgeV).normalized();
}

bool Quad::isInside(double a, double b) const {
    return a >= 0.0 && a <= 1.0 && b >= 0.0 && b <= 1.0;
}

bool Quad::intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {
    ++gStats.quadIntersectCalls;

    double denom = dot(normal, ray.direction);

    if (std::fabs(denom) < 1e-8) {
        return false;
    }

    double t = dot(corner - ray.origin, normal) / denom;

    if (t < tMin || t > tMax) {
        return false;
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
        return false;
    }

    double a = (uv * wv - vv * wu) / determinant;
    double b = (uv * wu - uu * wv) / determinant;

    if (!isInside(a, b)) {
        return false;
    }

    rec.t = t;
    rec.p = p;
    rec.normal = normal;

    if (dot(ray.direction, rec.normal) > 0.0) {
        rec.normal = -rec.normal;
    }

    rec.material = material;

    return true;
}

bool Quad::boundingBox(AABB& outputBox) const {
    Point3 p0 = corner;
    Point3 p1 = corner + edgeU;
    Point3 p2 = corner + edgeV;
    Point3 p3 = corner + edgeU + edgeV;

    double minX = std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x));
    double minY = std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y));
    double minZ = std::min(std::min(p0.z, p1.z), std::min(p2.z, p3.z));

    double maxX = std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x));
    double maxY = std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y));
    double maxZ = std::max(std::max(p0.z, p1.z), std::max(p2.z, p3.z));

    const double padding = 1e-4;

    outputBox = AABB(
        Point3(minX - padding, minY - padding, minZ - padding),
        Point3(maxX + padding, maxY + padding, maxZ + padding)
    );

    return true;
}