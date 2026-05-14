#include "render/scene.h"
#include "geometry/bvh.h"
#include "core/random.h"
#include "core/stats.h"

void Scene::add(const std::shared_ptr<Hittable>& object) {
    objects.push_back(object);

    useBVH = false;
    bvhRoot = nullptr;
}

void Scene::addLight(const std::shared_ptr<Light>& light) {
    lights.push_back(light);
}

void Scene::setEnvironment(const std::shared_ptr<EnvironmentLight>& env) {
    environment = env;

    if (env) {
        lights.push_back(env);
    }
}

LightSelectionSample Scene::sampleLight() const {
    LightSelectionSample result;

    if (lights.empty()) {
        return result;
    }

    double totalWeight = 0.0;

    for (const auto& light : lights) {
        if (!light) {
            continue;
        }

        double w = light->selectionWeight();

        if (w > 0.0) {
            totalWeight += w;
        }
    }

    if (totalWeight <= 0.0) {
        int lightCount =
            static_cast<int>(lights.size());

        int index =
            static_cast<int>(
                randomDouble() * lightCount
            );

        if (index >= lightCount) {
            index = lightCount - 1;
        }

        result.light = lights[index];
        result.selectionPdf =
            1.0 / static_cast<double>(lightCount);
        result.valid = true;

        return result;
    }

    double target =
        randomDouble() * totalWeight;

    double accum = 0.0;

    for (const auto& light : lights) {
        if (!light) {
            continue;
        }

        double w = light->selectionWeight();

        if (w <= 0.0) {
            continue;
        }

        accum += w;

        if (target <= accum) {
            result.light = light;
            result.selectionPdf = w / totalWeight;
            result.valid = true;
            return result;
        }
    }

    for (auto it = lights.rbegin(); it != lights.rend(); ++it) {
        const auto& light = *it;

        if (!light) {
            continue;
        }

        double w = light->selectionWeight();

        if (w > 0.0) {
            result.light = light;
            result.selectionPdf = w / totalWeight;
            result.valid = true;
            return result;
        }
    }

    return result;
}

double Scene::lightPdfSum(
    const Point3& refPoint,
    const Vec3& wi
) const {
    if (lights.empty()) {
        return 0.0;
    }

    double totalWeight = 0.0;

    for (const auto& light : lights) {
        if (!light) {
            continue;
        }

        double w = light->selectionWeight();

        if (w > 0.0) {
            totalWeight += w;
        }
    }

    if (totalWeight <= 0.0) {
        double pdf = 0.0;

        double selectionPdf =
            1.0 / static_cast<double>(lights.size());

        for (const auto& light : lights) {
            if (!light) {
                continue;
            }

            pdf += selectionPdf *
                light->pdf(refPoint, wi);
        }

        return pdf;
    }

    double pdf = 0.0;

    for (const auto& light : lights) {
        if (!light) {
            continue;
        }

        double w = light->selectionWeight();

        if (w <= 0.0) {
            continue;
        }

        double selectionPdf = w / totalWeight;

        pdf += selectionPdf *
            light->pdf(refPoint, wi);
    }

    return pdf;
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
