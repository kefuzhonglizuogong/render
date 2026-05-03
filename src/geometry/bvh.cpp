#include "geometry/bvh.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include "core/stats.h"

namespace {
    bool boxCompare(const std::shared_ptr<Hittable>& a,const std::shared_ptr<Hittable>& b,int axis) {
        AABB boxA;
        AABB boxB;

        if (!a->boundingBox(boxA) || !b->boundingBox(boxB)) {
            std::cerr << "BVHNode: missing bounding box.\n";
            return false;
        }

        if (axis == 0) {
            return boxA.minimum.x < boxB.minimum.x;
        }
        else if (axis == 1) {
            return boxA.minimum.y < boxB.minimum.y;
        }
        else {
            return boxA.minimum.z < boxB.minimum.z;
        }
    }

    bool boxXCompare(
        const std::shared_ptr<Hittable>& a,
        const std::shared_ptr<Hittable>& b
    ) {
        return boxCompare(a, b, 0);
    }

    bool boxYCompare(
        const std::shared_ptr<Hittable>& a,
        const std::shared_ptr<Hittable>& b
    ) {
        return boxCompare(a, b, 1);
    }

    bool boxZCompare(
        const std::shared_ptr<Hittable>& a,
        const std::shared_ptr<Hittable>& b
    ) {
        return boxCompare(a, b, 2);
    }
}

BVHNode::BVHNode(std::vector<std::shared_ptr<Hittable>>& objects,size_t start,size_t end) {
    int axis = std::rand() % 3;

    auto comparator =axis == 0 ? boxXCompare :axis == 1 ? boxYCompare :boxZCompare;

    size_t objectCount = end - start;

    if (objectCount == 1) {
        left = right = objects[start];
    }
    else if (objectCount == 2) {
        if (comparator(objects[start], objects[start + 1])) {
            left = objects[start];
            right = objects[start + 1];
        }
        else {
            left = objects[start + 1];
            right = objects[start];
        }
    }
    else {
        std::sort(objects.begin() + start,objects.begin() + end,comparator);

        size_t mid = start + objectCount / 2;

        left = std::make_shared<BVHNode>(objects,start,mid);

        right = std::make_shared<BVHNode>(objects,mid,end);
    }

    AABB boxLeft;
    AABB boxRight;

    if (!left->boundingBox(boxLeft) || !right->boundingBox(boxRight)) {
        std::cerr << "BVHNode: missing bounding box in constructor.\n";
    }

    box = surroundingBox(boxLeft, boxRight);
}

bool BVHNode::intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const {
    //BVH 节点求交函数被调用了多少次。
    ++gStats.bvhNodeIntersectCalls;

    if (!box.hit(ray, tMin, tMax)) {
        return false;
    }

    HitRecord leftRec;
    HitRecord rightRec;

    bool hitLeft = left->intersect(
        ray,
        tMin,
        tMax,
        leftRec
    );

    bool hitRight = right->intersect(
        ray,
        tMin,
        hitLeft ? leftRec.t : tMax,
        rightRec
    );

    if (hitRight) {
        rec = rightRec;
        return true;
    }

    if (hitLeft) {
        rec = leftRec;
        return true;
    }

    return false;
}

bool BVHNode::boundingBox(AABB& outputBox) const {
    outputBox = box;
    return true;
}