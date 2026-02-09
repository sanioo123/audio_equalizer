#pragma once
#include <cmath>
#include <algorithm>

namespace dsp {

inline float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

inline float linearToDb(float linear) {
    if (linear < 1e-10f) return -96.0f;
    return 20.0f * std::log10(linear);
}

inline float clamp(float value, float min, float max) {
    return std::min(std::max(value, min), max);
}

constexpr float PI = 3.14159265358979323846f;

} // namespace dsp
