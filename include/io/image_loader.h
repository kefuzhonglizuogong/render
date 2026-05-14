#pragma once

#include "image/float_image.h"

#include <string>

FloatImage loadPPMImage(
    const std::string& filename,
    double intensityScale = 1.0
);