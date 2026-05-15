#include "render/test_scenes.h"

#include "geometry/mesh.h"
#include "geometry/quad.h"
#include "geometry/sphere.h"
#include "io/obj_loader.h"
#include "material/material.h"

#include <memory>

void buildMaterialTestScene(Scene& scene) {
    // =====================================================
    // Materials
    // =====================================================

    auto groundMat = std::make_shared<Lambertian>(Color(0.75, 0.75, 0.75));
    auto redLambert = std::make_shared<Lambertian>(Color(0.75, 0.20, 0.18));
    auto mirrorMat = std::make_shared<Mirror>(Color(0.95, 0.95, 0.95));
    auto goldMetal = std::make_shared<GGXMetal>(Color(1.0, 0.72, 0.25), 0.25);
    auto glassMat = std::make_shared<Dielectric>(Color(1.0, 1.0, 1.0), 1.5);
    auto bunnyMat = std::make_shared<Lambertian>(Color(0.85, 0.75, 0.65));

    // =====================================================
    // Ground
    // =====================================================

    scene.add(std::make_shared<Quad>(
        Point3(-3.0, -0.5, -1.0),
        Vec3(6.0, 0.0, 0.0),
        Vec3(0.0, 0.0, -5.0),
        groundMat
    ));

    // =====================================================
    // Lambert sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(-1.5, -0.15, -2.4),
        0.35,
        redLambert
    ));

    // =====================================================
    // Mirror sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(-0.5, -0.15, -2.4),
        0.35,
        mirrorMat
    ));

    // =====================================================
    // GGX metal sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(0.5, -0.15, -2.4),
        0.35,
        goldMetal
    ));

    // =====================================================
    // Glass sphere
    // =====================================================

    scene.add(std::make_shared<Sphere>(
        Point3(1.5, -0.15, -2.4),
        0.35,
        glassMat
    ));

    // =====================================================
    // OBJ mesh
    // =====================================================

    auto bunny = loadOBJ(
        "D:/Program/Project/mini_renderer/models/bunny.obj",
        bunnyMat,
        0.75,
        Point3(0.0, 0.0, -4.0)
    );

    scene.add(bunny);


    Color lightEmission(5.0, 5.0, 5.0);

    auto lightMat =
        std::make_shared<DiffuseLight>(
            lightEmission
        );

    Point3 lightCorner(-0.5, 1.4, -2.8);
    Vec3 lightU(1.0, 0.0, 0.0);
    Vec3 lightV(0.0, 0.0, 1.0);

    scene.add(std::make_shared<Quad>(
        lightCorner,
        lightU,
        lightV,
        lightMat
    ));

    scene.addLight(std::make_shared<QuadLight>(
        lightCorner,
        lightU,
        lightV,
        lightEmission
    ));
}
