#include "band_limiter.h"
#include "dsp_common.h"
#include <cmath>
#include <algorithm>

void BandLimiter::updateParams(const BandLimiterParams& params, float sampleRate) {
    for (int e = 0; e < MAX_BL_ENTRIES; e++) {
        Entry& entry = entries_[e];
        entry.active = params.entries[e].active.load(std::memory_order_relaxed);
        if (!entry.active) continue;

        float lowFreq = params.entries[e].lowFreq.load(std::memory_order_relaxed);
        float highFreq = params.entries[e].highFreq.load(std::memory_order_relaxed);
        float limitDb = params.entries[e].limitDb.load(std::memory_order_relaxed);

        entry.limitLinear = dsp::dbToLinear(limitDb);
        entry.releaseCoeff = std::exp(-1.0f / (0.05f * sampleRate));

        if (lowFreq == entry.lastLowFreq && highFreq == entry.lastHighFreq)
            continue;

        entry.lastLowFreq = lowFreq;
        entry.lastHighFreq = highFreq;

        for (int ch = 0; ch < 2; ch++) {
            for (int s = 0; s < STAGES; s++) {
                entry.hpf[ch][s].setParams(Biquad::Type::HighPass, lowFreq, 0.0f, 0.707f, sampleRate);
                entry.lpf[ch][s].setParams(Biquad::Type::LowPass, highFreq, 0.0f, 0.707f, sampleRate);
            }
        }
    }
}

void BandLimiter::process(float* buffer, int numFrames, int numChannels) {
    int channels = std::min(numChannels, 2);

    for (int e = 0; e < MAX_BL_ENTRIES; e++) {
        if (!entries_[e].active) continue;
        Entry& entry = entries_[e];

        for (int frame = 0; frame < numFrames; frame++) {
            for (int ch = 0; ch < channels; ch++) {
                int idx = frame * numChannels + ch;
                float input = buffer[idx];

                float band = input;
                for (int s = 0; s < STAGES; s++)
                    band = entry.hpf[ch][s].process(band);
                for (int s = 0; s < STAGES; s++)
                    band = entry.lpf[ch][s].process(band);

                float absVal = std::abs(band);
                if (absVal > entry.envState[ch])
                    entry.envState[ch] = absVal;
                else
                    entry.envState[ch] *= entry.releaseCoeff;

                float gain = 1.0f;
                if (entry.envState[ch] > entry.limitLinear && entry.envState[ch] > 1e-10f)
                    gain = entry.limitLinear / entry.envState[ch];

                buffer[idx] = input + band * (gain - 1.0f);
            }
        }
    }
}

void BandLimiter::reset() {
    for (int e = 0; e < MAX_BL_ENTRIES; e++) {
        Entry& entry = entries_[e];
        for (int ch = 0; ch < 2; ch++) {
            for (int s = 0; s < STAGES; s++) {
                entry.hpf[ch][s].reset();
                entry.lpf[ch][s].reset();
            }
            entry.envState[ch] = 0.0f;
        }
        entry.lastLowFreq = 0;
        entry.lastHighFreq = 0;
    }
}
