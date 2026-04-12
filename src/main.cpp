#include <iostream>
#include <algorithm>

#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "camera.h"
#include "image.h"

// hit.n      : 交点法线（3D）
// lightDir   : 光照方向（3D，单位向量）
// albedo     : 物体本身颜色
Vec3 shadeLambert(const Hit& hit, const Vec3& lightDir, const Vec3& albedo, bool inShadow) {
    // 环境光强度
    double ambient = 0.3;

    // 如果在阴影里，只保留环境光
    if (inShadow) {
        return albedo * ambient;
    }

    // n·l 表示表面朝向光源的程度
    double ndotl = dot(hit.n, lightDir);
    // 背光面不允许为负，压到 0
    double diffuse = std::max(ndotl, 0.0);
    // 总亮度 = 环境光 + 漫反射
    double lighting = ambient + diffuse;
    return albedo * lighting;
}


Vec3 background(const Ray& ray) {
    double t = 0.5 * (ray.d.y + 1.0);
    return Vec3(1.0, 1.0, 1.0) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
}

int main() {
    const int width = 800;
    const int height = 600;

    Image image(width, height);
    Camera camera(Vec3(0.0, 0.0, 2.0), 45.0, width, height);

    // 主球
    Sphere sphere(Vec3(0.0, 0.0, -3.0), 1.0);

    // 地面：用超大球模拟
    Sphere ground(Vec3(0.0, -1001.0, -3.0), 1000.0);

    // 固定方向光
    Vec3 lightDir = normalize(Vec3(1.0, 1.0, 1.0));

    Vec3 sphereColor(0.9, 0.3, 0.2);
    Vec3 groundColor(0.7, 0.7, 0.7);

    // 阴影射线偏移，避免“自己打到自己”
    const double eps = 1e-4;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Ray ray = camera.generateRay(x, y);

            Hit hitSphere;  //主球的命中信息
            Hit hitGround;  //地面的命中信息

            bool hitS = sphere.intersect(ray, hitSphere);   //有没有打到主球
            bool hitG = ground.intersect(ray, hitGround);   //有没有打到地面

            //选最近命中
            //如果主球打中了并且地面没打中，或者主球更近，那就显示主球。否则如果地面打中了，就显示地面。
            if (hitS && (!hitG || hitSphere.t < hitGround.t)) {
                // 当前真正看到的是主球
                Hit hit = hitSphere;

                // 阴影测试：
                // 从命中点沿光照方向发一条阴影射线
                Ray shadowRay(hit.p + hit.n * eps, lightDir);

                Hit shadowHitSphere;
                Hit shadowHitGround;

                bool shadowBySphere = sphere.intersect(shadowRay, shadowHitSphere);     //阴影射线会不会撞到主球
                bool shadowByGround = ground.intersect(shadowRay, shadowHitGround);     //阴影射线会不会撞到地面

                // 主球自己不应该挡住自己，所以这里只关心有没有别的物体挡光
                // 对主球来说，主要看地面是否挡光。通常不会，但逻辑先写完整。
                bool inShadow = false;
                if (shadowByGround) {
                    inShadow = true;
                }

                image.setPixel(x, y, shadeLambert(hit, lightDir, sphereColor, inShadow));

            }
            else if (hitG) {
                // 当前真正看到的是地面
                Hit hit = hitGround;

                Ray shadowRay(hit.p + hit.n * eps, lightDir);

                Hit shadowHitSphere;
                Hit shadowHitGround;

                bool shadowBySphere = sphere.intersect(shadowRay, shadowHitSphere);
                bool shadowByGround = ground.intersect(shadowRay, shadowHitGround);

                // 地面最重要的是看主球是否挡住了光
                bool inShadow = false;
                if (shadowBySphere) {
                    inShadow = true;
                }

                image.setPixel(x, y, shadeLambert(hit, lightDir, groundColor, inShadow));

            }
            else {
                image.setPixel(x, y, background(ray));
            }
        }
    }

    if (image.savePNG("output_shadow.png")) {
        std::cout << "Saved output_shadow.png\n";
    }
    else {
        std::cout << "Failed to save output_shadow.png\n";
    }

    return 0;
}