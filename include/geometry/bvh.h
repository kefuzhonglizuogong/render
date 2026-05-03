#pragma once

#include "hittable.h"
#include "core/aabb.h"

#include <memory>
#include <vector>

class BVHNode : public Hittable {
public:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB box;

    BVHNode() = default;

    BVHNode(
        std::vector<std::shared_ptr<Hittable>>& objects,
        size_t start,
        size_t end
    );

    bool intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const override;

    bool boundingBox(AABB& outputBox) const override;
};