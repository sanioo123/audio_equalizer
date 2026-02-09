#pragma once

class Biquad {
public:
    enum class Type {
        PeakingEQ,
        HighPass,
        LowPass,
        LowShelf,
        HighShelf,
        BandPass
    };

    Biquad() = default;

    void setParams(Type type, float freqHz, float gainDb, float Q, float sampleRate);
    float process(float input);
    void reset();

private:
    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    float z1_ = 0.0f, z2_ = 0.0f;
};
