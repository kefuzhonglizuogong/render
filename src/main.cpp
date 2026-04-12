#include <iostream>
#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "camera.h"
#include "image.h"

Vec3 shadeNormal(const Hit& hit) {
    return Vec3(
        0.5 * (hit.n.x + 1.0),
        0.5 * (hit.n.y + 1.0),
        0.5 * (hit.n.z + 1.0)
    );
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

    Sphere sphere(Vec3(0.0, 0.0, -3.0), 1.0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Ray ray = camera.generateRay(x, y);

            Hit hit;
            if (sphere.intersect(ray, hit)) {
                image.setPixel(x, y, shadeNormal(hit));
            }
            else {
                image.setPixel(x, y, background(ray));
            }
        }
    }

    if (image.savePNG("output.png")) {
        std::cout << "Saved output.png\n";
    }
    else {
        std::cout << "Failed to save output.png\n";
    }

    return 0;
}