#include "biquad.h"
#include "dsp_common.h"
#include <cmath>

void Biquad::setParams(Type type, float freqHz, float gainDb, float Q, float sampleRate) {
    float omega = 2.0f * dsp::PI * freqHz / sampleRate;
    float sinW = std::sin(omega);
    float cosW = std::cos(omega);
    float alpha = sinW / (2.0f * Q);

    float b0, b1, b2, a0, a1, a2;

    switch (type) {
    case Type::PeakingEQ: {
        float A = std::pow(10.0f, gainDb / 40.0f);
        b0 =  1.0f + alpha * A;
        b1 = -2.0f * cosW;
        b2 =  1.0f - alpha * A;
        a0 =  1.0f + alpha / A;
        a1 = -2.0f * cosW;
        a2 =  1.0f - alpha / A;
        break;
    }
    case Type::HighPass: {
        b0 =  (1.0f + cosW) / 2.0f;
        b1 = -(1.0f + cosW);
        b2 =  (1.0f + cosW) / 2.0f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosW;
        a2 =  1.0f - alpha;
        break;
    }
    case Type::LowPass: {
        b0 = (1.0f - cosW) / 2.0f;
        b1 =  1.0f - cosW;
        b2 = (1.0f - cosW) / 2.0f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosW;
        a2 =  1.0f - alpha;
        break;
    }
    case Type::LowShelf: {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float sqrtA2alpha = 2.0f * std::sqrt(A) * alpha;
        b0 =     A * ((A + 1.0f) - (A - 1.0f) * cosW + sqrtA2alpha);
        b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW);
        b2 =     A * ((A + 1.0f) - (A - 1.0f) * cosW - sqrtA2alpha);
        a0 =          (A + 1.0f) + (A - 1.0f) * cosW + sqrtA2alpha;
        a1 =  -2.0f * ((A - 1.0f) + (A + 1.0f) * cosW);
        a2 =          (A + 1.0f) + (A - 1.0f) * cosW - sqrtA2alpha;
        break;
    }
    case Type::HighShelf: {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float sqrtA2alpha = 2.0f * std::sqrt(A) * alpha;
        b0 =      A * ((A + 1.0f) + (A - 1.0f) * cosW + sqrtA2alpha);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW);
        b2 =      A * ((A + 1.0f) + (A - 1.0f) * cosW - sqrtA2alpha);
        a0 =           (A + 1.0f) - (A - 1.0f) * cosW + sqrtA2alpha;
        a1 =   2.0f * ((A - 1.0f) - (A + 1.0f) * cosW);
        a2 =           (A + 1.0f) - (A - 1.0f) * cosW - sqrtA2alpha;
        break;
    }
    case Type::BandPass: {
        b0 =  alpha;
        b1 =  0.0f;
        b2 = -alpha;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosW;
        a2 =  1.0f - alpha;
        break;
    }
    }

    float invA0 = 1.0f / a0;
    b0_ = b0 * invA0;
    b1_ = b1 * invA0;
    b2_ = b2 * invA0;
    a1_ = a1 * invA0;
    a2_ = a2 * invA0;
}

float Biquad::process(float input) {
    // Direct Form II Transposed
    float output = b0_ * input + z1_;
    z1_ = b1_ * input - a1_ * output + z2_;
    z2_ = b2_ * input - a2_ * output;
    return output;
}

void Biquad::reset() {
    z1_ = 0.0f;
    z2_ = 0.0f;
}
