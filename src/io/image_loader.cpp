#include "io/image_loader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cctype>
#include <algorithm>
#include <cmath>

namespace {

    void skipWhitespaceAndComments(
        std::istream& input
    ) {
        while (true) {
            input >> std::ws;

            if (input.peek() == '#') {
                std::string comment;
                std::getline(input, comment);
                continue;
            }

            break;
        }
    }

    int readIntSkippingComments(
        std::istream& input
    ) {
        skipWhitespaceAndComments(input);

        int value = 0;
        input >> value;

        return value;
    }

    Color srgbToLinear(
        const Color& c
    ) {
        auto convert = [](double x) {
            x = std::max(0.0, std::min(1.0, x));

            if (x <= 0.04045) {
                return x / 12.92;
            }

            return std::pow(
                (x + 0.055) / 1.055,
                2.4
            );
            };

        return Color(
            convert(c.x),
            convert(c.y),
            convert(c.z)
        );
    }

} // namespace

FloatImage loadPPMImage(
    const std::string& filename,
    double intensityScale
) {
    std::ifstream file(
        filename,
        std::ios::binary
    );

    if (!file.is_open()) {
        std::cerr << "Failed to open PPM image: "
            << filename << std::endl;

        return FloatImage();
    }

    std::string magic;
    file >> magic;

    if (magic != "P3" && magic != "P6") {
        std::cerr << "Unsupported PPM format: "
            << magic
            << ". Only P3 and P6 are supported."
            << std::endl;

        return FloatImage();
    }

    int width = readIntSkippingComments(file);
    int height = readIntSkippingComments(file);
    int maxValue = readIntSkippingComments(file);

    if (
        width <= 0 ||
        height <= 0 ||
        maxValue <= 0
        ) {
        std::cerr << "Invalid PPM header: "
            << filename << std::endl;

        return FloatImage();
    }

    FloatImage image(width, height);

    if (magic == "P3") {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int r = readIntSkippingComments(file);
                int g = readIntSkippingComments(file);
                int b = readIntSkippingComments(file);

                Color srgb(
                    static_cast<double>(r) / maxValue,
                    static_cast<double>(g) / maxValue,
                    static_cast<double>(b) / maxValue
                );

                Color linear =
                    srgbToLinear(srgb) * intensityScale;

                image.setPixel(x, y, linear);
            }
        }
    }
    else {
        // P6: header 后面还有一个 whitespace，需要吃掉
        file.get();

        if (maxValue > 255) {
            std::cerr << "P6 with maxValue > 255 is not supported yet: "
                << filename << std::endl;

            return FloatImage();
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                unsigned char rgb[3];
                file.read(
                    reinterpret_cast<char*>(rgb),
                    3
                );

                if (!file) {
                    std::cerr << "Unexpected EOF while reading PPM: "
                        << filename << std::endl;

                    return FloatImage();
                }

                Color srgb(
                    static_cast<double>(rgb[0]) / maxValue,
                    static_cast<double>(rgb[1]) / maxValue,
                    static_cast<double>(rgb[2]) / maxValue
                );

                Color linear =
                    srgbToLinear(srgb) * intensityScale;

                image.setPixel(x, y, linear);
            }
        }
    }

    std::cout << "Loaded PPM image: "
        << filename << std::endl;

    std::cout << "Resolution: "
        << width << " x " << height << std::endl;

    std::cout << "Intensity scale: "
        << intensityScale << std::endl;

    return image;
}