#include "compressor.h"
#include "dsp_common.h"
#include <cmath>
#include <algorithm>

Compressor::Compressor() = default;

void Compressor::updateParams(const CompressorParams& params, float sampleRate) {
    thresholdDb_ = params.thresholdDb.load(std::memory_order_relaxed);
    ratio_ = std::max(1.0f, params.ratio.load(std::memory_order_relaxed));
    volumeLinear_ = params.volume.load(std::memory_order_relaxed);
    makeupGainLinear_ = dsp::dbToLinear(params.makeupGainDb.load(std::memory_order_relaxed));
    preGainLinear_ = dsp::dbToLinear(params.preGainDb.load(std::memory_order_relaxed));
    kneeDb_ = std::max(0.0f, params.kneeDb.load(std::memory_order_relaxed));
    expansionRatio_ = std::max(1.0f, params.expansionRatio.load(std::memory_order_relaxed));
    gateThresholdDb_ = params.gateThresholdDb.load(std::memory_order_relaxed);

    float attackMs = std::max(0.01f, params.attackMs.load(std::memory_order_relaxed));
    float releaseMs = std::max(0.01f, params.releaseMs.load(std::memory_order_relaxed));
    attackCoeff_ = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
    releaseCoeff_ = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));

    float newFreq = params.sidechainFreqHz.load(std::memory_order_relaxed);
    if (newFreq != sidechainFreq_) {
        sidechainFreq_ = newFreq;
        if (newFreq > 20.0f) {
            sidechainEnabled_ = true;
            for (int ch = 0; ch < 2; ch++)
                sidechainFilter_[ch].setParams(Biquad::Type::HighPass, newFreq, 0.0f, 0.707f, sampleRate);
        } else {
            sidechainEnabled_ = false;
            for (int ch = 0; ch < 2; ch++)
                sidechainFilter_[ch].reset();
        }
    }
}

void Compressor::process(float* buffer, int numFrames, int numChannels) {
    int channels = std::min(numChannels, 2);
    float maxCompression = 0.0f;
    float kneeHalf = kneeDb_ * 0.5f;

    for (int frame = 0; frame < numFrames; frame++) {
        for (int ch = 0; ch < numChannels; ch++)
            buffer[frame * numChannels + ch] *= preGainLinear_;

        float peakLevel = 0.0f;
        for (int ch = 0; ch < channels; ch++) {
            int idx = frame * numChannels + ch;
            float sample = buffer[idx];
            if (sidechainEnabled_)
                sample = sidechainFilter_[ch].process(sample);
            float absVal = std::abs(sample);
            if (absVal > peakLevel) peakLevel = absVal;
        }

        float inputDb = dsp::linearToDb(peakLevel);
        if (inputDb > envDb_)
            envDb_ = attackCoeff_ * envDb_ + (1.0f - attackCoeff_) * inputDb;
        else
            envDb_ = releaseCoeff_ * envDb_ + (1.0f - releaseCoeff_) * inputDb;

        float compressionDb = 0.0f;
        float totalReductionDb = 0.0f;

        if (envDb_ <= gateThresholdDb_) {
            totalReductionDb = 96.0f;
        } else {
            float kneeBottom = thresholdDb_ - kneeHalf;
            float kneeTop = thresholdDb_ + kneeHalf;

            if (envDb_ >= kneeTop) {
                float overDb = envDb_ - thresholdDb_;
                compressionDb = overDb * (1.0f - 1.0f / ratio_);
            } else if (kneeDb_ > 0.0f && envDb_ > kneeBottom) {
                float x = envDb_ - kneeBottom;
                compressionDb = (1.0f - 1.0f / ratio_) * (x * x) / (2.0f * kneeDb_);
            }

            totalReductionDb = compressionDb;

            if (compressionDb <= 0.0f && expansionRatio_ > 1.0f && envDb_ < kneeBottom) {
                float underDb = kneeBottom - envDb_;
                totalReductionDb = underDb * (1.0f - 1.0f / expansionRatio_);
            }
        }

        totalReductionDb = std::min(totalReductionDb, 96.0f);
        if (compressionDb > maxCompression) maxCompression = compressionDb;

        float gainLinear = dsp::dbToLinear(-totalReductionDb);
        float totalGain = gainLinear * makeupGainLinear_ * volumeLinear_;
        for (int ch = 0; ch < numChannels; ch++)
            buffer[frame * numChannels + ch] *= totalGain;
    }

    currentGainReductionDb_.store(maxCompression, std::memory_order_relaxed);
}

void Compressor::reset() {
    envDb_ = -96.0f;
    for (int ch = 0; ch < 2; ch++)
        sidechainFilter_[ch].reset();
    currentGainReductionDb_.store(0.0f, std::memory_order_relaxed);
}
