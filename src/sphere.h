#pragma once
#include "ray.h"

//命中记录
struct Hit {
    bool hit = false;   //是否命中
    double t = 1e30;    //命中参数，也就是沿着射线走多远打到了
    Vec3 p;             //交点位置
    Vec3 n;             //交点法线
};

struct Sphere {
    Vec3 center;
    double radius;

    Sphere(const Vec3& c, double r) : center(c), radius(r) {}

    //射线和球相交测试
    bool intersect(const Ray& ray, Hit& rec) const {
        Vec3 oc = ray.o - center;
        double a = dot(ray.d, ray.d);
        double b = 2.0 * dot(oc, ray.d);
        double c = dot(oc, oc) - radius * radius;
        double disc = b * b - 4.0 * a * c;

        if (disc < 0.0) return false;

        double sqrtDisc = std::sqrt(disc);
        double t0 = (-b - sqrtDisc) / (2.0 * a);
        double t1 = (-b + sqrtDisc) / (2.0 * a);

        double t = t0;
        if (t < 1e-4) t = t1;
        if (t < 1e-4) return false;

        if (t < rec.t) {
            rec.hit = true;
            rec.t = t;
            rec.p = ray.o + ray.d * t;
            rec.n = normalize(rec.p - center);
            return true;
        }

        return false;
    }
};