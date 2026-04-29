#pragma once

#include <vector>
#include <memory>

#include "hittable.h"
#include "light.h"

class Scene {
public:
    std::vector<std::shared_ptr<Hittable>> objects;
    std::vector<std::shared_ptr<Light>> lights;

    void add(const std::shared_ptr<Hittable>& object) {
        objects.push_back(object);
    }

    void addLight(const std::shared_ptr<Light>& light) {
        lights.push_back(light);
    }

    bool intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const;

};