#include "scene.h"

bool Scene::intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {
    HitRecord tempRec;//临时存每个物体的交点
    bool hitAnything = false;//有没有打中任何物体
    double closestSoFar = tMax;//当前已知最近交点距离

    for (const auto& obj : objects) {
        if (obj->intersect(ray, tMin, closestSoFar, tempRec)) {
            hitAnything = true;
            closestSoFar = tempRec.t;
            rec = tempRec;
        }
    }

    return hitAnything;
}