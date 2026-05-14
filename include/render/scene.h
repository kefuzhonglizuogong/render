#pragma once

#include <vector>
#include <memory>

#include "geometry/hittable.h"
#include "light/light.h"
#include "light/environment_light.h"

struct LightSelectionSample {
    std::shared_ptr<Light> light;
    double selectionPdf = 0.0;
    bool valid = false;
};

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

    LightSelectionSample sampleLight() const;

    double lightPdfSum(
        const Point3& refPoint,
        const Vec3& wi
    ) const;

    void buildBVH();

    bool intersect(const Ray& ray,double tMin,double tMax,HitRecord& rec) const;

};
