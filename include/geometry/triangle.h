#pragma once

#include "geometry/hittable.h"
#include "core/vec3.h"
#include "material/material.h"

class Triangle : public Hittable {
public:
    Point3 v0;
    Point3 v1;
    Point3 v2;

    Vec3 normal;

    Material* material;

    Triangle(const Point3& v0,const Point3& v1,const Point3& v2,Material* material);

    bool intersect(const Ray& ray,double tMin, double tMax,HitRecord& rec) const override;

    bool boundingBox(AABB& outputBox) const override;
};