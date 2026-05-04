#pragma once

#include "render/scene.h"
#include "render/camera.h"
#include "render/film.h"

class Renderer {
public:
    int samplesPerPixel;//每个像素采样多少次
    int maxDepth;//最多允许反弹多少次

    Renderer(int spp, int depth);

    Color trace(const Ray& ray, const Scene& scene, int depth) const;
    void render(const Scene& scene, const Camera& camera, Film& film) const;
};