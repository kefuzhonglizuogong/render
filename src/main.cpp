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
#include "render/renderer.h"
#include "render/render_config.h"
#include "light/light.h"
#include "light/environment_light.h"
#include "light/environment_debug.h"
#include "core/stats.h"
#include "core/random.h"
#include "geometry/mesh.h"
#include "io/obj_loader.h"
#include "io/image_loader.h"
#include "image/float_image.h"




namespace {
    std::filesystem::path projectPath(const std::string& path) {
        const std::filesystem::path fsPath(path);

        if (fsPath.empty() || fsPath.is_absolute()) {
            return fsPath;
        }

        return std::filesystem::path(PROJECT_ROOT_DIR) / fsPath;
    }

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

    void resolveConfigPaths(RenderConfig& config) {
        config.objPath = projectPath(config.objPath).string();
        config.environmentPath = projectPath(config.environmentPath).string();

        if (config.outputPath.empty()) {
            config.outputPath = makeOutputPath().string();
        }
        else {
            config.outputPath = projectPath(config.outputPath).string();
        }
    }

}  // namespace

int main() {
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

            // 测试阶段故意放大太阳，确认环境图方向和采样正常
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

    auto envLight =
        std::make_shared<LatLongEnvironmentLight>(
            envImage
        );

    /*debugEnvironmentSampling(
        *envLight,
        100000
    );*/

    scene.setEnvironment(envLight);

    Lambertian whiteMat(Color(0.75, 0.75, 0.75));
    Lambertian redMat(Color(0.75, 0.15, 0.15));
    Lambertian greenMat(Color(0.15, 0.75, 0.15));
    Lambertian blueMat(Color(0.20, 0.30, 0.80));
    Lambertian triangleMat(Color(0.1, 0.3, 0.9));
    
    Lambertian meshMat(Color(0.85, 0.65, 0.25));

    //批量测试
    Lambertian smallRedMat(Color(0.8, 0.2, 0.2));
    Lambertian smallGreenMat(Color(0.2, 0.8, 0.2));
    Lambertian smallBlueMat(Color(0.2, 0.3, 0.9));
    Lambertian smallWhiteMat(Color(0.75, 0.75, 0.75));

    DiffuseLight lightMat(Color(12.0, 12.0, 12.0));

    // Cornell Box 尺寸
    // x: -1 到 1
    // y: -0.5 到 1.5
    // z: -1 到 -3
    //
    // 相机在原点，看向 -z 方向。

    // 地面
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, -0.5, -1.0),
        Vec3(2.0, 0.0, 0.0),
        Vec3(0.0, 0.0, -2.0),
        &whiteMat
    ));
/*
    // 天花板
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, 1.5, -3.0),
        Vec3(2.0, 0.0, 0.0),
        Vec3(0.0, 0.0, 2.0),
        &whiteMat
    ));

    // 后墙
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, -0.5, -3.0),
        Vec3(2.0, 0.0, 0.0),
        Vec3(0.0, 2.0, 0.0),
        &whiteMat
    ));

    // 左墙，红色
    scene.add(std::make_shared<Quad>(
        Point3(-1.0, -0.5, -3.0),
        Vec3(0.0, 0.0, 2.0),
        Vec3(0.0, 2.0, 0.0),
        &redMat
    ));

    // 右墙，绿色
    scene.add(std::make_shared<Quad>(
        Point3(1.0, -0.5, -1.0),
        Vec3(0.0, 0.0, -2.0),
        Vec3(0.0, 2.0, 0.0),
        &greenMat
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
        &lightMat
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
        &redMat
    ));*/

    // 盒子中的蓝球
    /*scene.add(std::make_shared<Sphere>(
        Point3(0.35, -0.25, -2.35),
        0.25,
        &blueMat
    ));*/

    //盒子中的蓝色三角形
    /*scene.add(std::make_shared<Triangle>(
        Point3(0.15, -0.45, -2.55),
        Point3(0.85, -0.45, -2.55),
        Point3(0.50, 0.35, -2.55),
        &triangleMat
    ));*/

    //批量生成小球
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

            Material* mat = &smallWhiteMat;

            int choice = (ix + iz) % 4;
            if (choice == 0) {
                mat = &smallRedMat;
            }
            else if (choice == 1) {
                mat = &smallGreenMat;
            }
            else if (choice == 2) {
                mat = &smallBlueMat;
            }
            else {
                mat = &smallWhiteMat;
            }

            scene.add(std::make_shared<Sphere>(
                center,
                radius,
                mat
            ));
        }
    }*/

    //批量生成三角形
    /*const int triCountX = 20;
    const int triCountY = 10;

    Lambertian triMat(Color(0.2, 0.6, 0.9));

    for (int ix = 0; ix < triCountX; ++ix) {
        for (int iy = 0; iy < triCountY; ++iy) {
            double x0 = -0.9 + ix * 0.09;
            double y0 = -0.4 + iy * 0.09;
            double z = -2.75;

            scene.add(std::make_shared<Triangle>(
                Point3(x0, y0, z),
                Point3(x0 + 0.06, y0, z),
                Point3(x0 + 0.03, y0 + 0.07, z),
                &triMat
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
    pyramid->addTriangle(p0, p1, p2, &meshMat);
    pyramid->addTriangle(p0, p2, p3, &meshMat);

    // 四个侧面
    pyramid->addTriangle(p0, p1, top, &meshMat);
    pyramid->addTriangle(p1, p2, top, &meshMat);
    pyramid->addTriangle(p2, p3, top, &meshMat);
    pyramid->addTriangle(p3, p0, top, &meshMat);

    pyramid->buildBVH();

    scene.add(pyramid);*/

    //Lambertian meshMat(Color(0.85, 0.65, 0.25));

    /*auto objMesh = loadOBJ(
        "D:/Program/Project/mini_renderer/models/simple_pyramid.obj",
        &meshMat
    );

    scene.add(objMesh);*/

    //兔子
    /*Lambertian bunnyMat(Color(0.85, 0.75, 0.65));

    auto bunny = loadOBJ(
        config.objPath,
        &bunnyMat,
        config.meshTargetSize,
        Point3(
            config.meshCenterX,
            config.meshCenterY,
            config.meshCenterZ
        )
    );
    scene.add(bunny);*/

    //GGXMetal球
    Mirror mirrorMat(Color(0.95, 0.95, 0.95));
    scene.add(std::make_shared<Sphere>(
        Point3(0.65, -0.20, -2.15),
        0.30,
        &mirrorMat
    ));

    //玻璃球
    Dielectric glass(
        Color(1.0, 1.0, 1.0),
        1.5
    );
    scene.add(std::make_shared<Sphere>(
        Point3(0.05, -0.20, -2.15),
        0.30,
        &glass
    ));

    /*GGXMetal roughGold(Color(1.0, 0.72, 0.25), 0.25);
    GGXMetal roughSilver(Color(0.9, 0.9, 0.9), 0.12);

    GGXMetal metalVerySmooth(Color(0.9, 0.9, 0.9), 0.05);
    GGXMetal metalMedium(Color(0.9, 0.7, 0.3), 0.25);
    GGXMetal metalRough(Color(0.9, 0.7, 0.3), 0.60);
    scene.add(std::make_shared<Sphere>(
        Point3(0.25, -0.20, -2.15),
        0.30,
        &metalRough
    ));*/

    /*GGXMetal debugMetal(Color(0.9, 0.7, 0.3), 0.05);

    debugBSDFSampling(
        debugMetal,
        Vec3(0.0, 1.0, 0.0),
        Vec3(0.0, 1.0, 0.0),
        100000
    );*/


    // 所有物体添加完成后构建 BVH
    if (config.enableBVH) {
        scene.buildBVH();
    }


    Camera camera(aspectRatio);
    Film film(imageWidth, imageHeight);
    Renderer renderer(samplesPerPixel, maxDepth);

    gStats.reset();

    auto startTime = std::chrono::high_resolution_clock::now();
    renderer.render(scene, camera, film);
    auto endTime = std::chrono::high_resolution_clock::now();

    const std::filesystem::path outputPath(config.outputPath);
    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    film.savePPM(config.outputPath, samplesPerPixel);

    double renderSeconds =
        std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "Render finished: "
        << config.outputPath
        << std::endl;

    std::cout << "\n=== Render Stats ===\n";
    std::cout << "Render time seconds:       " << renderSeconds << "\n";
    std::cout << "Scene intersect calls:     " << gStats.sceneIntersectCalls << "\n";
    std::cout << "BVH node intersect calls:  " << gStats.bvhNodeIntersectCalls << "\n";
    std::cout << "AABB hit calls:            " << gStats.aabbHitCalls << "\n";
    std::cout << "Sphere intersect calls:    " << gStats.sphereIntersectCalls << "\n";
    std::cout << "Quad intersect calls:      " << gStats.quadIntersectCalls << "\n";
    std::cout << "Triangle intersect calls:  " << gStats.triangleIntersectCalls << "\n";

    std::uint64_t primitiveCalls =
        gStats.sphereIntersectCalls +
        gStats.quadIntersectCalls +
        gStats.triangleIntersectCalls;

    std::cout << "Mesh intersect calls:      " << gStats.meshIntersectCalls << "\n";
    std::cout << "Primitive intersect calls: " << primitiveCalls << "\n";
    std::cout << "====================\n";

    return 0;
}

/*
* 现在的场景里，有 6 个 Quad（组成了 Cornell Box 的地板、天花板和四面墙），1 个 Sphere（球），1 个 Triangle（三角形）。
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
核心洞察：为什么大概率渲染时间没变快（甚至变慢了）？
虽然成功跳过了 1.5 亿次图元计算，但你付出了一笔极其高昂的过路费：

新增开销： 多出了 2.96 亿次 AABB hit calls（安检门测试）！

这就是经典悖论：
当场景里只有 8 个物体时，直接暴力循环算 8 次数学方程的代价，其实远远小于去遍历一棵 BVH 树、访问堆内存指针、以及算几十次 AABB 长方体求交的代价。
这就好比，你为了管理桌子上的 8 支笔，专门买了一个带索引目录的大型档案柜。每次找笔都要先查目录、开柜子，反而不如直接在桌子上扫一眼来得快。
*/

/*
大量小球(80个)
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
* 200个三角形
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