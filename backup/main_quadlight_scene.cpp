#include <memory>
#include <iostream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "scene.h"
#include "sphere.h"
#include "quad.h"
#include "plane.h"
#include "material.h"
#include "camera.h"
#include "film.h"
#include "renderer.h"
#include "light.h"

namespace {

    std::filesystem::path makeOutputPath() {
        const std::filesystem::path outputDir = std::filesystem::path(PROJECT_ROOT_DIR) / "output";
        std::filesystem::create_directories(outputDir);

        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &nowTime);
#else
        localtime_r(&nowTime, &localTime);
#endif

        std::ostringstream filenameBuilder;
        filenameBuilder << "render_" << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".ppm";
        return outputDir / filenameBuilder.str();
    }

}  // namespace

int testSomething() {
    const double aspectRatio = 16.0 / 9.0;
    const int imageWidth = 400;
    const int imageHeight = static_cast<int>(imageWidth / aspectRatio);
    const int samplesPerPixel = 50;
    const int maxDepth = 10;

    Lambertian groundMat(Color(0.8, 0.8, 0.0));
    Lambertian centerMat(Color(0.7, 0.3, 0.3));
    Lambertian blueMat(Color(0.2, 0.3, 0.8));

    // 光源 1：画面上方偏左，白光
    Point3 lightCenter1(-0.8, 1.2, -1.4);
    double lightRadius1 = 0.22;
    Color lightEmission1(12.0, 12.0, 12.0);
    DiffuseLight lightMat1(lightEmission1);

    // 光源 2：画面上方偏右，稍微偏暖
    Point3 lightCenter2(0.9, 0.9, -1.8);
    double lightRadius2 = 0.18;
    Color lightEmission2(10.0, 7.0, 4.0);
    DiffuseLight lightMat2(lightEmission2);

    // 光源 3：画面左上，更弱一点，偏蓝
    Point3 lightCenter3(-1.2, 0.5, -2.2);
    double lightRadius3 = 0.20;
    Color lightEmission3(3.0, 5.0, 10.0);
    DiffuseLight lightMat3(lightEmission3);

    // 顶部矩形面光源
    Point3 quadLightCorner(-0.6, 1.3, -2.1);
    Vec3 quadLightU(1.2, 0.0, 0.0);
    Vec3 quadLightV(0.0, 0.0, 0.8);
    Color quadLightEmission(8.0, 8.0, 8.0);

    DiffuseLight quadLightMat(quadLightEmission);

    Scene scene;

    scene.add(std::make_shared<Plane>(
        Point3(0.0, -0.5, 0.0),
        Vec3(0.0, 1.0, 0.0),
        &groundMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(0.0, 0.0, -1.5),
        0.5,
        &centerMat
    ));

    scene.add(std::make_shared<Sphere>(
        Point3(1.0, -0.1, -2.2),
        0.35,
        &blueMat
    ));

    // 光源 1：几何体身份
    scene.add(std::make_shared<Sphere>(
        lightCenter1,
        lightRadius1,
        &lightMat1
    ));

    // 光源 1：采样器身份
    scene.addLight(std::make_shared<SphereLight>(
        lightCenter1,
        lightRadius1,
        lightEmission1
    ));

    // 光源 2：几何体身份
    scene.add(std::make_shared<Sphere>(
        lightCenter2,
        lightRadius2,
        &lightMat2
    ));

    // 光源 2：采样器身份
    scene.addLight(std::make_shared<SphereLight>(
        lightCenter2,
        lightRadius2,
        lightEmission2
    ));

    // 光源 3：几何体身份
    scene.add(std::make_shared<Sphere>(
        lightCenter3,
        lightRadius3,
        &lightMat3
    ));

    // 光源 3：采样器身份
    scene.addLight(std::make_shared<SphereLight>(
        lightCenter3,
        lightRadius3,
        lightEmission3
    ));

    // 矩形光源 ：几何体身份
    scene.add(std::make_shared<Quad>(
        quadLightCorner,
        quadLightU,
        quadLightV,
        &quadLightMat
    ));
    // 矩形光源 ：采样器身份
    scene.addLight(std::make_shared<QuadLight>(
        quadLightCorner,
        quadLightU,
        quadLightV,
        quadLightEmission
    ));

    Camera camera(aspectRatio);
    Film film(imageWidth, imageHeight);
    Renderer renderer(samplesPerPixel, maxDepth);

    renderer.render(scene, camera, film);
    const std::filesystem::path outputPath = makeOutputPath();

    film.savePPM(outputPath.string(), samplesPerPixel);

    std::cout << "Render finished: " << outputPath.string() << std::endl;
    return 0;
}
