#pragma once

#include "hittable.h"
#include "vec3.h"
#include "material.h"

class Quad : public Hittable {
public:

    /*
    corner：矩形的一个角点
    edgeU ：从 corner 出发的一条边
    edgeV ：从 corner 出发的另一条边
    */
    Point3 corner;
    Vec3 edgeU;
    Vec3 edgeV;
    Vec3 normal;
    Material* material;

    Quad(
        const Point3& corner,
        const Vec3& edgeU,
        const Vec3& edgeV,
        Material* material
    );

    bool intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const override;

    bool boundingBox(AABB& outputBox) const override;

private:
    bool isInside(double a, double b) const;
};