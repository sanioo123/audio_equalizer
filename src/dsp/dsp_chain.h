#pragma once
#include "compressor.h"
#include "equalizer.h"
#include "reverb.h"
#include "crossover.h"
#include "band_limiter.h"
#include "multiband_processor.h"
#include "biquad.h"
#include "common/params.h"

class DSPChain {
public:
    DSPChain(SharedParams& params);

    void process(float* buffer, int numFrames, int numChannels, float sampleRate);

    Compressor& getCompressor() { return compressor_; }
    Equalizer&  getEqualizer()  { return equalizer_; }
    MultibandProcessor& getMultiband() { return multiband_; }

private:
    void updateTone(float sampleRate);

    SharedParams& params_;
    Compressor compressor_;
    Equalizer  equalizer_;
    Reverb     reverb_;
    Crossover  crossover_;
    BandLimiter bandLimiter_;
    MultibandProcessor multiband_;
    bool       reverbInitialized_ = false;

    Biquad bassTone_[2];
    Biquad trebleTone_[2];
    float lastBassFreq_ = 0, lastBassQ_ = 0, lastBassGain_ = -999;
    float lastTrebleFreq_ = 0, lastTrebleQ_ = 0, lastTrebleGain_ = -999;
    float lastToneSampleRate_ = 0;
};
