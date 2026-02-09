#pragma once
#include <atomic>
#include "biquad.h"

static constexpr int MAX_BL_ENTRIES = 4;

struct BandLimiterEntryParams {
    std::atomic<bool>  active{false};
    std::atomic<float> lowFreq{20.0f};
    std::atomic<float> highFreq{70.0f};
    std::atomic<float> limitDb{0.0f};
};

struct BandLimiterParams {
    std::atomic<bool> enabled{false};
    BandLimiterEntryParams entries[MAX_BL_ENTRIES];
};

class BandLimiter {
public:
    void updateParams(const BandLimiterParams& params, float sampleRate);
    void process(float* buffer, int numFrames, int numChannels);
    void reset();

private:
    static constexpr int STAGES = 2;

    struct Entry {
        bool active = false;
        float limitLinear = 1.0f;

        Biquad hpf[2][STAGES];
        Biquad lpf[2][STAGES];

        float envState[2] = {};
        float releaseCoeff = 0.0f;

        float lastLowFreq = 0;
        float lastHighFreq = 0;
    };

    Entry entries_[MAX_BL_ENTRIES];
};
