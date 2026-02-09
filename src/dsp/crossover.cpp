#include "crossover.h"
#include "dsp_common.h"
#include <cmath>
#include <algorithm>

static int slopeToStages(int slope) {
    switch (slope) {
        case 6:  return 0;
        case 12: return 1;
        case 24: return 2;
        case 48: return 4;
        default: return 2;
    }
}

void Crossover::updateParams(const CrossoverParams& params, float sampleRate) {
    float lowFreq = params.lowFreq.load(std::memory_order_relaxed);
    float highFreq = params.highFreq.load(std::memory_order_relaxed);
    int hpfSlope = params.hpfSlope.load(std::memory_order_relaxed);
    int lpfSlope = params.lpfSlope.load(std::memory_order_relaxed);
    float subGainDb = params.subGainDb.load(std::memory_order_relaxed);

    subGainLinear_ = dsp::dbToLinear(subGainDb);
    lpfEnabled_ = params.lpfEnabled.load(std::memory_order_relaxed);
    hpfSlope_ = hpfSlope;
    lpfSlope_ = lpfSlope;

    bool needsUpdate = (lowFreq != lastLowFreq_ || highFreq != lastHighFreq_ ||
                        hpfSlope != lastHpfSlope_ || lpfSlope != lastLpfSlope_ ||
                        sampleRate != lastSampleRate_);

    if (!needsUpdate) return;

    lastLowFreq_ = lowFreq;
    lastHighFreq_ = highFreq;
    lastHpfSlope_ = hpfSlope;
    lastLpfSlope_ = lpfSlope;
    lastSampleRate_ = sampleRate;

    hpfStages_ = slopeToStages(hpfSlope);
    if (hpfSlope == 6) {
        hpfOnePoleCoeff_ = 1.0f - std::exp(-2.0f * dsp::PI * lowFreq / sampleRate);
    } else {
        for (int ch = 0; ch < 2; ch++)
            for (int s = 0; s < hpfStages_; s++)
                hpf_[ch][s].setParams(Biquad::Type::HighPass, lowFreq, 0.0f, 0.707f, sampleRate);
    }

    lpfStages_ = slopeToStages(lpfSlope);
    if (lpfSlope == 6) {
        lpfOnePoleCoeff_ = 1.0f - std::exp(-2.0f * dsp::PI * highFreq / sampleRate);
    } else {
        for (int ch = 0; ch < 2; ch++)
            for (int s = 0; s < lpfStages_; s++)
                lpf_[ch][s].setParams(Biquad::Type::LowPass, highFreq, 0.0f, 0.707f, sampleRate);
    }
}

void Crossover::process(float* buffer, int numFrames, int numChannels) {
    int channels = std::min(numChannels, 2);

    float extraGain = subGainLinear_ - 1.0f;
    if (std::abs(extraGain) < 0.001f) return;

    for (int frame = 0; frame < numFrames; frame++) {
        for (int ch = 0; ch < channels; ch++) {
            int idx = frame * numChannels + ch;
            float original = buffer[idx];

            float hpfOut;
            if (hpfSlope_ == 6) {
                hpfOnePoleState_[ch] += hpfOnePoleCoeff_ * (original - hpfOnePoleState_[ch]);
                hpfOut = original - hpfOnePoleState_[ch];
            } else {
                hpfOut = original;
                for (int s = 0; s < hpfStages_; s++)
                    hpfOut = hpf_[ch][s].process(hpfOut);
            }

            float sub = original - hpfOut;

            if (lpfEnabled_) {
                if (lpfSlope_ == 6) {
                    lpfOnePoleState_[ch] += lpfOnePoleCoeff_ * (sub - lpfOnePoleState_[ch]);
                    sub = lpfOnePoleState_[ch];
                } else {
                    for (int s = 0; s < lpfStages_; s++)
                        sub = lpf_[ch][s].process(sub);
                }
            }

            buffer[idx] = original + sub * extraGain;
        }
    }
}

void Crossover::reset() {
    for (int ch = 0; ch < 2; ch++) {
        for (int s = 0; s < MAX_STAGES; s++) {
            hpf_[ch][s].reset();
            lpf_[ch][s].reset();
        }
        hpfOnePoleState_[ch] = 0.0f;
        lpfOnePoleState_[ch] = 0.0f;
    }
}
