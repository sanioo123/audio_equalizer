#pragma once
#include "biquad.h"
#include "compressor.h"
#include "spectral_analyzer.h"
#include "exciter.h"
#include <vector>
#include <array>
#include <atomic>
#include <thread>
#include <mutex>

struct MultibandBand {
    float lowFreq;
    float highFreq;
    float gain;
    float compression;
    float energy;
    bool enabled = true;
    float manualGain = 0.0f;
};

class MultibandProcessor {
public:
    MultibandProcessor();

    void init(float sampleRate);
    void process(float* buffer, int numFrames, int numChannels, float sampleRate);

    void setEnabled(bool enabled) { enabled_ = enabled; }
    void setAutoBalance(bool enable) { autoBalance_ = enable; }
    void setAutoBalanceSpeed(float speed) { autoBalanceSpeed_ = speed; }
    void setGlobalCompression(float amount) { globalCompression_ = amount; }
    void setOutputGain(float gainDb) { outputGainDb_ = gainDb; }
    void setSubBassBoost(float boostDb) { subBassBoostDb_ = boostDb; }
    void setSubBassRange(float lowFreq, float highFreq);

    int getNumBands() const { return (int)bands_.size(); }
    MultibandBand& getBand(int idx) { return bands_[idx]; }
    const MultibandBand& getBand(int idx) const { return bands_[idx]; }

    void reset();

private:
    struct BandProcessor {
        Biquad lpfL, lpfR;
        Biquad hpfL, hpfR;
        Compressor compressor;
        float currentGain = 1.0f;
        float targetGain = 1.0f;

        BandProcessor() = default;
        BandProcessor(const BandProcessor&) = delete;
        BandProcessor& operator=(const BandProcessor&) = delete;
        BandProcessor(BandProcessor&&) = delete;
        BandProcessor& operator=(BandProcessor&&) = delete;
    };

    void updateFilters();
    void updateAutoBalance();

    static constexpr int NUM_BANDS = 9;
    std::vector<MultibandBand> bands_;
    std::array<BandProcessor, NUM_BANDS> processors_;
    SpectralAnalyzer analyzer_;
    Exciter exciter_;

    float sampleRate_ = 48000.0f;
    bool enabled_ = true;
    bool autoBalance_ = true;
    float autoBalanceSpeed_ = 0.1f;
    float globalCompression_ = 0.5f;
    float outputGainDb_ = 0.0f;
    float outputGainLinear_ = 1.0f;
    float subBassBoostDb_ = 10.0f;
    float subBassLowFreq_ = 30.0f;
    float subBassHighFreq_ = 250.0f;
    bool subBassRangeChanged_ = false;

    bool initialized_ = false;
};
