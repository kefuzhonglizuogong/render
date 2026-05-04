#include "render/scene.h"
#include "geometry/bvh.h"
#include "core/stats.h"

void Scene::add(const std::shared_ptr<Hittable>& object) {
    objects.push_back(object);

    useBVH = false;
    bvhRoot = nullptr;
}

void Scene::addLight(const std::shared_ptr<Light>& light) {
    lights.push_back(light);
}

void Scene::buildBVH() {
    if (objects.empty()) {
        bvhRoot = nullptr;
        useBVH = false;
        return;
    }

    std::vector<std::shared_ptr<Hittable>> bvhObjects;

    for (const auto& object : objects) {
        AABB box;

        if (object->boundingBox(box)) {
            bvhObjects.push_back(object);
        }
    }

    if (bvhObjects.empty()) {
        bvhRoot = nullptr;
        useBVH = false;
        return;
    }

    bvhRoot = std::make_shared<BVHNode>(bvhObjects, 0, bvhObjects.size());
    useBVH = true;
}

bool Scene::intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {
    ++gStats.sceneIntersectCalls;

    if (useBVH && bvhRoot) {
        return bvhRoot->intersect(ray, tMin, tMax, rec);
    }

    HitRecord tempRec;
    bool hitAnything = false;
    double closest = tMax;

    for (const auto& object : objects) {
        if (object->intersect(ray, tMin, closest, tempRec)) {
            hitAnything = true;
            closest = tempRec.t;
            rec = tempRec;
        }
    }

    return hitAnything;
}