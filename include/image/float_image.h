#pragma once

#include "core/vec3.h"

#include <vector>
#include <algorithm>

class FloatImage {
public:
    int width = 0;
    int height = 0;

    std::vector<Color> pixels;

    FloatImage() = default;

    FloatImage(int w, int h): width(w),height(h),pixels(static_cast<size_t>(w* h), Color(0.0, 0.0, 0.0)) {
    }

    bool valid() const {
        return width > 0 && height > 0 &&
            pixels.size() == static_cast<size_t>(width * height);
    }

    Color getPixel(int x, int y) const {
        x = std::max(0, std::min(width - 1, x));
        y = std::max(0, std::min(height - 1, y));

        return pixels[static_cast<size_t>(y * width + x)];
    }

    void setPixel(int x, int y, const Color& color) {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return;
        }

        pixels[static_cast<size_t>(y * width + x)] = color;
    }
};