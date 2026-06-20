#include "geometry/triangle.h"
#include "core/stats.h"

#include <algorithm>
#include <cmath>
#include <memory>

Triangle::Triangle(
    const Point3& a,
    const Point3& b,
    const Point3& c,
    const std::shared_ptr<Material>& m
) : v0(a), v1(b), v2(c), material(m) {
    normal = cross(v1 - v0, v2 - v0).normalized();
    n0 = normal;
    n1 = normal;
    n2 = normal;
}

Triangle::Triangle(
    const Point3& a,
    const Point3& b,
    const Point3& c,
    const Vec3& normal0,
    const Vec3& normal1,
    const Vec3& normal2,
    const std::shared_ptr<Material>& m
) : v0(a), v1(b), v2(c), material(m) {
    normal = cross(v1 - v0, v2 - v0).normalized();
    n0 = normal0.normalized();
    n1 = normal1.normalized();
    n2 = normal2.normalized();
    hasVertexNormals = true;
}

void Triangle::setUVs(
    const Vec2& a,
    const Vec2& b,
    const Vec2& c
) {
    uv0 = a;
    uv1 = b;
    uv2 = c;
    hasUV = true;
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
    double baryU = f * dot(s, h);

    if (baryU < 0.0 || baryU > 1.0) {
        return false;
    }

    Vec3 q = cross(s, edge1);
    double baryV = f * dot(ray.direction, q);

    if (baryV < 0.0 || baryU + baryV > 1.0) {
        return false;
    }

    double t = f * dot(edge2, q);

    if (t < tMin || t > tMax) {
        return false;
    }

    rec.t = t;
    rec.p = ray.at(t);
    rec.setFaceNormal(ray, normal);

    double baryW = 1.0 - baryU - baryV;
    Vec3 interpolatedNormal = (baryW * n0 + baryU * n1 + baryV * n2).normalized();

    if (interpolatedNormal.lengthSquared() <= 0.0) {
        interpolatedNormal = normal;
    }

    if (!rec.frontFace) {
        interpolatedNormal = -interpolatedNormal;
    }

    rec.shadingNormal = interpolatedNormal;
    rec.material = material;

    rec.hasUV = hasUV;
    if (hasUV) {
        rec.uv = uv0 * static_cast<float>(baryW) +
            uv1 * static_cast<float>(baryU) +
            uv2 * static_cast<float>(baryV);
    }

    const Lambertian* lambert = dynamic_cast<const Lambertian*>(material.get());
    if (lambert && lambert->texture && lambert->texture->isValid() && rec.hasUV) {
        rec.baseColor = lambert->texture->sample(rec.uv.x, rec.uv.y);
        rec.hasBaseColor = true;
    }

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
