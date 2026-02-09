#include "multiband_processor.h"
#include "dsp_common.h"
#include <cmath>
#include <algorithm>
#include <cstring>

MultibandProcessor::MultibandProcessor() {
    bands_.push_back({30.0f, 250.0f, 1.0f, 0.2f, 0.0f});
    bands_.push_back({250.0f, 500.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({500.0f, 1000.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({1000.0f, 2000.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({2000.0f, 4000.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({4000.0f, 8000.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({8000.0f, 12000.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({12000.0f, 16000.0f, 1.0f, 0.3f, 0.0f});
    bands_.push_back({16000.0f, 20000.0f, 1.0f, 0.3f, 0.0f});

    init(48000.0f);
}

void MultibandProcessor::init(float sampleRate) {
    sampleRate_ = sampleRate;

    analyzer_.init(sampleRate);
    exciter_.init(sampleRate);
    exciter_.setAmount(0.3f);
    exciter_.setFrequency(4000.0f);

    for (auto& proc : processors_) {
        proc.currentGain = 1.0f;
        proc.targetGain = 1.0f;
    }

    updateFilters();
    initialized_ = true;
}

void MultibandProcessor::setSubBassRange(float lowFreq, float highFreq) {
    lowFreq = std::max(20.0f, std::min(lowFreq, 100.0f));
    highFreq = std::max(100.0f, std::min(highFreq, 500.0f));

    if (lowFreq >= highFreq) {
        lowFreq = highFreq - 10.0f;
    }

    if (subBassLowFreq_ != lowFreq || subBassHighFreq_ != highFreq) {
        subBassLowFreq_ = lowFreq;
        subBassHighFreq_ = highFreq;
        subBassRangeChanged_ = true;

        if (!bands_.empty()) {
            bands_[0].lowFreq = lowFreq;
            bands_[0].highFreq = highFreq;
        }
    }
}

void MultibandProcessor::updateFilters() {
    const float Q = 0.707f;

    for (size_t i = 0; i < bands_.size(); i++) {
        auto& band = bands_[i];
        auto& proc = processors_[i];

        proc.hpfL.setParams(Biquad::Type::HighPass, band.lowFreq, 0.0f, Q, sampleRate_);
        proc.hpfR.setParams(Biquad::Type::HighPass, band.lowFreq, 0.0f, Q, sampleRate_);
        proc.lpfL.setParams(Biquad::Type::LowPass, band.highFreq, 0.0f, Q, sampleRate_);
        proc.lpfR.setParams(Biquad::Type::LowPass, band.highFreq, 0.0f, Q, sampleRate_);
    }

    subBassRangeChanged_ = false;
}

void MultibandProcessor::updateAutoBalance() {
    if (!autoBalance_) return;

    float avgEnergy = analyzer_.getAverageEnergy();
    if (avgEnergy < 0.0001f) return;

    for (size_t i = 0; i < bands_.size(); i++) {
        auto& band = bands_[i];
        auto& proc = processors_[i];

        float energy = analyzer_.getBandEnergy(band.lowFreq, band.highFreq);
        band.energy = energy;

        float energyRatio = energy / (avgEnergy + 0.0001f);
        float targetGain = 1.0f / std::sqrt(energyRatio + 0.5f);
        targetGain = std::max(0.5f, std::min(targetGain, 2.0f));

        float manualGainLinear = dsp::dbToLinear(band.manualGain);
        proc.targetGain = targetGain * manualGainLinear;

        float alpha = autoBalanceSpeed_ * 0.01f;
        proc.currentGain = proc.currentGain * (1.0f - alpha) + proc.targetGain * alpha;
    }
}

void MultibandProcessor::process(float* buffer, int numFrames, int numChannels, float sampleRate) {
    if (!enabled_ || !initialized_) return;

    if (sampleRate != sampleRate_) {
        init(sampleRate);
    }

    if (subBassRangeChanged_) {
        updateFilters();
    }

    analyzer_.process(buffer, numFrames, numChannels);
    updateAutoBalance();

    std::array<std::vector<float>, NUM_BANDS> bandBuffers;
    for (int b = 0; b < NUM_BANDS; b++) {
        bandBuffers[b].resize(numFrames * numChannels, 0.0f);
    }

    auto processBand = [&](int b) {
        if (!bands_[b].enabled) return;

        auto& proc = processors_[b];

        std::memcpy(bandBuffers[b].data(), buffer, numFrames * numChannels * sizeof(float));

        for (int i = 0; i < numFrames; i++) {
            float L = bandBuffers[b][i * numChannels];
            L = proc.hpfL.process(L);
            L = proc.lpfL.process(L);
            bandBuffers[b][i * numChannels] = L;

            if (numChannels > 1) {
                float R = bandBuffers[b][i * numChannels + 1];
                R = proc.hpfR.process(R);
                R = proc.lpfR.process(R);
                bandBuffers[b][i * numChannels + 1] = R;
            }
        }

        CompressorParams compParams;
        float ratio = 1.0f + (globalCompression_ * 3.0f);
        compParams.ratio.store(ratio, std::memory_order_relaxed);
        compParams.thresholdDb.store(-12.0f, std::memory_order_relaxed);
        compParams.attackMs.store(5.0f, std::memory_order_relaxed);
        compParams.releaseMs.store(50.0f, std::memory_order_relaxed);
        compParams.kneeDb.store(3.0f, std::memory_order_relaxed);
        compParams.enabled.store(globalCompression_ > 0.01f, std::memory_order_relaxed);

        proc.compressor.updateParams(compParams, sampleRate);
        proc.compressor.process(bandBuffers[b].data(), numFrames, numChannels);

        float gain = proc.currentGain;

        if (b == 0) {
            gain *= dsp::dbToLinear(subBassBoostDb_);
        }

        for (int i = 0; i < numFrames * numChannels; i++) {
            bandBuffers[b][i] *= gain;
        }
    };

    std::thread t1([&]() { for (int b = 0; b < 3; b++) processBand(b); });
    std::thread t2([&]() { for (int b = 3; b < 6; b++) processBand(b); });
    std::thread t3([&]() { for (int b = 6; b < 9; b++) processBand(b); });

    t1.join();
    t2.join();
    t3.join();

    std::memset(buffer, 0, numFrames * numChannels * sizeof(float));
    for (int b = 0; b < (int)bands_.size(); b++) {
        if (!bands_[b].enabled) continue;

        for (int i = 0; i < numFrames * numChannels; i++) {
            buffer[i] += bandBuffers[b][i];
        }
    }

    exciter_.process(buffer, numFrames, numChannels);

    outputGainLinear_ = dsp::dbToLinear(outputGainDb_);
    for (int i = 0; i < numFrames * numChannels; i++) {
        buffer[i] *= outputGainLinear_;
    }
}

void MultibandProcessor::reset() {
    for (auto& proc : processors_) {
        proc.hpfL.reset();
        proc.hpfR.reset();
        proc.lpfL.reset();
        proc.lpfR.reset();
        proc.compressor.reset();
        proc.currentGain = 1.0f;
        proc.targetGain = 1.0f;
    }
    analyzer_.reset();
    exciter_.reset();
}
