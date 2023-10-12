#include <cmath>

#include "subvolume.hpp"

static const float tolerance = 1e-3f;

float fmod_with_tolerance(float x, float y) {
    float remainder = std::fmod(x, y);

    if (std::abs(remainder - y) < tolerance) {
        return 0;
    }
    return remainder;
}

float floor_with_tolerance(float x) {
    float ceil = std::ceil(x);

    if (std::abs(ceil - x) < tolerance) {
        return ceil;
    }
    return std::floor(x);
}

float ceil_with_tolerance(float x) {
    float floor = std::floor(x);

    if (std::abs(x - floor) < tolerance) {
        return floor;
    }
    return std::ceil(x);
}
