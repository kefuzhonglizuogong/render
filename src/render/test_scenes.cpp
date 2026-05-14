#include "render/test_scenes.h"

#include "geometry/quad.h"
#include "geometry/sphere.h"
#include "geometry/mesh.h"
#include "io/obj_loader.h"
#include "material/material.h"

#include <memory>

void buildMaterialTestScene(Scene& scene) {
    // =====================================================
    // Materials
    // =====================================================

    static Lambertian groundMat(Color(0.75, 0.75, 0.75));

    static Lambertian redLambert(Color(0.75, 0.20, 0.18));

    static Mirror mirrorMat(Color(0.95, 0.95, 0.95));

    static GGXMetal goldMetal(Color(1.0, 0.72, 0.25),0.25);

    static Dielectric glassMat(Color(1.0, 1.0, 1.0),1.5);

    static Lambertian bunnyMat(Color(0.85, 0.75, 0.65));

    // =====================================================
    // Ground
    // =====================================================

    scene.add(std::make_shared<Quad>(
        Point3(-3.0, -0.5, -1.0),
        Vec3(6.0, 0.0, 0.0),
        Vec3(0.0, 0.0, -5.0),
        &groundMat
    ));

    // =====================================================
    // Lambert sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(-1.5, -0.15, -2.4),
        0.35,
        &redLambert
    ));

    // =====================================================
    // Mirror sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(-0.5, -0.15, -2.4),
        0.35,
        &mirrorMat
    ));

    // =====================================================
    // GGX metal sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(0.5, -0.15, -2.4),
        0.35,
        &goldMetal
    ));

    // =====================================================
    // Glass sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(1.5, -0.15, -2.4),
        0.35,
        &glassMat
    ));

    // =====================================================
    // OBJ mesh
    // =====================================================
    // 这里先写成你当前项目的 bunny 路径。
    // 如果你想完全通过 RenderConfig 管理路径，
    // 下一步我们再把 config 传进这个函数。

    auto bunny = loadOBJ(
        "D:/Program/Project/mini_renderer/models/bunny.obj",
        &bunnyMat,
        0.75,
        Point3(0.0, 0.0, -4.0)
    );

    scene.add(bunny);
}