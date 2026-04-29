#pragma once

#include "vec3.h"
#include "ray.h"

class Material;

struct HitRecord {
    Point3 p;
    Vec3 normal;
    double t;
    bool frontFace;
    Material* material;

    HitRecord()
        : p(), normal(), t(0.0), frontFace(true), material(nullptr) {
    }

    void setFaceNormal(const Ray& ray, const Vec3& outwardNormal) {
        frontFace = dot(ray.direction, outwardNormal) < 0.0;//如果点积小于 0，说明：射线方向和外法线方向夹角大于 90°射线是从物体外面打进来的
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};