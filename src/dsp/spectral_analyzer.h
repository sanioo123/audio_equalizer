#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

class SpectralAnalyzer {
public:
    SpectralAnalyzer();

    void init(float sampleRate, int fftSize = 4096);
    void process(const float* buffer, int numFrames, int numChannels);

    float getBandEnergy(float lowFreq, float highFreq) const;
    float getAverageEnergy() const;

    void reset();

private:
    struct FrequencyBand {
        float lowFreq;
        float highFreq;
        float energy;
        float targetEnergy;
    };

    void performFFT(const float* input, int size);
    void updateBandEnergies();

    std::vector<float> fftBuffer_;
    std::vector<float> magnitudes_;
    std::vector<float> window_;
    std::vector<FrequencyBand> bands_;

    float sampleRate_ = 48000.0f;
    int fftSize_ = 4096;
    int writePos_ = 0;
    float avgEnergy_ = 0.0f;
};
