#include "render/texture/texture.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

bool Texture::loadFromFile(const std::string& path) {
    int loadedWidth = 0;
    int loadedHeight = 0;
    int loadedChannels = 0;

    unsigned char* loadedData =
        stbi_load(path.c_str(), &loadedWidth, &loadedHeight, &loadedChannels, 3);

    if (!loadedData) {
        std::cerr << "[Texture] Failed to load: " << path << "\n";
        return false;
    }

    width = loadedWidth;
    height = loadedHeight;
    channels = 3;

    size_t byteCount =
        static_cast<size_t>(width) *
        static_cast<size_t>(height) *
        static_cast<size_t>(channels);

    data.assign(loadedData, loadedData + byteCount);

    stbi_image_free(loadedData);

    std::cout << "[Texture] Loaded: " << path << " (" << width << "x" << height << ")\n";

    return true;
}

Vec3 Texture::sample(float u, float v) const {
    if (data.empty() || width <= 0 || height <= 0) {
        return Vec3(1.0, 1.0, 1.0);
    }

    u = u - std::floor(u);
    v = v - std::floor(v);

    int x = std::clamp(static_cast<int>(u * static_cast<float>(width)), 0, width - 1);
    int y = std::clamp(static_cast<int>((1.0f - v) * static_cast<float>(height)), 0, height - 1);

    int index = (y * width + x) * channels;

    double r = static_cast<double>(data[index + 0]) / 255.0;
    double g = static_cast<double>(data[index + 1]) / 255.0;
    double b = static_cast<double>(data[index + 2]) / 255.0;

    return Vec3(r, g, b);
}

bool Texture::isValid() const {
    return !data.empty() && width > 0 && height > 0;
}
