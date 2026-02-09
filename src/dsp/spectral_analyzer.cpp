#include "spectral_analyzer.h"
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpectralAnalyzer::SpectralAnalyzer() {
    init(48000.0f);
}

void SpectralAnalyzer::init(float sampleRate, int fftSize) {
    sampleRate_ = sampleRate;
    fftSize_ = fftSize;

    fftBuffer_.resize(fftSize, 0.0f);
    magnitudes_.resize(fftSize / 2, 0.0f);
    window_.resize(fftSize);

    for (int i = 0; i < fftSize; i++) {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1)));
    }

    bands_.clear();
    bands_.push_back({30.0f, 250.0f, 0.0f, 1.0f});
    bands_.push_back({250.0f, 500.0f, 0.0f, 1.0f});
    bands_.push_back({500.0f, 1000.0f, 0.0f, 1.0f});
    bands_.push_back({1000.0f, 2000.0f, 0.0f, 1.0f});
    bands_.push_back({2000.0f, 4000.0f, 0.0f, 1.0f});
    bands_.push_back({4000.0f, 8000.0f, 0.0f, 1.0f});
    bands_.push_back({8000.0f, 12000.0f, 0.0f, 1.0f});
    bands_.push_back({12000.0f, 16000.0f, 0.0f, 1.0f});
    bands_.push_back({16000.0f, 20000.0f, 0.0f, 1.0f});

    writePos_ = 0;
}

void SpectralAnalyzer::process(const float* buffer, int numFrames, int numChannels) {
    for (int i = 0; i < numFrames; i++) {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ch++) {
            sample += buffer[i * numChannels + ch];
        }
        sample /= numChannels;

        fftBuffer_[writePos_] = sample;
        writePos_ = (writePos_ + 1) % fftSize_;

        if ((writePos_ % (fftSize_ / 4)) == 0) {
            performFFT(fftBuffer_.data(), fftSize_);
            updateBandEnergies();
        }
    }
}

void SpectralAnalyzer::performFFT(const float* input, int size) {
    int halfSize = size / 2;

    for (int k = 0; k < halfSize; k += 16) {
        float real = 0.0f;
        float imag = 0.0f;

        float w = 2.0f * M_PI * k / size;

        for (int n = 0; n < size; n += 8) {
            float windowed = input[n] * window_[n];
            real += windowed * std::cos(w * n);
            imag += windowed * std::sin(w * n);
        }

        magnitudes_[k] = std::sqrt(real * real + imag * imag) / (size / 8);

        if (k + 16 < halfSize) {
            for (int j = 1; j < 16; j++) {
                magnitudes_[k + j] = magnitudes_[k];
            }
        }
    }
}

void SpectralAnalyzer::updateBandEnergies() {
    float binWidth = sampleRate_ / fftSize_;

    for (auto& band : bands_) {
        int startBin = (int)(band.lowFreq / binWidth);
        int endBin = (int)(band.highFreq / binWidth);

        startBin = std::max(0, std::min(startBin, (int)magnitudes_.size() - 1));
        endBin = std::max(0, std::min(endBin, (int)magnitudes_.size() - 1));

        float sum = 0.0f;
        int count = 0;
        for (int i = startBin; i <= endBin; i++) {
            sum += magnitudes_[i];
            count++;
        }

        if (count > 0) {
            float newEnergy = sum / count;
            band.energy = band.energy * 0.8f + newEnergy * 0.2f;
        }
    }

    float totalEnergy = 0.0f;
    for (const auto& band : bands_) {
        totalEnergy += band.energy;
    }
    avgEnergy_ = totalEnergy / bands_.size();
}

float SpectralAnalyzer::getBandEnergy(float lowFreq, float highFreq) const {
    for (const auto& band : bands_) {
        if (band.lowFreq <= lowFreq && band.highFreq >= highFreq) {
            return band.energy;
        }
    }
    return 0.0f;
}

float SpectralAnalyzer::getAverageEnergy() const {
    return avgEnergy_;
}

void SpectralAnalyzer::reset() {
    std::fill(fftBuffer_.begin(), fftBuffer_.end(), 0.0f);
    std::fill(magnitudes_.begin(), magnitudes_.end(), 0.0f);
    for (auto& band : bands_) {
        band.energy = 0.0f;
    }
    avgEnergy_ = 0.0f;
    writePos_ = 0;
}
