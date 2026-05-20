#include <memory>
#include <iostream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "render/scene.h"
#include "geometry/sphere.h"
#include "geometry/plane.h"
#include "geometry/quad.h"
#include "geometry/triangle.h"
#include "material/material.h"
#include "material/bsdf_debug.h"
#include "render/camera.h"
#include "render/film.h"
#include "render/light_selection_debug.h"
#include "render/renderer.h"
#include "render/render_config.h"
#include "render/test_scenes.h"
#include "light/light.h"
#include "light/environment_light.h"
#include "light/environment_debug.h"
#include "core/stats.h"
#include "core/random.h"
#include "geometry/mesh.h"
#include "io/obj_loader.h"
#include "io/image_loader.h"
#include "image/float_image.h"
#include "guiding/directional_histogram_debug.h"





namespace {
    std::filesystem::path projectPath(const std::string& path) {
        const std::filesystem::path fsPath(path);

        if (fsPath.empty() || fsPath.is_absolute()) {
            return fsPath;
        }

        return std::filesystem::path(PROJECT_ROOT_DIR) / fsPath;
    }

    std::string makeOutputTimestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &nowTime);
#else
        localtime_r(&nowTime, &localTime);
#endif

        std::ostringstream timestampBuilder;
        timestampBuilder << std::put_time(&localTime, "%Y%m%d_%H%M%S");
        return timestampBuilder.str();
    }

    std::filesystem::path makeOutputPath(const std::string& suffix = "") {
        const std::filesystem::path outputDir = std::filesystem::path(PROJECT_ROOT_DIR) / "output";
        std::filesystem::create_directories(outputDir);

        std::ostringstream filenameBuilder;
        filenameBuilder << "render_" << makeOutputTimestamp();

        if (!suffix.empty()) {
            filenameBuilder << "_" << suffix;
        }

        filenameBuilder << ".ppm";
        return outputDir / filenameBuilder.str();
    }

    void resolveConfigPaths(RenderConfig& config) {
        config.objPath = projectPath(config.objPath).string();
        config.environmentPath = projectPath(config.environmentPath).string();

        if (config.outputPath.empty()) {
            config.outputPath = makeOutputPath().string();
        }
        else {
            config.outputPath = projectPath(config.outputPath).string();
        }

        if (config.runGuidingComparison) {
            const std::string timestamp = makeOutputTimestamp();
            const std::filesystem::path outputDir = std::filesystem::path(PROJECT_ROOT_DIR) / "output";
            std::filesystem::create_directories(outputDir);

            if (config.baselineOutputPath.empty()) {
                config.baselineOutputPath = (outputDir / ("render_" + timestamp + "_baseline.ppm")).string();
            }
            else {
                config.baselineOutputPath = projectPath(config.baselineOutputPath).string();
            }

            if (config.guidedOutputPath.empty()) {
                config.guidedOutputPath = (outputDir / ("render_" + timestamp + "_guided.ppm")).string();
            }
            else {
                config.guidedOutputPath = projectPath(config.guidedOutputPath).string();
            }
        }
    }

}  // namespace

int main() {
    //debugDirectionalHistogram();

    RenderConfig config;
    resolveConfigPaths(config);

    const int imageWidth = config.imageWidth;
    const double aspectRatio = config.aspectRatio;
    const int imageHeight = static_cast<int>(imageWidth / aspectRatio);

    const int samplesPerPixel = config.samplesPerPixel;
    const int maxDepth = config.maxDepth;

    Scene scene;

    /*scene.setEnvironment(
        std::make_shared<ConstantEnvironmentLight>(
            Color(0.03, 0.04, 0.06)
        )
    );*/

    /*FloatImage envImage(512, 256);

    for (int y = 0; y < envImage.height; ++y) {
        for (int x = 0; x < envImage.width; ++x) {
            double u =(static_cast<double>(x) + 0.5) /static_cast<double>(envImage.width);

            double v =(static_cast<double>(y) + 0.5) /static_cast<double>(envImage.height);

            Color sky(0.03, 0.05, 0.08);

            // 人造太阳位置
            double sunU = 0.12;
            double sunV = 0.35;

            double du = std::abs(u - sunU);
            du = std::min(du, 1.0 - du);

            double dv = v - sunV;

            double dist2 = du * du + dv * dv;

            Color sun(0.0, 0.0, 0.0);

            // 测试阶段故意放大太阳，确认环境图方向和采样是否正常
            if (dist2 < 0.0040) {
                sun = Color(18.0, 16.0, 10.0);
            }

            envImage.setPixel(x,y,sky + sun);
        }
    }
    */
    FloatImage envImage = loadPPMImage(
        config.environmentPath,
        config.environmentIntensity
    );

    auto envLight =std::make_shared<LatLongEnvironmentLight>(envImage);

    /*debugEnvironmentSampling(
        *envLight,
        100000
    );*/

    scene.setEnvironment(envLight);
    
    /*
    auto whiteMat = std::make_shared<Lambertian>(Color(0.75, 0.75, 0.75));
    auto redMat = std::make_shared<Lambertian>(Color(0.75, 0.15, 0.15));
    auto greenMat = std::make_shared<Lambertian>(Color(0.15, 0.75, 0.15));
    auto blueMat = std::make_shared<Lambertian>(Color(0.20, 0.30, 0.80));
    auto triangleMat = std::make_shared<Lambertian>(Color(0.1, 0.3, 0.9));
    
    auto meshMat = std::make_shared<Lambertian>(Color(0.85, 0.65, 0.25));

    // 批量测试
    auto smallRedMat = std::make_shared<Lambertian>(Color(0.8, 0.2, 0.2));
    auto smallGreenMat = std::make_shared<Lambertian>(Color(0.2, 0.8, 0.2));
    auto smallBlueMat = std::make_shared<Lambertian>(Color(0.2, 0.3, 0.9));
    auto smallWhiteMat = std::make_shared<Lambertian>(Color(0.75, 0.75, 0.75));

    auto lightMat = std::make_shared<DiffuseLight>(Color(12.0, 12.0, 12.0));

    // Cornell Box 尺寸
    // x: -1 到 1
    // y: -0.5 到 1.5
    // z: -1 到 -3
    //
    // 相机在原点，朝向 -z 方向

    // 地面
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, -0.5, -1.0),
        Vec3(2.0, 0.0, 0.0),
        Vec3(0.0, 0.0, -2.0),
        whiteMat
    ));

    // 天花板
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, 1.5, -3.0),
        Vec3(2.0, 0.0, 0.0),
        Vec3(0.0, 0.0, 2.0),
        whiteMat
    ));

    // 后墙
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, -0.5, -3.0),
        Vec3(2.0, 0.0, 0.0),
        Vec3(0.0, 2.0, 0.0),
        whiteMat
    ));

    // 左墙，红色
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, -0.5, -3.0),
        Vec3(0.0, 0.0, 2.0),
        Vec3(0.0, 2.0, 0.0),
        redMat
    ));

    // 右墙，绿色
    scene.add(std::make_shared<Quad>(
        Point3(1.0, -0.5, -1.0),
        Vec3(0.0, 0.0, -2.0),
        Vec3(0.0, 2.0, 0.0),
        greenMat
    ));

    // 顶部矩形面光源
    Point3 lightCorner(-0.35, 1.49, -2.35);
    Vec3 lightU(0.70, 0.0, 0.0);
    Vec3 lightV(0.0, 0.0, 0.70);
    Color lightEmission(12.0, 12.0, 12.0);

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
*/
    // 盒子中的红球
    /*scene.add(std::make_shared<Sphere>(
        Point3(-0.35, -0.15, -1.85),
        0.35,
        redMat
    ));*/

    // 盒子中的蓝球
    /*scene.add(std::make_shared<Sphere>(
        Point3(0.35, -0.25, -2.35),
        0.25,
        blueMat
    ));*/

    // 盒子中的蓝色三角形
    /*scene.add(std::make_shared<Triangle>(
        Point3(0.15, -0.45, -2.55),
        Point3(0.85, -0.45, -2.55),
        Point3(0.50, 0.35, -2.55),
        triangleMat
    ));*/

    // 批量生成小球
    /*const int sphereCountX = 10;
    const int sphereCountZ = 8;

    for (int ix = 0; ix < sphereCountX; ++ix) {
        for (int iz = 0; iz < sphereCountZ; ++iz) {
            double x = -0.85 + ix * (1.70 / (sphereCountX - 1));
            double z = -1.25 - iz * (1.45 / (sphereCountZ - 1));

            double radius = 0.055;

            double jitterX = (randomDouble() - 0.5) * 0.04;
            double jitterZ = (randomDouble() - 0.5) * 0.04;

            Point3 center(
                x + jitterX,
                -0.5 + radius,
                z + jitterZ
            );

            std::shared_ptr<Material> mat = smallWhiteMat;

            int choice = (ix + iz) % 4;
            if (choice == 0) {
                mat = smallRedMat;
            }
            else if (choice == 1) {
                mat = smallGreenMat;
            }
            else if (choice == 2) {
                mat = smallBlueMat;
            }
            else {
                mat = smallWhiteMat;
            }

            scene.add(std::make_shared<Sphere>(
                center,
                radius,
                mat
            ));
        }
    }*/

    // 批量生成三角形
    /*const int triCountX = 20;
    const int triCountY = 10;

    auto triMat = std::make_shared<Lambertian>(Color(0.2, 0.6, 0.9));

    for (int ix = 0; ix < triCountX; ++ix) {
        for (int iy = 0; iy < triCountY; ++iy) {
            double x0 = -0.9 + ix * 0.09;
            double y0 = -0.4 + iy * 0.09;
            double z = -2.75;

            scene.add(std::make_shared<Triangle>(
                Point3(x0, y0, z),
                Point3(x0 + 0.06, y0, z),
                Point3(x0 + 0.03, y0 + 0.07, z),
                triMat
            ));
        }
    }*/

    /*auto pyramid = std::make_shared<Mesh>();

    Point3 p0(-0.35, -0.50, -2.15);
    Point3 p1(0.35, -0.50, -2.15);
    Point3 p2(0.35, -0.50, -2.85);
    Point3 p3(-0.35, -0.50, -2.85);
    Point3 top(0.0, 0.25, -2.50);

    // 底面，两个三角形
    pyramid->addTriangle(p0, p1, p2, meshMat);
    pyramid->addTriangle(p0, p2, p3, meshMat);

    // 四个侧面
    pyramid->addTriangle(p0, p1, top, meshMat);
    pyramid->addTriangle(p1, p2, top, meshMat);
    pyramid->addTriangle(p2, p3, top, meshMat);
    pyramid->addTriangle(p3, p0, top, meshMat);

    pyramid->buildBVH();

    scene.add(pyramid);*/

    //auto meshMat = std::make_shared<Lambertian>(Color(0.85, 0.65, 0.25));

    /*auto objMesh = loadOBJ(
        "D:/Program/Project/mini_renderer/models/simple_pyramid.obj",
        meshMat
    );

    scene.add(objMesh);*/

    // 兔子
    /*auto bunnyMat = std::make_shared<Lambertian>(Color(0.85, 0.75, 0.65));

    auto bunny = loadOBJ(
        config.objPath,
        bunnyMat,
        config.meshTargetSize,
        Point3(
            config.meshCenterX,
            config.meshCenterY,
            config.meshCenterZ
        )
    );
    scene.add(bunny);

    // GGXMetal 球
    auto mirrorMat = std::make_shared<Mirror>(Color(0.95, 0.95, 0.95));
    scene.add(std::make_shared<Sphere>(
        Point3(0.65, -0.20, -2.15),
        0.30,
        mirrorMat
    ));

    // 玻璃球
    auto glass = std::make_shared<Dielectric>(
        Color(1.0, 1.0, 1.0),
        1.5
    );
    scene.add(std::make_shared<Sphere>(
        Point3(0.05, -0.20, -2.15),
        0.30,
        glass
    ));
    */

    /*GGXMetal roughGold(Color(1.0, 0.72, 0.25), 0.25);
    GGXMetal roughSilver(Color(0.9, 0.9, 0.9), 0.12);

    GGXMetal metalVerySmooth(Color(0.9, 0.9, 0.9), 0.05);
    GGXMetal metalMedium(Color(0.9, 0.7, 0.3), 0.25);
    auto metalRough = std::make_shared<GGXMetal>(Color(0.9, 0.7, 0.3), 0.60);
    scene.add(std::make_shared<Sphere>(
        Point3(0.25, -0.20, -2.15),
        0.30,
        metalRough
    ));*/


    buildTestScene(scene);

    // 十万次光源选择测试
    //debugLightSelection(scene, 100000);

    // 所有物体添加完成后构建 BVH
    if (config.enableBVH) {
        scene.buildBVH();
    }


    Camera camera(aspectRatio);

    auto ensureOutputDirectory = [](const std::string& filename) {
        const std::filesystem::path outputPath(filename);
        if (!outputPath.parent_path().empty()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }
    };

    // 对比模式：依次运行 baseline / training / guided 三个阶段。
    if (config.runGuidingComparison) {
        std::cout << "\n=== Baseline Render ===\n";

        // Baseline：不使用 guiding 的普通路径追踪。
        Film baselineFilm(imageWidth, imageHeight);
        Renderer baselineRenderer(samplesPerPixel, maxDepth);
        baselineRenderer.setEnableGuidingRecord(false);
        baselineRenderer.setEnableGuidedSampling(false);

        gStats.reset();

        auto baselineStart = std::chrono::high_resolution_clock::now();
        baselineRenderer.render(scene, camera, baselineFilm);
        auto baselineEnd = std::chrono::high_resolution_clock::now();

        ensureOutputDirectory(config.baselineOutputPath);
        baselineFilm.savePPM(config.baselineOutputPath, samplesPerPixel);

        double baselineTime =
            std::chrono::duration<double>(baselineEnd - baselineStart).count();

        std::cout << "Baseline render finished: " << config.baselineOutputPath << "\n";
        std::cout << "Baseline time seconds: " << baselineTime << "\n";

        std::cout << "\n=== Guiding Training Pass ===\n";

        // Training：收集 guiding 数据，但训练阶段本身不使用 guiding 采样。
        Film trainingFilm(imageWidth, imageHeight);
        Renderer trainingRenderer(config.trainingSamplesPerPixel, maxDepth);
        trainingRenderer.setEnableGuidingRecord(true);
        trainingRenderer.setEnableGuidedSampling(false);

        gStats.reset();

        auto trainingStart = std::chrono::high_resolution_clock::now();
        trainingRenderer.render(scene, camera, trainingFilm);
        auto trainingEnd = std::chrono::high_resolution_clock::now();

        double trainingTime =
            std::chrono::duration<double>(trainingEnd - trainingStart).count();

        std::cout << "Training pass finished.\n";
        std::cout << "Training time seconds: " << trainingTime << "\n";

        std::cout << "\n=== Guided Render ===\n";

        // Guided：复用训练好的分布生成最终输出。
        Film guidedFilm(imageWidth, imageHeight);
        Renderer guidedRenderer(samplesPerPixel, maxDepth);
        guidedRenderer.setEnableGuidingRecord(false);
        guidedRenderer.setEnableGuidedSampling(true);
        guidedRenderer.setGuidingProbability(config.guidingProbability);

        gStats.reset();

        auto guidedStart = std::chrono::high_resolution_clock::now();
        guidedRenderer.render(scene, camera, guidedFilm);
        auto guidedEnd = std::chrono::high_resolution_clock::now();

        ensureOutputDirectory(config.guidedOutputPath);
        guidedFilm.savePPM(config.guidedOutputPath, samplesPerPixel);

        double guidedTime = std::chrono::duration<double>(guidedEnd - guidedStart).count();

        std::cout << "Guided render finished: " << config.guidedOutputPath << "\n";
        std::cout << "Guided render time seconds: " << guidedTime << "\n";

        std::cout << "\n=== Render Stats ===\n";
        std::cout << "Render time seconds:       " << guidedTime << "\n";
        std::cout << "Scene intersect calls:     " << gStats.sceneIntersectCalls << "\n";
        std::cout << "Guiding vertices:          " << gStats.guidingVertices << "\n";
        std::cout << "BVH node intersect calls:  " << gStats.bvhNodeIntersectCalls << "\n";
        std::cout << "AABB hit calls:            " << gStats.aabbHitCalls << "\n";
        std::cout << "Sphere intersect calls:    " << gStats.sphereIntersectCalls << "\n";
        std::cout << "Quad intersect calls:      " << gStats.quadIntersectCalls << "\n";
        std::cout << "Triangle intersect calls:  " << gStats.triangleIntersectCalls << "\n";

        std::uint64_t primitiveCalls =
            gStats.sphereIntersectCalls + gStats.quadIntersectCalls + gStats.triangleIntersectCalls;

        std::cout << "Mesh intersect calls:      " << gStats.meshIntersectCalls << "\n";
        std::cout << "Primitive intersect calls: " << primitiveCalls << "\n";
        std::cout << "====================\n";

        std::cout << "\n=== Guided Sampling Stats ===\n";
        std::cout << "BSDF strategy samples:        " << gStats.bsdfStrategySamples << "\n";
        std::cout << "Guided strategy samples:      " << gStats.guidedStrategySamples << "\n";
        std::cout << "Guided fallback samples:      " << gStats.guidedFallbackSamples << "\n";
        std::cout << "Guided invalid samples:       " << gStats.guidedInvalidSamples << "\n";
        std::cout << "Guided below-surface samples: " << gStats.guidedBelowSurfaceSamples << "\n";
        std::cout << "Guided pdf zero samples:      " << gStats.guidedPdfZeroSamples << "\n";
        std::cout << "Guided pdf bad samples:       " << gStats.guidedPdfBadSamples << "\n";
        std::cout << "Final pdf zero samples:       " << gStats.finalPdfZeroSamples << "\n";
        std::cout << "Final pdf bad samples:        " << gStats.finalPdfBadSamples << "\n";

        std::uint64_t nonDeltaSamples =
            gStats.bsdfStrategySamples + gStats.guidedStrategySamples;

        if (nonDeltaSamples > 0) {
            std::cout << "Guided attempt ratio:         "
                      << static_cast<double>(gStats.guidedStrategySamples) / static_cast<double>(nonDeltaSamples)
                      << "\n";
        }

        if (gStats.guidedStrategySamples > 0) {
            std::cout << "Guided fallback ratio:        "
                      << static_cast<double>(gStats.guidedFallbackSamples) / static_cast<double>(gStats.guidedStrategySamples)
                      << "\n";
        }

        std::cout << "Min BSDF pdf:                 " << gStats.minBsdfPdf << "\n";
        std::cout << "Max BSDF pdf:                 " << gStats.maxBsdfPdf << "\n";
        std::cout << "Min guided pdf:               " << gStats.minGuidedPdf << "\n";
        std::cout << "Max guided pdf:               " << gStats.maxGuidedPdf << "\n";
        std::cout << "Min final pdf:                " << gStats.minFinalPdf << "\n";
        std::cout << "Max final pdf:                " << gStats.maxFinalPdf << "\n";
        std::cout << "=============================\n";

        std::cout << "\n=== Guiding Comparison Summary ===\n";
        std::cout << "Baseline spp:       " << samplesPerPixel << "\n";
        std::cout << "Training spp:       " << config.trainingSamplesPerPixel << "\n";
        std::cout << "Guided spp:         " << samplesPerPixel << "\n";
        std::cout << "Max depth:          " << maxDepth << "\n";
        std::cout << "Guiding probability:" << config.guidingProbability << "\n";
        std::cout << "Baseline time:      " << baselineTime << " s\n";
        std::cout << "Training time:      " << trainingTime << " s\n";
        std::cout << "Guided time:        " << guidedTime << " s\n";
        std::cout << "Total guided cost:  " << trainingTime + guidedTime << " s\n";
        std::cout << "===============================\n";

        return 0;
    }

    // 普通模式：不做 comparison，只渲染一张图。
    Film film(imageWidth, imageHeight);
    Renderer renderer(samplesPerPixel, maxDepth);
    renderer.setEnableGuidingRecord(false);
    renderer.setEnableGuidedSampling(false);

    gStats.reset();

    auto start = std::chrono::high_resolution_clock::now();
    renderer.render(scene, camera, film);
    auto end = std::chrono::high_resolution_clock::now();

    ensureOutputDirectory(config.outputPath);
    film.savePPM(config.outputPath, samplesPerPixel);

    std::cout << "Render finished: " << config.outputPath << "\n";
    std::cout << "Render time seconds: " << std::chrono::duration<double>(end - start).count() << "\n";

    return 0;

    // 下面保留旧的两阶段 guiding 入口，仅用于参考。
    /*
    =====================================================
    旧的 training + guided 渲染入口
    -----------------------------------------------------
    这是旧的两阶段渲染流程：
    1. Training Pass
    2. Guided Render Pass

    它已经被上面的新流程替代：
    - Guiding comparison：baseline -> training -> guided
    - 普通单图渲染 fallback

    这里保留这段旧代码仅用于参考。
    当前不会执行，因为上面的新流程在两条路径上都已经 return。
    =====================================================
    */
    /* Film trainingFilm(imageWidth, imageHeight);
    Renderer trainingRenderer(samplesPerPixel, maxDepth);

    trainingRenderer.setEnableGuidingRecord(true);
    trainingRenderer.setEnableGuidedSampling(false);

    // =====================================================
    // 引导渲染阶段：
    // 使用训练好的 guiding 分布参与方向采样，
    // 最终保存这一阶段的渲染结果。
    // =====================================================
    Film film(imageWidth, imageHeight);
    Renderer renderer(samplesPerPixel, maxDepth);

    renderer.setEnableGuidingRecord(false);
    renderer.setEnableGuidedSampling(true);
    renderer.setGuidingProbability(0.5);

    gStats.reset();

    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "\n=== Training Pass ===\n";
    trainingRenderer.render(scene, camera, trainingFilm);

    std::cout << "\n=== Guided Render Pass ===\n";
    renderer.render(scene, camera, film);

    auto endTime = std::chrono::high_resolution_clock::now();

    const std::filesystem::path outputPath(config.outputPath);
    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    film.savePPM(config.outputPath, samplesPerPixel);

    double renderSeconds = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "Render finished: "<< config.outputPath<< std::endl;

    std::cout << "\n=== Render Stats ===\n";
    std::cout << "Render time seconds:       " << renderSeconds << "\n";
    std::cout << "Scene intersect calls:     " << gStats.sceneIntersectCalls << "\n";
    std::cout << "Guiding vertices:          " << gStats.guidingVertices << "\n";
    std::cout << "BVH node intersect calls:  " << gStats.bvhNodeIntersectCalls << "\n";
    std::cout << "AABB hit calls:            " << gStats.aabbHitCalls << "\n";
    std::cout << "Sphere intersect calls:    " << gStats.sphereIntersectCalls << "\n";
    std::cout << "Quad intersect calls:      " << gStats.quadIntersectCalls << "\n";
    std::cout << "Triangle intersect calls:  " << gStats.triangleIntersectCalls << "\n";

    std::uint64_t primitiveCalls = gStats.sphereIntersectCalls + gStats.quadIntersectCalls + gStats.triangleIntersectCalls;

    std::cout << "Mesh intersect calls:      " << gStats.meshIntersectCalls << "\n";
    std::cout << "Primitive intersect calls: " << primitiveCalls << "\n";
    std::cout << "====================\n";

    std::cout << "\n=== Guided Sampling Stats ===\n";

    std::cout << "BSDF strategy samples:        "
              << gStats.bsdfStrategySamples << "\n";

    std::cout << "Guided strategy samples:      " << gStats.guidedStrategySamples << "\n";

    std::cout << "Guided fallback samples:      " << gStats.guidedFallbackSamples << "\n";

    std::cout << "Guided invalid samples:       " << gStats.guidedInvalidSamples << "\n";

    std::cout << "Guided below-surface samples: " << gStats.guidedBelowSurfaceSamples << "\n";

    std::cout << "Guided pdf zero samples:      " << gStats.guidedPdfZeroSamples << "\n";

    std::cout << "Guided pdf bad samples:       " << gStats.guidedPdfBadSamples << "\n";

    std::cout << "Final pdf zero samples:       " << gStats.finalPdfZeroSamples << "\n";

    std::cout << "Final pdf bad samples:        " << gStats.finalPdfBadSamples << "\n";

    std::uint64_t nonDeltaSamples = gStats.bsdfStrategySamples + gStats.guidedStrategySamples;

    if (nonDeltaSamples > 0) {
        std::cout << "Guided attempt ratio:         " << static_cast<double>(gStats.guidedStrategySamples) / static_cast<double>(nonDeltaSamples) << "\n";
    }

    if (gStats.guidedStrategySamples > 0) {
        std::cout << "Guided fallback ratio:        " << static_cast<double>(gStats.guidedFallbackSamples) / static_cast<double>(gStats.guidedStrategySamples) << "\n";
    }

    std::cout << "Min BSDF pdf:                 " << gStats.minBsdfPdf << "\n";

    std::cout << "Max BSDF pdf:                 " << gStats.maxBsdfPdf << "\n";

    std::cout << "Min guided pdf:               " << gStats.minGuidedPdf << "\n";

    std::cout << "Max guided pdf:               " << gStats.maxGuidedPdf << "\n";

    std::cout << "Min final pdf:                " << gStats.minFinalPdf << "\n";

    std::cout << "Max final pdf:                " << gStats.maxFinalPdf << "\n";

    std::cout << "=============================\n";

    return 0; */
}

/*
* 当前场景里有 6 个 Quad（组成 Cornell Box 的地板、天花板和四面墙），
  1 个 Sphere（球），1 个 Triangle（三角形）。
****************** 关闭 BVH ******************
Rendering line 225 / 225
Render finished: D:/Program/Project/mini_renderer\output\render_20260503_183412.ppm

=== Render Stats ===
Scene intersect calls:     52159438
BVH node intersect calls:  0
AABB hit calls:            0
Sphere intersect calls:    52159438
Quad intersect calls:      312956628
Triangle intersect calls:  52159438
Primitive intersect calls: 417275504
====================

****************** 开启 BVH ******************
Rendering line 225 / 225
Render finished: D:/Program/Project/mini_renderer\output\render_20260503_183726.ppm

=== Render Stats ===
Scene intersect calls:     52166381
BVH node intersect calls:  296778477
AABB hit calls:            296778477
Sphere intersect calls:    25854934
Quad intersect calls:      217500773
Triangle intersect calls:  24626507
Primitive intersect calls: 267982214
====================
核心观察：为什么大概率渲染时间没有变快，甚至可能变慢？
虽然成功跳过了 1.5 亿次图元计算，但也付出了很高的遍历代价：

新增开销：多出了 2.96 亿次 AABB hit calls（包围盒测试）。

这就是经典结论：
当场景里只有 8 个物体时，直接暴力循环求交 8 次的代价，
其实远小于遍历一棵 BVH 树、访问堆内存指针，以及做几十次 AABB 求交的代价。
这就好比为了管理桌子上的 8 支笔，专门买了一个带索引目录的大型档案柜。
每次找笔都要先查目录、开柜子，反而不如直接在桌子上扫一眼更快。
*/

/*
大量小球（80 个）
****************** 关闭 BVH ******************
Rendering line 168 / 168
=== Render Stats ===
Render time seconds:       14.6906
Scene intersect calls:     2887035
BVH node intersect calls:  0
AABB hit calls:            0
Sphere intersect calls:    233849835
Quad intersect calls:      17322210
Triangle intersect calls:  2887035
Primitive intersect calls: 254059080
====================

****************** 开启 BVH ******************
* Rendering line 168 / 168
=== Render Stats ===
Render time seconds:       10.7481
Scene intersect calls:     2888229
BVH node intersect calls:  63500815
AABB hit calls:            63500815
Sphere intersect calls:    5319538
Quad intersect calls:      8924025
Triangle intersect calls:  110215
Primitive intersect calls: 14353778
====================
*/

/*
* 200 个三角形
****************** 关闭 BVH ******************
=== Render Stats ===
Render time seconds:       44.8837
Scene intersect calls:     2905762
BVH node intersect calls:  0
AABB hit calls:            0
Sphere intersect calls:    2905762
Quad intersect calls:      17434572
Triangle intersect calls:  584058162
Primitive intersect calls: 604398496
====================

****************** 开启 BVH ******************
* === Render Stats ===
Render time seconds:       11.9847
Scene intersect calls:     2906880
BVH node intersect calls:  79891276
AABB hit calls:            79891276
Sphere intersect calls:    727349
Quad intersect calls:      7254279
Triangle intersect calls:  2319748
Primitive intersect calls: 10301376
====================
*/

/*
Rendering line 225 / 225
Render finished: D:/Program/Project/mini_renderer\output\render_20260504_105127.ppm

=== Render Stats ===
Render time seconds:       181.47
Scene intersect calls:     52148637
BVH node intersect calls:  1564153995
AABB hit calls:            1564153995
Sphere intersect calls:    6695911
Quad intersect calls:      154048148
Triangle intersect calls:  106157659
Mesh intersect calls:      7306038
Primitive intersect calls: 266901718
====================
*/

/*Stanford Bunny
=== OBJ Normalize Info ===
Original bbox min: -0.35, -0.5, -2.85
Original bbox max: 0.35, 0.25, -2.15
Original center: 0, -0.125, -2.5
Scale factor: 1.33333
Loaded OBJ: D:/Program/Project/mini_renderer/models/simple_pyramid.obj
Vertices: 5
Triangles: 6

=== OBJ Normalize Info ===
Original bbox min: -0.0943804, 0.0333099, -0.0616792
Original bbox max: 0.0607788, 0.186996, 0.0587146
Original center: -0.0168008, 0.110153, -0.00148226
Scale factor: 6.44499
Loaded OBJ: D:/Program/Project/mini_renderer/models/bunny.obj
Vertices: 2503
Triangles: 4968
Rendering line 225 / 225
Render finished: D:/Program/Project/mini_renderer\output\render_20260510_143159.ppm

=== Render Stats ===
Render time seconds:       562.819
Scene intersect calls:     163681593
BVH node intersect calls:  2455459573
AABB hit calls:            2455459573
Sphere intersect calls:    1328
Quad intersect calls:      273412122
Triangle intersect calls:  995286136
Mesh intersect calls:      327359324
Primitive intersect calls: 1268699586
====================
*/

/*BSDF 调试结果（GGX 采样）
=== BSDF Sampling Debug ===   GGXMetal debugMetal(Color(0.9, 0.7, 0.3), 0.25);
Sample count:       100000
Valid samples:      99597
Invalid samples:    403
Zero pdf samples:   0
Min pdf:            0.00123544
Max pdf:            20.3712
Min weight:         0.0197446
Max weight:         0.9
Avg weight:         0.899626
===========================

=== BSDF Sampling Debug ===   GGXMetal debugMetal(Color(0.9, 0.7, 0.3), 0.05);
Sample count:       100000
Valid samples:      99999
Invalid samples:    1
Zero pdf samples:   0
Min pdf:            1.71077e-05
Max pdf:            12732.3
Min weight:         0.899999
Max weight:         0.9
Avg weight:         0.9
===========================
*/
/*
514
LatLongEnvironmentLight distribution built.
Resolution: 512 x 256
Total weight: 27411.1

=== Environment Sampling Debug ===
Sample count:              100000
Valid samples:             100000
Invalid samples:           0
Zero pdf samples:          0
Bad number samples:        0
Pdf mismatch samples:      0
Min pdf:                   0.0116069
Max pdf:                   3.88557
Avg pdf:                   3.33009
Avg sampled luminance:     13.7468
Max sampled luminance:     16.0399
Avg uniform luminance:     0.339608
Max uniform luminance:     16.0399
Sampled / Uniform avg lum: 40.4785
==================================

=== OBJ Normalize Info ===
Original bbox min: -0.0943804, 0.0333099, -0.0616792
Original bbox max: 0.0607788, 0.186996, 0.0587146
Original center: -0.0168008, 0.110153, -0.00148226
Scale factor: 4.18925
Loaded OBJ: D:/Program/Project/mini_renderer/models/bunny.obj
Vertices: 2503
Vertex normals: 0
Triangles: 4968
Rendering line 225 / 225
Render finished: D:/Program/Project/mini_renderer\output\render_20260514_132436.ppm

=== Render Stats ===
Render time seconds:       73.4039
Scene intersect calls:     22049619
BVH node intersect calls:  385471636
AABB hit calls:            385471636
Sphere intersect calls:    9717673
Quad intersect calls:      2530042
Triangle intersect calls:  40797038
Mesh intersect calls:      9717673
Primitive intersect calls: 53044753
====================

*/
/*
Loaded PPM image: D:/Program/Project/mini_renderer\models/test_env.ppm
Resolution: 8 x 4
Intensity scale: 8
LatLongEnvironmentLight distribution built.
Resolution: 8 x 4
Total weight: 16.4792

=== OBJ Normalize Info ===
Original bbox min: -0.0943804, 0.0333099, -0.0616792
Original bbox max: 0.0607788, 0.186996, 0.0587146
Original center: -0.0168008, 0.110153, -0.00148226
Scale factor: 4.83374
Loaded OBJ: D:/Program/Project/mini_renderer/models/bunny.obj
Vertices: 2503
Vertex normals: 0
Triangles: 4968
Rendering line 225 / 225
Render finished: D:/Program/Project/mini_renderer\output\render_20260516_204936.ppm

=== Render Stats ===
Render time seconds:       77.7202
Scene intersect calls:     28569328
Guiding vertices:          6472115
BVH node intersect calls:  164609249
AABB hit calls:            164609249
Sphere intersect calls:    15617334
Quad intersect calls:      12614759
Triangle intersect calls:  3585824
Mesh intersect calls:      4212039
Primitive intersect calls: 31817917
====================
*/

/*
Loaded PPM image: D:/Program/Project/mini_renderer\models/test_env.ppm
Resolution: 8 x 4
Intensity scale: 8
LatLongEnvironmentLight distribution built.
Resolution: 8 x 4
Total weight: 16.4792

=== OBJ Normalize Info ===
Original bbox min: -0.0943804, 0.0333099, -0.0616792
Original bbox max: 0.0607788, 0.186996, 0.0587146
Original center: -0.0168008, 0.110153, -0.00148226
Scale factor: 5.47824
Loaded OBJ: D:/Program/Project/mini_renderer/models/bunny.obj
Vertices: 2503
Vertex normals: 0
Triangles: 4968

=== Baseline Render ===
Rendering line 225 / 225
Baseline render finished: output/baseline.ppm
Baseline time seconds: 98.5362

=== Guiding Training Pass ===
Rendering line 225 / 225

=== Guiding Debug Stats ===
Total vertices:              1190833
Diffuse vertices:            798389
Glossy vertices:             43318
Delta reflection vertices:   60124
Delta transmission vertices: 289002
None vertices:               0
Delta vertices:              349126
Non-delta vertices:          841707
Zero bsdf pdf vertices:      349126
Zero light pdf vertices:     349126
Bad number vertices:         0
Min bsdf pdf:                0.000195432
Max bsdf pdf:                352.744
Avg bsdf pdf:                0.868016
Min light pdf:               0.00433554
Max light pdf:               868.375
Avg light pdf:               0.111518
Min cos theta:               0.000603742
Max cos theta:               1
Avg cos theta:               0.764403
Max throughput component:    1.50734
Avg throughput component:    0.971588

Depth counts:
  depth 0: 669772
  depth 1: 174477
  depth 2: 92405
  depth 3: 63735
  depth 4: 35512
  depth 5: 30067
  depth 6: 26448
  depth 7: 23717
  depth 8: 21395
  depth 9: 19423
  depth 10: 17678
  depth 11: 16204
===========================

=== Guiding Trainer Stats ===
Received vertices:       1190833
Trained vertices:        841707
Skipped delta vertices:  349126
Skipped invalid vertices:0
Skipped bad weights:     0
Total training weight:   613581
Max training weight:     1.5073
Histogram total weight:  613581
Avg training weight:     0.728972
=============================
Training pass finished.
Training time seconds: 14.4051

=== Guided Render ===
Rendering line 225 / 225
Guided render finished: output/guided.ppm
Guided render time seconds: 100.722

=== Render Stats ===
Render time seconds:       100.722
Scene intersect calls:     32469611
Guiding vertices:          0
BVH node intersect calls:  233786272
AABB hit calls:            233786272
Sphere intersect calls:    17279328
Quad intersect calls:      65728021
Triangle intersect calls:  6672018
Mesh intersect calls:      9746099
Primitive intersect calls: 89679367
====================

=== Guided Sampling Stats ===
BSDF strategy samples:        3380782
Guided strategy samples:      3378050
Guided fallback samples:      0
Guided invalid samples:       0
Guided below-surface samples: 0
Guided pdf zero samples:      0
Guided pdf bad samples:       0
Final pdf zero samples:       0
Final pdf bad samples:        0
Guided attempt ratio:         0.499798
Guided fallback ratio:        0
Min BSDF pdf:                 5.98856e-07
Max BSDF pdf:                 469.795
Min guided pdf:               0.013348
Max guided pdf:               0.356823
Min final pdf:                0.00667852
Max final pdf:                234.906
=============================

=== Guiding Comparison Summary ===
Baseline spp:       200
Training spp:       25
Guided spp:         200
Max depth:          12
Guiding probability:0.5
Baseline time:      98.5362 s
Training time:      14.4051 s
Guided time:        100.722 s
Total guided cost:  115.127 s
===============================
*/
