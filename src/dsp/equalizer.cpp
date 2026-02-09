#include "equalizer.h"
#include "dsp_common.h"
#include <cmath>

Biquad::Type Equalizer::mapFilterType(int configType) {
    switch (configType) {
        case 1: return Biquad::Type::HighShelf;
        case 2: return Biquad::Type::LowShelf;
        case 3: return Biquad::Type::PeakingEQ;
        case 4: return Biquad::Type::BandPass;
        case 5: return Biquad::Type::HighPass;
        case 6: return Biquad::Type::LowPass;
        default: return Biquad::Type::PeakingEQ;
    }
}

void Equalizer::updateParams(const EQParams& params, float sampleRate) {
    int nBands = params.numBands();
    bool rateChanged = (sampleRate != lastSampleRate_);

    if (nBands != numBands_) {
        filtersL_.resize(nBands);
        filtersR_.resize(nBands);
        lastGainDb_.resize(nBands, -999.0f);
        numBands_ = nBands;
        initialized_ = false;
    }

    float preampDb = params.preamp.load(std::memory_order_relaxed);
    preampLinear_ = dsp::dbToLinear(preampDb);

    for (int band = 0; band < nBands; band++) {
        const BandParam& bp = params.bands[band];
        float gainDb = bp.gainDb.load(std::memory_order_relaxed);

        if (!initialized_ || rateChanged || gainDb != lastGainDb_[band]) {
            Biquad::Type bqType = mapFilterType(bp.type);
            filtersL_[band].setParams(bqType, bp.freq, gainDb, bp.q, sampleRate);
            filtersR_[band].setParams(bqType, bp.freq, gainDb, bp.q, sampleRate);
            lastGainDb_[band] = gainDb;
        }
    }

    lastSampleRate_ = sampleRate;
    initialized_ = true;
}

void Equalizer::process(float* buffer, int numFrames, int numChannels) {
    int channels = (numChannels > 2) ? 2 : numChannels;

    for (int frame = 0; frame < numFrames; frame++) {
        for (int ch = 0; ch < channels; ch++) {
            int idx = frame * numChannels + ch;
            float sample = buffer[idx] * preampLinear_;

            auto& filters = (ch == 0) ? filtersL_ : filtersR_;
            for (int band = 0; band < numBands_; band++) {
                sample = filters[band].process(sample);
            }

            buffer[idx] = sample;
        }
    }
}

void Equalizer::reset() {
    for (auto& f : filtersL_) f.reset();
    for (auto& f : filtersR_) f.reset();
    initialized_ = false;
}
