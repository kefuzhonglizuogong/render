#pragma once

#include "ray.h"
#include "vec3.h"
#include "core/stats.h"

#include <algorithm>

class AABB {
public:
    Point3 minimum;
    Point3 maximum;

    AABB() = default;

    AABB(const Point3& a, const Point3& b)
        : minimum(a), maximum(b) {
    }

    bool hit(const Ray& ray,double tMin,double tMax) const {
        //每调用一次 AABB::hit()，AABB 测试次数加 1
        ++gStats.aabbHitCalls;

        for (int axis = 0; axis < 3; ++axis) {
            double origin;
            double direction;
            double minVal;
            double maxVal;

            if (axis == 0) {
                origin = ray.origin.x;
                direction = ray.direction.x;
                minVal = minimum.x;
                maxVal = maximum.x;
            }
            else if (axis == 1) {
                origin = ray.origin.y;
                direction = ray.direction.y;
                minVal = minimum.y;
                maxVal = maximum.y;
            }
            else {
                origin = ray.origin.z;
                direction = ray.direction.z;
                minVal = minimum.z;
                maxVal = maximum.z;
            }

            double invD = 1.0 / direction;
            double t0 = (minVal - origin) * invD;
            double t1 = (maxVal - origin) * invD;

            if (invD < 0.0) {
                std::swap(t0, t1);
            }

            tMin = std::max(t0, tMin);
            tMax = std::min(t1, tMax);

            if (tMax <= tMin) {
                return false;
            }
        }

        return true;
    }
};

inline AABB surroundingBox(const AABB& box0,const AABB& box1) {
    Point3 small(
        std::min(box0.minimum.x, box1.minimum.x),
        std::min(box0.minimum.y, box1.minimum.y),
        std::min(box0.minimum.z, box1.minimum.z)
    );

    Point3 big(
        std::max(box0.maximum.x, box1.maximum.x),
        std::max(box0.maximum.y, box1.maximum.y),
        std::max(box0.maximum.z, box1.maximum.z)
    );

    return AABB(small, big);
}