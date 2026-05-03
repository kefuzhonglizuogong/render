#pragma once

#include <vector>
#include <memory>

#include "hittable.h"
#include "light.h"

class Scene {
public:
    std::vector<std::shared_ptr<Hittable>> objects;
    std::vector<std::shared_ptr<Light>> lights;

    std::shared_ptr<Hittable> bvhRoot;
    bool useBVH = false;

    void add(const std::shared_ptr<Hittable>& object);

    void addLight(const std::shared_ptr<Light>& light);

    void buildBVH();

    bool intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const;

};