#pragma once

#include "render/scene.h"
#include "render/camera.h"
#include "render/film.h"
#include "render/guiding_record.h"
#include "guiding/guiding_mode.h"

class Renderer {
public:
    int samplesPerPixel;//每个像素采样多少次
    int maxDepth;//最多允许反弹多少次

    bool enableGuidingRecord = false;
    bool enableGuidedSampling = false;

    double guidingProbability = 0.5;

    Renderer(int spp, int depth);

    void setEnableGuidingRecord(bool enabled);

    void setEnableGuidedSampling(bool enabled);

    void setGuidingProbability(double probability);

    void setGuidingMode(GuidingMode mode);

    Color trace(const Ray& ray, const Scene& scene, int depth) const;
    void render(const Scene& scene, const Camera& camera, Film& film) const;

    GuidingMode guidingMode = GuidingMode::Spatial;
};

void printGuidingDistributionSummary();
