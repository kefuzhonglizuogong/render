#pragma once

#include "core/ray.h"

class Camera {
public:
    Point3 origin;
    Point3 lowerLeftCorner;
    Vec3 horizontal;
    Vec3 vertical;

    Camera(double aspectRatio);

    Ray generateRay(double u, double v) const;
};