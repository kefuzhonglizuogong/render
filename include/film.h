#pragma once

#include <string>
#include <vector>
#include "vec3.h"

class Film {
public:
    int width;
    int height;
    std::vector<Color> pixels;

    Film(int w, int h);

    void setPixel(int x, int y, const Color& c);
    void savePPM(const std::string& filename, int samplesPerPixel) const;
};