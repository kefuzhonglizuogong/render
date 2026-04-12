#pragma once
#include "ray.h"
#include <cmath>

struct Camera {
    Vec3 origin;    //相机位置
    double fovY;    //垂直视场角
    int width;
    int height;

    Camera(const Vec3& o, double fov_deg, int w, int h)
        : origin(o), fovY(fov_deg), width(w), height(h) {
    }

    Ray generateRay(int px, int py) const {
        double aspect = static_cast<double>(width) / static_cast<double>(height);
        double tanHalfFov = std::tan(fovY * 0.5 * 3.14159265358979323846 / 180.0);//把视场角转换成图像平面大小的关键量

        //像素坐标从整数网格映射到 [0,1]
        double ndcX = (px + 0.5) / width;
        double ndcY = (py + 0.5) / height;

        double screenX = (2.0 * ndcX - 1.0) * aspect * tanHalfFov;//把 [0,1] 映射到 [-1,1]，纠正宽高比，让图像平面大小和视场角对应起来
        double screenY = (1.0 - 2.0 * ndcY) * tanHalfFov;

        Vec3 dir = normalize(Vec3(screenX, screenY, -1.0));//图像平面放在相机前方的 z = -1 处，当前像素对应图像平面上一点(screenX, screenY, -1)，从相机原点看向这个点，就得到该像素对应的射线方向
        return Ray(origin, dir);
    }
};