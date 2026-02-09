#include "dsp_chain.h"
#include <cmath>
#include <algorithm>

DSPChain::DSPChain(SharedParams& params) : params_(params) {}

void DSPChain::updateTone(float sampleRate) {
    const ToneParams& tp = params_.tone;
    bool rateChanged = (sampleRate != lastToneSampleRate_);

    float bFreq = tp.bassFreq.load(std::memory_order_relaxed);
    float bQ = tp.bassQ.load(std::memory_order_relaxed);
    float bGain = tp.bassGainDb.load(std::memory_order_relaxed);

    if (rateChanged || bFreq != lastBassFreq_ || bQ != lastBassQ_ || bGain != lastBassGain_) {
        for (int ch = 0; ch < 2; ch++)
            bassTone_[ch].setParams(Biquad::Type::LowShelf, bFreq, bGain, bQ, sampleRate);
        lastBassFreq_ = bFreq;
        lastBassQ_ = bQ;
        lastBassGain_ = bGain;
    }

    float tFreq = tp.trebleFreq.load(std::memory_order_relaxed);
    float tQ = tp.trebleQ.load(std::memory_order_relaxed);
    float tGain = tp.trebleGainDb.load(std::memory_order_relaxed);

    if (rateChanged || tFreq != lastTrebleFreq_ || tQ != lastTrebleQ_ || tGain != lastTrebleGain_) {
        for (int ch = 0; ch < 2; ch++)
            trebleTone_[ch].setParams(Biquad::Type::HighShelf, tFreq, tGain, tQ, sampleRate);
        lastTrebleFreq_ = tFreq;
        lastTrebleQ_ = tQ;
        lastTrebleGain_ = tGain;
    }

    lastToneSampleRate_ = sampleRate;
}

void DSPChain::process(float* buffer, int numFrames, int numChannels, float sampleRate) {
    if (params_.bypassAll.load(std::memory_order_relaxed))
        return;

    if (params_.eq.enabled.load(std::memory_order_relaxed)) {
        equalizer_.updateParams(params_.eq, sampleRate);
        equalizer_.process(buffer, numFrames, numChannels);
    }

    updateTone(sampleRate);
    bool bassOn = params_.tone.bassEnabled.load(std::memory_order_relaxed);
    bool trebleOn = params_.tone.trebleEnabled.load(std::memory_order_relaxed);

    if (bassOn || trebleOn) {
        int channels = std::min(numChannels, 2);
        for (int frame = 0; frame < numFrames; frame++) {
            for (int ch = 0; ch < channels; ch++) {
                int idx = frame * numChannels + ch;
                float sample = buffer[idx];
                if (bassOn)   sample = bassTone_[ch].process(sample);
                if (trebleOn) sample = trebleTone_[ch].process(sample);
                buffer[idx] = sample;
            }
        }
    }

    if (params_.crossover.enabled.load(std::memory_order_relaxed)) {
        crossover_.updateParams(params_.crossover, sampleRate);
        crossover_.process(buffer, numFrames, numChannels);
    }

    if (params_.bandLimiter.enabled.load(std::memory_order_relaxed)) {
        bandLimiter_.updateParams(params_.bandLimiter, sampleRate);
        bandLimiter_.process(buffer, numFrames, numChannels);
    }

    if (params_.multiband.enabled.load(std::memory_order_relaxed)) {
        multiband_.setAutoBalance(params_.multiband.autoBalance.load(std::memory_order_relaxed));
        multiband_.setAutoBalanceSpeed(params_.multiband.autoBalanceSpeed.load(std::memory_order_relaxed));
        multiband_.setGlobalCompression(params_.multiband.compression.load(std::memory_order_relaxed));
        multiband_.setOutputGain(params_.multiband.outputGain.load(std::memory_order_relaxed));
        multiband_.setSubBassBoost(params_.multiband.subBassBoost.load(std::memory_order_relaxed));
        multiband_.setSubBassRange(
            params_.multiband.subBassLowFreq.load(std::memory_order_relaxed),
            params_.multiband.subBassHighFreq.load(std::memory_order_relaxed)
        );
        multiband_.process(buffer, numFrames, numChannels, sampleRate);
    }

    if (params_.compressor.enabled.load(std::memory_order_relaxed)) {
        compressor_.updateParams(params_.compressor, sampleRate);
        compressor_.process(buffer, numFrames, numChannels);
    }

    if (params_.reverb.enabled.load(std::memory_order_relaxed)) {
        if (!reverbInitialized_) {
            reverb_.init(sampleRate);
            reverbInitialized_ = true;
        }
        reverb_.updateParams(params_.reverb);
        reverb_.process(buffer, numFrames, numChannels);
    }

    {
        constexpr float threshold = 0.9f;
        constexpr float headroom = 1.0f - threshold;
        int totalSamples = numFrames * numChannels;
        for (int i = 0; i < totalSamples; i++) {
            float x = buffer[i];
            float ax = std::abs(x);
            if (ax > threshold) {
                float sign = (x >= 0.0f) ? 1.0f : -1.0f;
                float over = (ax - threshold) / headroom;
                buffer[i] = sign * (threshold + headroom * std::tanh(over));
            }
        }
    }
}
