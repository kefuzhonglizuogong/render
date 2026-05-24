#include "render/test_scenes.h"

#include "geometry/mesh.h"
#include "geometry/quad.h"
#include "geometry/sphere.h"
#include "io/obj_loader.h"
#include "material/material.h"

#include <memory>

/*
1. 主要使用 Lambertian 材质
2. 有地面、背墙、侧墙，形成间接光环境
3. 小面积强光源放在上方偏侧位置
4. 加几个遮挡物，让直接采样和 BSDF 采样更有挑战
5. 暂时不放 glass / mirror / GGX，避免干扰判断
*/
void buildTestScene(Scene& scene) {
    auto whiteMat = std::make_shared<Lambertian>(Color(0.75, 0.75, 0.75));
    auto redMat = std::make_shared<Lambertian>(Color(0.75, 0.18, 0.16));
    auto greenMat = std::make_shared<Lambertian>(Color(0.18, 0.65, 0.22));
    auto blueMat = std::make_shared<Lambertian>(Color(0.18, 0.28, 0.80));
    auto darkMat = std::make_shared<Lambertian>(Color(0.45, 0.45, 0.45));
    auto blockerMat = std::make_shared<Lambertian>(Color(0.55, 0.55, 0.55));

    // Ground
    scene.add(std::make_shared<Quad>(
        Point3(-3.0, -0.5, -1.0),
        Vec3(6.0, 0.0, 0.0),
        Vec3(0.0, 0.0, -5.5),
        whiteMat
    ));

    // Back wall
    scene.add(std::make_shared<Quad>(
        Point3(-3.0, -0.5, -6.5),
        Vec3(6.0, 0.0, 0.0),
        Vec3(0.0, 3.0, 0.0),
        whiteMat
    ));

    // Left wall
    scene.add(std::make_shared<Quad>(
        Point3(-3.0, -0.5, -6.5),
        Vec3(0.0, 0.0, 5.5),
        Vec3(0.0, 3.0, 0.0),
        redMat
    ));

    // Right wall
    scene.add(std::make_shared<Quad>(
        Point3(3.0, -0.5, -1.0),
        Vec3(0.0, 0.0, -5.5),
        Vec3(0.0, 3.0, 0.0),
        blueMat
    ));

    // Ceiling
    scene.add(std::make_shared<Quad>(
        Point3(-3.0, 2.5, -6.5),
        Vec3(6.0, 0.0, 0.0),
        Vec3(0.0, 0.0, 5.5),
        whiteMat
    ));

    // Small diffuse objects
    scene.add(std::make_shared<Sphere>(
        Point3(-1.25, -0.05, -3.25),
        0.45,
        greenMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(1.05, -0.18, -3.65),
        0.32,
        whiteMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(0.15, -0.28, -4.65),
        0.22,
        darkMat
    ));

    // 垂直遮挡体：会削弱直射光照占比，同时扩大间接光照作用区域。
    scene.add(std::make_shared<Quad>(
        Point3(-0.35, -0.5, -2.75),
        Vec3(0.70, 0.0, 0.0),
        Vec3(0.0, 1.75, 0.0),
        blockerMat
    ));

    // 侧向遮挡体：会形成暗区，在此类区域中，空间引导与全局引导的采样表现会出现明显差异。
    scene.add(std::make_shared<Quad>(
        Point3(0.85, -0.5, -4.2),
        Vec3(0.0, 0.0, -0.9),
        Vec3(0.0, 1.35, 0.0),
        blockerMat
    ));

    // Small bright area light.
    Color lightEmission(55.0, 55.0, 55.0);
    auto lightMat = std::make_shared<DiffuseLight>(lightEmission);

    Point3 lightCorner(-0.28, 2.38, -3.95);
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