#pragma once

#include "core/vec3.h"

#include <string>
#include <vector>

class Texture {
public:
    Texture() = default;

    bool loadFromFile(const std::string& path);
    Vec3 sample(float u, float v) const;
    bool isValid() const;

private:
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> data;
};
