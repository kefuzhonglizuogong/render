#include "light/environment_light.h"

#include "core/random.h"

#include <cmath>
#include <algorithm>
#include <iostream>

namespace {
    constexpr double PI = 3.14159265358979323846;

    double clampDouble(double x, double a, double b) {
        if (x < a) {
            return a;
        }

        if (x > b) {
            return b;
        }

        return x;
    }

    double luminance(const Color& c) {
        return 0.2126 * c.x + 0.7152 * c.y + 0.0722 * c.z;
    }

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

    void directionToUV(const Vec3& direction,double& u,double& v) {
        Vec3 d = direction.normalized();

        double theta = std::acos(clampDouble(d.y, -1.0, 1.0));

        double phi = std::atan2(d.z, d.x);

        if (phi < 0.0) {
            phi += 2.0 * PI;
        }

        u = phi / (2.0 * PI);
        v = theta / PI;
    }

    Vec3 uvToDirection(double u,double v) {
        double phi = u * 2.0 * PI;
        double theta = v * PI;

        double sinTheta = std::sin(theta);

        double x = sinTheta * std::cos(phi);
        double y = std::cos(theta);
        double z = sinTheta * std::sin(phi);

        return Vec3(x, y, z).normalized();
    }
}

// =====================================================
// ConstantEnvironmentLight
// =====================================================

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

double ConstantEnvironmentLight::selectionWeight() const {
    return 4.0 * PI * std::max(0.0, luminance(radiance));
}

// =====================================================
// LatLongEnvironmentLight
// =====================================================

LatLongEnvironmentLight::LatLongEnvironmentLight(const FloatImage& img): image(img) {
    buildDistribution();
}

void LatLongEnvironmentLight::buildDistribution() {
    if (!image.valid()) {
        totalWeight = 0.0;
        return;
    }

    int w = image.width;
    int h = image.height;

    weights.assign(static_cast<size_t>(w * h),0.0);

    marginalCdf.assign(static_cast<size_t>(h + 1),0.0);

    conditionalCdf.assign(static_cast<size_t>(h * (w + 1)),0.0);

    totalWeight = 0.0;

    for (int y = 0; y < h; ++y) {
        double v = (static_cast<double>(y) + 0.5) /static_cast<double>(h);

        double theta = v * PI;
        double sinTheta = std::sin(theta);

        double rowSum = 0.0;

        conditionalCdf[static_cast<size_t>(y * (w + 1))] = 0.0;

        for (int x = 0; x < w; ++x) {
            Color c = image.getPixel(x, y);

            double weight =std::max(0.0, luminance(c)) *std::max(0.0, sinTheta);

            weights[static_cast<size_t>(y * w + x)] = weight;

            rowSum += weight;

            conditionalCdf[
                static_cast<size_t>(y * (w + 1) + x + 1)
            ] = rowSum;
        }

        if (rowSum > 0.0) {
            for (int x = 1; x <= w; ++x) {
                conditionalCdf[
                    static_cast<size_t>(y * (w + 1) + x)
                ] /= rowSum;
            }
        }
        else {
            for (int x = 1; x <= w; ++x) {
                conditionalCdf[
                    static_cast<size_t>(y * (w + 1) + x)
                ] = static_cast<double>(x) /
                        static_cast<double>(w);
            }
        }

        totalWeight += rowSum;
        marginalCdf[static_cast<size_t>(y + 1)] = totalWeight;
    }

    if (totalWeight > 0.0) {
        for (int y = 1; y <= h; ++y) {
            marginalCdf[static_cast<size_t>(y)] /= totalWeight;
        }
    }
    else {
        for (int y = 1; y <= h; ++y) {
            marginalCdf[static_cast<size_t>(y)] =
                static_cast<double>(y) /
                static_cast<double>(h);
        }
    }

    std::cout << "LatLongEnvironmentLight distribution built.\n";
    std::cout << "Resolution: "
        << w << " x " << h << "\n";
    std::cout << "Total weight: "
        << totalWeight << "\n";
}

int LatLongEnvironmentLight::sampleDiscrete(const std::vector<double>& cdf,double u) const {
    auto it = std::upper_bound(cdf.begin(),cdf.end(),u);

    int index = static_cast<int>(std::distance(cdf.begin(), it)) - 1;

    int maxIndex = static_cast<int>(cdf.size()) - 2;

    index = std::max(0, std::min(maxIndex, index));

    return index;
}

Color LatLongEnvironmentLight::eval(const Vec3& direction) const {
    if (!image.valid()) {
        return Color(0.0, 0.0, 0.0);
    }

    double u;
    double v;

    directionToUV(direction, u, v);

    int x = static_cast<int>(u * image.width);
    int y = static_cast<int>(v * image.height);

    if (x >= image.width) {
        x = image.width - 1;
    }

    if (y >= image.height) {
        y = image.height - 1;
    }

    return image.getPixel(x, y);
}

double LatLongEnvironmentLight::texelPdfSolidAngle(int x,int y) const {
    if (!image.valid() ||totalWeight <= 0.0 ||x < 0 ||x >= image.width ||y < 0 ||y >= image.height) {
        return 1.0 / (4.0 * PI);
    }

    int w = image.width;
    int h = image.height;

    double weight =weights[static_cast<size_t>(y * w + x)];

    if (weight <= 0.0) {
        return 0.0;
    }

    double pTexel = weight / totalWeight;

    double v = (static_cast<double>(y) + 0.5) /static_cast<double>(h);

    double theta = v * PI;
    double sinTheta = std::max(1e-6, std::sin(theta));

    double dTheta = PI / static_cast<double>(h);
    double dPhi = 2.0 * PI / static_cast<double>(w);

    double solidAngle = sinTheta * dTheta * dPhi;

    return pTexel / solidAngle;
}

bool LatLongEnvironmentLight::sample(const Point3& refPoint,LightSample& sample) const {
    (void)refPoint;

    if (!image.valid()) {
        return false;
    }

    if (totalWeight <= 0.0) {
        Vec3 wi = sampleUniformSphere();

        sample.position = Point3(0.0, 0.0, 0.0);
        sample.normal = Vec3(0.0, 0.0, 0.0);
        sample.wi = wi;
        sample.distance = 1e30;
        sample.pdf = 1.0 / (4.0 * PI);
        sample.emission = eval(wi);
        sample.isInfinite = true;

        return true;
    }

    double uy = randomDouble();
    int y = sampleDiscrete(marginalCdf, uy);

    std::vector<double> rowCdf(conditionalCdf.begin() + y * (image.width + 1),conditionalCdf.begin() + (y + 1) * (image.width + 1));

    double ux = randomDouble();
    int x = sampleDiscrete(rowCdf, ux);

    double jitterX = randomDouble();
    double jitterY = randomDouble();

    double u =
        (static_cast<double>(x) + jitterX) /
        static_cast<double>(image.width);

    double v =
        (static_cast<double>(y) + jitterY) /
        static_cast<double>(image.height);

    Vec3 wi = uvToDirection(u, v);

    sample.position = Point3(0.0, 0.0, 0.0);
    sample.normal = Vec3(0.0, 0.0, 0.0);
    sample.wi = wi;
    sample.distance = 1e30;
    sample.pdf = texelPdfSolidAngle(x, y);
    sample.emission = eval(wi);
    sample.isInfinite = true;

    return sample.pdf > 1e-12;
}

double LatLongEnvironmentLight::pdf(const Point3& refPoint,const Vec3& wi) const {
    (void)refPoint;

    if (!image.valid()) {
        return 0.0;
    }

    if (totalWeight <= 0.0) {
        return 1.0 / (4.0 * PI);
    }

    double u;
    double v;

    directionToUV(wi, u, v);

    int x = static_cast<int>(u * image.width);
    int y = static_cast<int>(v * image.height);

    if (x >= image.width) {
        x = image.width - 1;
    }

    if (y >= image.height) {
        y = image.height - 1;
    }

    return texelPdfSolidAngle(x, y);
}

double LatLongEnvironmentLight::selectionWeight() const {
    if (totalWeight > 0.0) {
        return totalWeight;
    }

    return 1.0;
}
