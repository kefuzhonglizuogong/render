#pragma once

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;

    Vec2(float x_, float y_)
        : x(x_), y(y_) {
    }

    Vec2 operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2 operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    Vec2 operator*(float s) const {
        return Vec2(x * s, y * s);
    }
};
