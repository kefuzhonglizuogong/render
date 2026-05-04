#include "geometry/mesh.h"
#include "geometry/bvh.h"
#include "core/stats.h"

#include <iostream>

void Mesh::addTriangle(const Point3& v0,const Point3& v1,const Point3& v2,Material* material) {
    auto triangle = std::make_shared<Triangle>(
        v0,
        v1,
        v2,
        material
    );

    addTriangle(triangle);
}

void Mesh::addTriangle(const std::shared_ptr<Triangle>& triangle) {
    triangles.push_back(triangle);

    hasBVH = false;
    hasBox = false;
    bvhRoot = nullptr;
}

void Mesh::buildBVH() {
    if (triangles.empty()) {
        bvhRoot = nullptr;
        hasBVH = false;
        hasBox = false;
        return;
    }

    std::vector<std::shared_ptr<Hittable>> objects = triangles;

    bvhRoot = std::make_shared<BVHNode>(objects,0,objects.size());

    hasBVH = true;

    AABB tempBox;

    if (bvhRoot->boundingBox(tempBox)) {
        box = tempBox;
        hasBox = true;
    }
    else {
        std::cerr << "Mesh::buildBVH(): failed to compute mesh bounding box.\n";
        hasBox = false;
    }
}

bool Mesh::intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const {
    ++gStats.meshIntersectCalls;

    if (hasBVH && bvhRoot) {
        return bvhRoot->intersect(ray, tMin, tMax, rec);
    }

    HitRecord tempRec;
    bool hitAnything = false;
    double closest = tMax;

    for (const auto& triangle : triangles) {
        if (triangle->intersect(ray, tMin, closest, tempRec)) {
            hitAnything = true;
            closest = tempRec.t;
            rec = tempRec;
        }
    }

    return hitAnything;
}

bool Mesh::boundingBox(AABB& outputBox) const {
    if (hasBox) {
        outputBox = box;
        return true;
    }

    if (triangles.empty()) {
        return false;
    }

    AABB resultBox;
    bool firstBox = true;

    for (const auto& triangle : triangles) {
        AABB triBox;

        if (!triangle->boundingBox(triBox)) {
            return false;
        }

        if (firstBox) {
            resultBox = triBox;
            firstBox = false;
        }
        else {
            resultBox = surroundingBox(resultBox, triBox);
        }
    }

    outputBox = resultBox;
    return true;
}