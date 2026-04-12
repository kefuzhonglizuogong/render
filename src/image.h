#pragma once
#include <vector>
#include <string>
#include "vec3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Image {
    int width;
    int height;
    std::vector<unsigned char> pixels;

    Image(int w, int h) : width(w), height(h), pixels(w* h * 3, 0) {}

    void setPixel(int x, int y, const Vec3& color) {
        Vec3 c = clamp01(color);
        int idx = (y * width + x) * 3;
        pixels[idx + 0] = static_cast<unsigned char>(255.0 * c.x);
        pixels[idx + 1] = static_cast<unsigned char>(255.0 * c.y);
        pixels[idx + 2] = static_cast<unsigned char>(255.0 * c.z);
    }

    bool savePNG(const std::string& filename) const {
        return stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3) != 0;
    }
};
