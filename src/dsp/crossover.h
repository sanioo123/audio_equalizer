#pragma once
#include <atomic>
#include "biquad.h"

struct CrossoverParams {
    std::atomic<bool>  enabled{true};
    std::atomic<bool>  lpfEnabled{false};
    std::atomic<float> lowFreq{30.0f};
    std::atomic<float> highFreq{70.0f};
    std::atomic<int>   hpfSlope{24};
    std::atomic<int>   lpfSlope{24};
    std::atomic<float> subGainDb{6.0f};
};

class Crossover {
public:
    void updateParams(const CrossoverParams& params, float sampleRate);
    void process(float* buffer, int numFrames, int numChannels);
    void reset();

private:
    static constexpr int MAX_STAGES = 4;

    Biquad hpf_[2][MAX_STAGES];
    Biquad lpf_[2][MAX_STAGES];

    float hpfOnePoleState_[2] = {};
    float lpfOnePoleState_[2] = {};
    float hpfOnePoleCoeff_ = 0.0f;
    float lpfOnePoleCoeff_ = 0.0f;

    bool lpfEnabled_ = false;
    int hpfSlope_ = 24;
    int lpfSlope_ = 24;
    int hpfStages_ = 2;
    int lpfStages_ = 2;
    float subGainLinear_ = 1.0f;

    float lastLowFreq_ = 0;
    float lastHighFreq_ = 0;
    int lastHpfSlope_ = 0;
    int lastLpfSlope_ = 0;
    float lastSampleRate_ = 0;
};
