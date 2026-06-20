#pragma once

#include "core/vec3.h"
#include "core/ray.h"
#include "render/core/vec2.h"

#include <memory>

class Material;

struct HitRecord {
    Point3 p;
    Vec3 geometricNormal;
    Vec3 shadingNormal;
    Vec2 uv;
    Color baseColor;
    double t;
    bool hasUV;
    bool hasBaseColor;
    bool frontFace;
    std::shared_ptr<Material> material;

    HitRecord()
        : p(),
          geometricNormal(),
          shadingNormal(),
          uv(),
          baseColor(1.0, 1.0, 1.0),
          t(0.0),
          hasUV(false),
          hasBaseColor(false),
          frontFace(true),
          material(nullptr) {
    }

    void setFaceNormal(const Ray& ray, const Vec3& outwardNormal) {
        frontFace = dot(ray.direction, outwardNormal) < 0.0;//如果点积小于 0，说明：射线方向和外法线方向夹角大于 90°射线是从物体外面打进来的
        geometricNormal = frontFace ? outwardNormal : -outwardNormal;
    }
};
