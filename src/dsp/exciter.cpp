#include "exciter.h"
#include <cmath>
#include <algorithm>

Exciter::Exciter() {
    init(48000.0f);
}

void Exciter::init(float sampleRate) {
    sampleRate_ = sampleRate;
    setFrequency(frequency_);
    reset();
}

void Exciter::setFrequency(float freq) {
    frequency_ = std::max(1000.0f, std::min(freq, 16000.0f));
    hpfL_.setParams(Biquad::Type::HighPass, frequency_, 0.0f, 0.707f, sampleRate_);
    hpfR_.setParams(Biquad::Type::HighPass, frequency_, 0.0f, 0.707f, sampleRate_);
}

void Exciter::process(float* buffer, int numFrames, int numChannels) {
    if (amount_ < 0.001f) return;

    for (int i = 0; i < numFrames; i++) {
        float dryL = buffer[i * numChannels];
        float highL = hpfL_.process(dryL);

        float excitedL = highL;
        if (harmonicOrder_ >= 2) {
            excitedL = std::tanh(highL * 2.0f) * 0.5f;
        }
        if (harmonicOrder_ >= 3) {
            float squared = highL * highL;
            excitedL += squared * highL * 0.3f;
        }

        buffer[i * numChannels] = dryL + excitedL * amount_;

        if (numChannels > 1) {
            float dryR = buffer[i * numChannels + 1];
            float highR = hpfR_.process(dryR);

            float excitedR = highR;
            if (harmonicOrder_ >= 2) {
                excitedR = std::tanh(highR * 2.0f) * 0.5f;
            }
            if (harmonicOrder_ >= 3) {
                float squared = highR * highR;
                excitedR += squared * highR * 0.3f;
            }

            buffer[i * numChannels + 1] = dryR + excitedR * amount_;
        }
    }
}

void Exciter::reset() {
    hpfL_.reset();
    hpfR_.reset();
}
