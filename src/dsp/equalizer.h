#pragma once
#include "biquad.h"
#include "common/params.h"
#include <vector>

class Equalizer {
public:
    Equalizer() = default;

    void updateParams(const EQParams& params, float sampleRate);
    void process(float* buffer, int numFrames, int numChannels);
    void reset();

private:
    static Biquad::Type mapFilterType(int configType);

    std::vector<Biquad> filtersL_;
    std::vector<Biquad> filtersR_;
    std::vector<float> lastGainDb_;
    float lastSampleRate_ = 0.0f;
    float preampLinear_ = 1.0f;
    int numBands_ = 0;
    bool initialized_ = false;
};
