#include "film.h"
#include <fstream>
#include <algorithm>
#include <cmath>

Film::Film(int w, int h)
    : width(w), height(h), pixels(w* h, Color(0.0, 0.0, 0.0)) {
}

void Film::setPixel(int x, int y, const Color& c) {
    pixels[y * width + x] = c;
}

void Film::savePPM(const std::string& filename, int samplesPerPixel) const {
    std::ofstream out(filename);
    out << "P3\n" << width << " " << height << "\n255\n";

    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            Color c = pixels[j * width + i];

            c /= static_cast<double>(samplesPerPixel);

            c.x = std::sqrt(std::max(0.0, c.x));
            c.y = std::sqrt(std::max(0.0, c.y));
            c.z = std::sqrt(std::max(0.0, c.z));

            int ir = static_cast<int>(256.0 * std::clamp(c.x, 0.0, 0.999));
            int ig = static_cast<int>(256.0 * std::clamp(c.y, 0.0, 0.999));
            int ib = static_cast<int>(256.0 * std::clamp(c.z, 0.0, 0.999));

            out << ir << " " << ig << " " << ib << "\n";
        }
    }
}