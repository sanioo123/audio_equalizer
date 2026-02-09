#pragma once
#include "biquad.h"
#include "common/params.h"
#include <atomic>

class Compressor {
public:
    Compressor();

    void updateParams(const CompressorParams& params, float sampleRate);
    void process(float* buffer, int numFrames, int numChannels);
    void reset();

    float getGainReduction() const {
        return currentGainReductionDb_.load(std::memory_order_relaxed);
    }

private:
    float envDb_ = -96.0f;

    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;

    float thresholdDb_ = -20.0f;
    float ratio_ = 4.0f;
    float makeupGainLinear_ = 1.0f;
    float volumeLinear_ = 1.0f;
    float preGainLinear_ = 1.0f;
    float kneeDb_ = 0.0f;
    float expansionRatio_ = 1.0f;
    float gateThresholdDb_ = -90.0f;

    Biquad sidechainFilter_[2];
    float sidechainFreq_ = 0.0f;
    bool sidechainEnabled_ = false;

    std::atomic<float> currentGainReductionDb_{0.0f};
};
