#include "render/test_scenes.h"

#include "geometry/mesh.h"
#include "geometry/quad.h"
#include "geometry/sphere.h"
#include "io/obj_loader.h"
#include "material/material.h"

#include <memory>

void buildTestScene(Scene& scene) {
    auto whiteMat = std::make_shared<Lambertian>(Color(0.75, 0.75, 0.75));
    auto wallMat = std::make_shared<Lambertian>(Color(0.70, 0.70, 0.70));
    auto redMat = std::make_shared<Lambertian>(Color(0.75, 0.18, 0.16));
    auto blueMat = std::make_shared<Lambertian>(Color(0.18, 0.28, 0.80));
    auto greenMat = std::make_shared<Lambertian>(Color(0.18, 0.65, 0.22));
    auto darkMat = std::make_shared<Lambertian>(Color(0.38, 0.38, 0.38));
    auto baffleMat = std::make_shared<Lambertian>(Color(0.50, 0.50, 0.50));

    const double xMin = -2.6;
    const double xMax = 2.6;
    const double yFloor = -0.5;
    const double yCeiling = 2.20;
    const double zNear = -1.2;
    const double zFar = -5.8;

    // Ground
    scene.add(std::make_shared<Quad>(
        Point3(xMin, yFloor, zNear),
        Vec3(xMax - xMin, 0.0, 0.0),
        Vec3(0.0, 0.0, zFar - zNear),
        whiteMat
    ));

    // Back wall
    scene.add(std::make_shared<Quad>(
        Point3(xMin, yFloor, zFar),
        Vec3(xMax - xMin, 0.0, 0.0),
        Vec3(0.0, yCeiling - yFloor, 0.0),
        wallMat
    ));

    // Left wall
    scene.add(std::make_shared<Quad>(
        Point3(xMin, yFloor, zFar),
        Vec3(0.0, 0.0, zNear - zFar),
        Vec3(0.0, yCeiling - yFloor, 0.0),
        redMat
    ));

    // Right wall
    scene.add(std::make_shared<Quad>(
        Point3(xMax, yFloor, zNear),
        Vec3(0.0, 0.0, zFar - zNear),
        Vec3(0.0, yCeiling - yFloor, 0.0),
        blueMat
    ));

    // Ceiling
    scene.add(std::make_shared<Quad>(
        Point3(xMin, yCeiling, zFar),
        Vec3(xMax - xMin, 0.0, 0.0),
        Vec3(0.0, 0.0, zNear - zFar),
        wallMat
    ));

    // -----------------------------------------------------
    // Baffle with a square hole
    // -----------------------------------------------------

    const double baffleY = 1.35;
    const double baffleXMin = -2.1;
    const double baffleXMax = 2.1;
    const double baffleZNear = -1.55;
    const double baffleZFar = -5.45;
    const double holeXMin = -0.42;
    const double holeXMax = 0.42;
    const double holeZNear = -3.18;
    const double holeZFar = -4.02;

    // Left part of baffle
    scene.add(std::make_shared<Quad>(
        Point3(baffleXMin, baffleY, baffleZNear),
        Vec3(holeXMin - baffleXMin, 0.0, 0.0),
        Vec3(0.0, 0.0, baffleZFar - baffleZNear),
        baffleMat
    ));

    // Right part of baffle
    scene.add(std::make_shared<Quad>(
        Point3(holeXMax, baffleY, baffleZNear),
        Vec3(baffleXMax - holeXMax, 0.0, 0.0),
        Vec3(0.0, 0.0, baffleZFar - baffleZNear),
        baffleMat
    ));

    // Front part of baffle
    scene.add(std::make_shared<Quad>(
        Point3(holeXMin, baffleY, baffleZNear),
        Vec3(holeXMax - holeXMin, 0.0, 0.0),
        Vec3(0.0, 0.0, holeZNear - baffleZNear),
        baffleMat
    ));

    // Back part of baffle
    scene.add(std::make_shared<Quad>(
        Point3(holeXMin, baffleY, holeZFar),
        Vec3(holeXMax - holeXMin, 0.0, 0.0),
        Vec3(0.0, 0.0, baffleZFar - holeZFar),
        baffleMat
    ));

    // -----------------------------------------------------
    // Objects below the baffle
    // -----------------------------------------------------

    const double blockerY = 0.85;
    const double blockerSize = 0.85;

    scene.add(std::make_shared<Quad>(
        Point3(-0.425, blockerY, -3.175),
        Vec3(blockerSize, 0.0, 0.0),
        Vec3(0.0, 0.0, -blockerSize),
        baffleMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(-1.10, -0.05, -3.10),
        0.45,
        greenMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(1.05, -0.18, -3.85),
        0.32,
        whiteMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(0.15, -0.25, -4.85),
        0.25,
        darkMat
    ));

    /*
    // Low vertical blocker below the hole
    scene.add(std::make_shared<Quad>(
        Point3(-0.28, yFloor, -2.65),
        Vec3(0.56, 0.0, 0.0),
        Vec3(0.0, 0.95, 0.0),
        baffleMat
    ));

    // Side blocker to create spatially different visible directions
    scene.add(std::make_shared<Quad>(
        Point3(0.85, yFloor, -4.20),
        Vec3(0.0, 0.0, -0.85),
        Vec3(0.0, 1.05, 0.0),
        baffleMat
    ));
    */
    // -----------------------------------------------------
    // Small area light above the hole
    // -----------------------------------------------------

    Color lightEmission(105.0, 105.0, 105.0);
    auto lightMat = std::make_shared<DiffuseLight>(lightEmission);

    Point3 lightCorner(-0.28, yCeiling - 1e-4, -3.88);
    Vec3 lightU(0.56, 0.0, 0.0);
    Vec3 lightV(0.0, 0.0, 0.56);

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
/*
void buildMaterialTestScene(Scene& scene) {
    // =====================================================
    // Materials
    // =====================================================

    auto groundMat = std::make_shared<Lambertian>(Color(0.75, 0.75, 0.75));
    auto redLambert = std::make_shared<Lambertian>(Color(0.75, 0.20, 0.18));
    auto blueLambert = std::make_shared<Lambertian>(Color(0.18, 0.28, 0.80));
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
    // Back diffuse wall / card
    // =====================================================
    // 不是封闭盒子，只放一个远处背景板，方便看间接光和阴影

    scene.add(std::make_shared<Quad>(
        Point3(-3.0, -0.5, -4.2),
        Vec3(6.0, 0.0, 0.0),
        Vec3(0.0, 2.8, 0.0),
        blueLambert
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
        0.85,
        Point3(0.0, 0.0, -4.0)
    );

    scene.add(bunny);


    Color lightEmission(5.0, 5.0, 5.0);

    auto lightMat =std::make_shared<DiffuseLight>(lightEmission);

    Point3 lightCorner(0.5, 1.4, -2.8);
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
*/