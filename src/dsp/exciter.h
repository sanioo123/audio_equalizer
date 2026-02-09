#pragma once
#include "biquad.h"

class Exciter {
public:
    Exciter();

    void init(float sampleRate);
    void process(float* buffer, int numFrames, int numChannels);

    void setAmount(float amount) { amount_ = amount; }
    void setFrequency(float freq);
    void setHarmonics(int order) { harmonicOrder_ = order; }

    void reset();

private:
    float processSample(float input);

    Biquad hpfL_, hpfR_;
    float amount_ = 0.3f;
    float frequency_ = 4000.0f;
    float sampleRate_ = 48000.0f;
    int harmonicOrder_ = 2;
};
