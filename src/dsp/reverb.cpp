#include "reverb.h"
#include <cmath>
#include <cstring>
#include <algorithm>

// Prime delay lengths tuned for 48kHz, spread across 23-47ms for rich, dense tail
static const int COMB_TUNING_48K[12] = {
    1117, 1201, 1301, 1399, 1499, 1601,
    1709, 1811, 1907, 2011, 2113, 2239
};

// Input diffusion allpass lengths (3.4-10.5ms at 48kHz)
static const int INPUT_AP_TUNING_48K[4] = { 163, 271, 383, 503 };

// Output decorrelation allpass lengths
static const int OUTPUT_AP_TUNING_48K[2] = { 131, 197 };

void Reverb::CombFilter::init(int sz) {
    size = sz;
    buffer.assign(sz, 0.0f);
    idx = 0;
    filterState = 0.0f;
}

float Reverb::CombFilter::process(float input, float feedback, float damping) {
    float output = buffer[idx];
    filterState = output + damping * (filterState - output);
    buffer[idx] = input + filterState * feedback;
    if (++idx >= size) idx = 0;
    return output;
}

void Reverb::CombFilter::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    filterState = 0.0f;
    idx = 0;
}

void Reverb::AllpassFilter::init(int sz) {
    size = sz;
    buffer.assign(sz, 0.0f);
    idx = 0;
}

float Reverb::AllpassFilter::process(float input, float feedback) {
    float bufOut = buffer[idx];
    buffer[idx] = input + bufOut * feedback;
    if (++idx >= size) idx = 0;
    return bufOut - input * feedback;
}

void Reverb::AllpassFilter::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    idx = 0;
}

void Reverb::DelayLine::init(int maxSamples) {
    size = maxSamples;
    buffer.assign(maxSamples, 0.0f);
    writeIdx = 0;
    readIdx = 0;
}

void Reverb::DelayLine::setDelay(int samples) {
    if (samples >= size) samples = size - 1;
    if (samples < 0) samples = 0;
    readIdx = writeIdx - samples;
    if (readIdx < 0) readIdx += size;
}

float Reverb::DelayLine::process(float input) {
    buffer[writeIdx] = input;
    float output = buffer[readIdx];
    if (++writeIdx >= size) writeIdx = 0;
    if (++readIdx >= size) readIdx = 0;
    return output;
}

void Reverb::DelayLine::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
}

Reverb::Reverb() = default;

void Reverb::init(float sampleRate) {
    sampleRate_ = sampleRate;
    float scale = sampleRate / 48000.0f;

    for (int i = 0; i < NUM_COMBS; i++) {
        int sz = std::max(1, (int)(COMB_TUNING_48K[i] * scale));
        combL_[i].init(sz);
        combR_[i].init(sz + STEREO_SPREAD);
    }

    for (int i = 0; i < NUM_INPUT_AP; i++) {
        int sz = std::max(1, (int)(INPUT_AP_TUNING_48K[i] * scale));
        inputApL_[i].init(sz);
        inputApR_[i].init(sz + 13);
    }

    for (int i = 0; i < NUM_OUTPUT_AP; i++) {
        int sz = std::max(1, (int)(OUTPUT_AP_TUNING_48K[i] * scale));
        outputApL_[i].init(sz);
        outputApR_[i].init(sz + 11);
    }

    int maxDelay = std::max(1, (int)(sampleRate * 0.15f));
    preDelay_.init(maxDelay);
    lateDelayL_.init(maxDelay);
    lateDelayR_.init(maxDelay);

    inputHPF_.setParams(Biquad::Type::HighPass, 90.0f, 0.0f, 0.707f, sampleRate);
    inputLPF_.setParams(Biquad::Type::LowPass, 11000.0f, 0.0f, 0.707f, sampleRate);

    for (int i = 0; i < NUM_COMBS; i++) {
        combFeedback_[i] = 0.0f;
        combGain_[i] = 1.0f;
    }
    combNorm_ = 1.0f / std::sqrt((float)NUM_COMBS);

    initialized_ = true;
}

void Reverb::updateParams(const ReverbParams& params) {
    float decayTime  = params.decayTime.load(std::memory_order_relaxed);
    float hiRatio    = params.hiRatio.load(std::memory_order_relaxed);
    float diffusion  = params.diffusion.load(std::memory_order_relaxed);
    float initDelay  = params.initialDelay.load(std::memory_order_relaxed);
    float density    = params.density.load(std::memory_order_relaxed);
    float lpfFreq    = params.lpfFreq.load(std::memory_order_relaxed);
    float hpfFreq    = params.hpfFreq.load(std::memory_order_relaxed);
    float revDelay   = params.reverbDelay.load(std::memory_order_relaxed);
    float balance    = params.balance.load(std::memory_order_relaxed);

    if (decayTime != lastDecayTime_) {
        float rt60 = std::max(0.1f, decayTime);
        for (int i = 0; i < NUM_COMBS; i++) {
            float delaySec = (float)combL_[i].size / sampleRate_;
            combFeedback_[i] = std::pow(10.0f, -3.0f * delaySec / rt60);
        }
        lastDecayTime_ = decayTime;
    }

    if (hiRatio != lastHiRatio_) {
        float hr = std::max(0.0f, std::min(1.0f, hiRatio));
        damping_ = 1.0f - hr;
        lastHiRatio_ = hiRatio;
    }

    if (diffusion != lastDiffusion_) {
        float d = std::max(0.0f, std::min(1.0f, diffusion));
        diffusionFb_ = d * 0.75f;
        lastDiffusion_ = diffusion;
    }

    if (density != lastDensity_) {
        float d = std::max(0.0f, std::min(12.0f, density)) / 12.0f;
        float sumSq = 0.0f;
        for (int i = 0; i < NUM_COMBS; i++) {
            if (i < 4)
                combGain_[i] = 1.0f;
            else if (i < 8)
                combGain_[i] = 0.3f + 0.7f * d;
            else
                combGain_[i] = 0.1f + 0.9f * d * d;
            sumSq += combGain_[i] * combGain_[i];
        }
        combNorm_ = 1.0f / std::sqrt(sumSq);
        lastDensity_ = density;
    }

    if (lpfFreq != lastLpfFreq_) {
        float freq = std::max(1000.0f, std::min(20000.0f, lpfFreq));
        inputLPF_.setParams(Biquad::Type::LowPass, freq, 0.0f, 0.707f, sampleRate_);
        lastLpfFreq_ = lpfFreq;
    }

    if (hpfFreq != lastHpfFreq_) {
        float freq = std::max(20.0f, std::min(500.0f, hpfFreq));
        inputHPF_.setParams(Biquad::Type::HighPass, freq, 0.0f, 0.707f, sampleRate_);
        lastHpfFreq_ = hpfFreq;
    }

    int preDelaySamples = (int)(initDelay * 0.001f * sampleRate_);
    preDelay_.setDelay(preDelaySamples);

    int lateDelaySamples = (int)(revDelay * 0.001f * sampleRate_);
    lateDelayL_.setDelay(lateDelaySamples);
    lateDelayR_.setDelay(lateDelaySamples);

    float bal = std::max(0.0f, std::min(100.0f, balance)) / 100.0f;
    wet_ = bal;
    dry_ = 1.0f - bal * 0.5f;
}

void Reverb::process(float* buffer, int numFrames, int numChannels) {
    if (!initialized_) return;

    int channels = std::min(numChannels, 2);

    for (int frame = 0; frame < numFrames; frame++) {
        int idxL = frame * numChannels;
        int idxR = (channels > 1) ? (frame * numChannels + 1) : idxL;

        float inputL = buffer[idxL];
        float inputR = buffer[idxR];

        float mono = (inputL + inputR) * 0.5f;

        float filtered = inputHPF_.process(mono);
        filtered = inputLPF_.process(filtered);

        float pd = preDelay_.process(filtered) * INPUT_GAIN;

        float diffL = pd;
        float diffR = pd;
        for (int i = 0; i < NUM_INPUT_AP; i++) {
            diffL = inputApL_[i].process(diffL, diffusionFb_);
            diffR = inputApR_[i].process(diffR, diffusionFb_);
        }

        float delL = lateDelayL_.process(diffL);
        float delR = lateDelayR_.process(diffR);

        float outL = 0.0f, outR = 0.0f;
        for (int i = 0; i < NUM_COMBS; i++) {
            float g = combGain_[i];
            outL += combL_[i].process(delL, combFeedback_[i], damping_) * g;
            outR += combR_[i].process(delR, combFeedback_[i], damping_) * g;
        }

        outL *= combNorm_;
        outR *= combNorm_;

        for (int i = 0; i < NUM_OUTPUT_AP; i++) {
            outL = outputApL_[i].process(outL, diffusionFb_ * 0.8f);
            outR = outputApR_[i].process(outR, diffusionFb_ * 0.8f);
        }

        buffer[idxL] = inputL * dry_ + outL * wet_;
        if (channels > 1) {
            buffer[idxR] = inputR * dry_ + outR * wet_;
        }
    }
}

void Reverb::reset() {
    for (int i = 0; i < NUM_COMBS; i++) {
        combL_[i].reset();
        combR_[i].reset();
    }
    for (int i = 0; i < NUM_INPUT_AP; i++) {
        inputApL_[i].reset();
        inputApR_[i].reset();
    }
    for (int i = 0; i < NUM_OUTPUT_AP; i++) {
        outputApL_[i].reset();
        outputApR_[i].reset();
    }
    preDelay_.reset();
    lateDelayL_.reset();
    lateDelayR_.reset();
    inputHPF_.reset();
    inputLPF_.reset();
}
