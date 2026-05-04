#pragma once

#include <string>
#include "core/vec3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

inline void savePNG(
    const std::string& filename,
    const Color* pixels,
    int width,
    int height,
    int samplesPerPixel = 1
) {
    unsigned char* data = new unsigned char[width * height * 3];

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            Color c = pixels[j * width + i];
            c /= static_cast<double>(samplesPerPixel);

            c.x = std::sqrt(c.x);
            c.y = std::sqrt(c.y);
            c.z = std::sqrt(c.z);

            int ir = static_cast<int>(255.999 * std::clamp(c.x, 0.0, 0.999));
            int ig = static_cast<int>(255.999 * std::clamp(c.y, 0.0, 0.999));
            int ib = static_cast<int>(255.999 * std::clamp(c.z, 0.0, 0.999));

            int idx = (j * width + i) * 3;
            data[idx + 0] = static_cast<unsigned char>(ir);
            data[idx + 1] = static_cast<unsigned char>(ig);
            data[idx + 2] = static_cast<unsigned char>(ib);
        }
    }

    stbi_write_png(filename.c_str(), width, height, 3, data, width * 3);
    delete[] data;
}