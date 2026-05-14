#pragma once

#include <vector>
#include <memory>

#include "geometry/hittable.h"
#include "light/light.h"
#include "light/environment_light.h"

class Scene {
public:
    std::vector<std::shared_ptr<Hittable>> objects;
    std::vector<std::shared_ptr<Light>> lights;
    
    std::shared_ptr<EnvironmentLight> environment;

    std::shared_ptr<Hittable> bvhRoot;
    bool useBVH = false;

    void add(const std::shared_ptr<Hittable>& object);

    void addLight(const std::shared_ptr<Light>& light);

    void setEnvironment(const std::shared_ptr<EnvironmentLight>& env);

    void buildBVH();

    bool intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const;

};