#include "light/environment_light.h"

#include "core/random.h"

#include <cmath>
#include <algorithm>

namespace {
    constexpr double PI = 3.14159265358979323846;

    Vec3 sampleUniformSphere() {
        double u1 = randomDouble();
        double u2 = randomDouble();

        double z = 1.0 - 2.0 * u1;
        double r = std::sqrt(std::max(0.0, 1.0 - z * z));
        double phi = 2.0 * PI * u2;

        double x = r * std::cos(phi);
        double y = z;
        double zz = r * std::sin(phi);

        return Vec3(x, y, zz).normalized();
    }
}

ConstantEnvironmentLight::ConstantEnvironmentLight(const Color& r): radiance(r) {
}

Color ConstantEnvironmentLight::eval(const Vec3& direction) const {
    (void)direction;

    return radiance;
}

bool ConstantEnvironmentLight::sample(const Point3& refPoint,LightSample& sample) const {
    (void)refPoint;

    Vec3 wi = sampleUniformSphere();

    sample.position = Point3(0.0, 0.0, 0.0);
    sample.normal = Vec3(0.0, 0.0, 0.0);
    sample.wi = wi;
    sample.distance = 1e30;
    sample.pdf = 1.0 / (4.0 * PI);
    sample.emission = radiance;
    sample.isInfinite = true;

    return true;
}

double ConstantEnvironmentLight::pdf(const Point3& refPoint,const Vec3& wi) const {
    (void)refPoint;
    (void)wi;

    return 1.0 / (4.0 * PI);
}