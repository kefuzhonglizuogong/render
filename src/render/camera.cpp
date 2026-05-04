#include "render/camera.h"

Camera::Camera(double aspectRatio) {
    double viewportHeight = 2.0;
    double viewportWidth = aspectRatio * viewportHeight;
    double focalLength = 1.0;

    origin = Point3(0.0, 0.0, 0.0);
    horizontal = Vec3(viewportWidth, 0.0, 0.0);
    vertical = Vec3(0.0, viewportHeight, 0.0);

    lowerLeftCorner =
        origin
        - horizontal / 2.0
        - vertical / 2.0
        - Vec3(0.0, 0.0, focalLength);
}

Ray Camera::generateRay(double u, double v) const {
    Vec3 dir = lowerLeftCorner + u * horizontal + v * vertical - origin;
    return Ray(origin, dir.normalized());
}