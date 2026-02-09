#pragma once
#include <vector>
#include <atomic>
#include "biquad.h"

struct ReverbParams {
    std::atomic<bool>  enabled{true};
    std::atomic<float> decayTime{0.9f};
    std::atomic<float> hiRatio{0.7f};
    std::atomic<float> diffusion{0.9f};
    std::atomic<float> initialDelay{26.0f};
    std::atomic<float> density{3.0f};
    std::atomic<float> lpfFreq{11000.0f};
    std::atomic<float> hpfFreq{90.0f};
    std::atomic<float> reverbDelay{17.0f};
    std::atomic<float> balance{20.0f};
};

class Reverb {
public:
    Reverb();

    void init(float sampleRate);
    void updateParams(const ReverbParams& params);
    void process(float* buffer, int numFrames, int numChannels);
    void reset();

private:
    static constexpr int NUM_COMBS = 12;
    static constexpr int NUM_INPUT_AP = 4;
    static constexpr int NUM_OUTPUT_AP = 2;
    static constexpr int STEREO_SPREAD = 37;
    static constexpr float INPUT_GAIN = 0.012f;

    struct CombFilter {
        std::vector<float> buffer;
        int size = 0;
        int idx = 0;
        float filterState = 0.0f;

        void init(int sz);
        float process(float input, float feedback, float damping);
        void reset();
    };

    struct AllpassFilter {
        std::vector<float> buffer;
        int size = 0;
        int idx = 0;

        void init(int sz);
        float process(float input, float feedback);
        void reset();
    };

    struct DelayLine {
        std::vector<float> buffer;
        int size = 0;
        int writeIdx = 0;
        int readIdx = 0;

        void init(int maxSamples);
        void setDelay(int samples);
        float process(float input);
        void reset();
    };

    CombFilter combL_[NUM_COMBS];
    CombFilter combR_[NUM_COMBS];

    AllpassFilter inputApL_[NUM_INPUT_AP];
    AllpassFilter inputApR_[NUM_INPUT_AP];
    AllpassFilter outputApL_[NUM_OUTPUT_AP];
    AllpassFilter outputApR_[NUM_OUTPUT_AP];

    DelayLine preDelay_;
    DelayLine lateDelayL_, lateDelayR_;

    Biquad inputHPF_;
    Biquad inputLPF_;

    float combFeedback_[NUM_COMBS] = {};
    float combGain_[NUM_COMBS] = {};
    float combNorm_ = 1.0f;
    float damping_ = 0.3f;
    float diffusionFb_ = 0.5f;
    float wet_ = 0.2f;
    float dry_ = 0.8f;

    float sampleRate_ = 48000.0f;
    bool initialized_ = false;

    float lastDecayTime_ = -1.0f;
    float lastHiRatio_ = -1.0f;
    float lastDiffusion_ = -1.0f;
    float lastDensity_ = -1.0f;
    float lastLpfFreq_ = -1.0f;
    float lastHpfFreq_ = -1.0f;
};
