#include <memory>
#include <iostream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "scene.h"
#include "sphere.h"
#include "plane.h"
#include "quad.h"
#include "triangle.h"
#include "material.h"
#include "camera.h"
#include "film.h"
#include "renderer.h"
#include "light.h"
#include "core/stats.h"
#include "random.h"


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

int main() {
    const double aspectRatio = 16.0 / 9.0;
    const int imageWidth = 300;//400
    const int imageHeight = static_cast<int>(imageWidth / aspectRatio);

    const int samplesPerPixel = 20;//200
    const int maxDepth = 8;//12

    Scene scene;

    Lambertian whiteMat(Color(0.75, 0.75, 0.75));
    Lambertian redMat(Color(0.75, 0.15, 0.15));
    Lambertian greenMat(Color(0.15, 0.75, 0.15));
    Lambertian blueMat(Color(0.20, 0.30, 0.80));
    Lambertian triangleMat(Color(0.1, 0.3, 0.9));

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

    // 盒子中的红球
    scene.add(std::make_shared<Sphere>(
        Point3(-0.35, -0.15, -1.85),
        0.35,
        &redMat
    ));

    // 盒子中的蓝球
    /*scene.add(std::make_shared<Sphere>(
        Point3(0.35, -0.25, -2.35),
        0.25,
        &blueMat
    ));*/

    //盒子中的蓝色三角形
    scene.add(std::make_shared<Triangle>(
        Point3(0.15, -0.45, -2.55),
        Point3(0.85, -0.45, -2.55),
        Point3(0.50, 0.35, -2.55),
        &triangleMat
    ));

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
    const int triCountX = 20;
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
    }
    // 所有物体添加完成后构建 BVH
    scene.buildBVH();

    Camera camera(aspectRatio);
    Film film(imageWidth, imageHeight);
    Renderer renderer(samplesPerPixel, maxDepth);

    gStats.reset();

    auto startTime = std::chrono::high_resolution_clock::now();
    renderer.render(scene, camera, film);
    auto endTime = std::chrono::high_resolution_clock::now();

    const std::filesystem::path outputPath = makeOutputPath();
    film.savePPM(outputPath.string(), samplesPerPixel);

    double renderSeconds =
        std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "Render finished: " << outputPath.string() << std::endl;

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