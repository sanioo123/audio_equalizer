#pragma once
#include "common/params.h"
#include "dsp/reverb.h"
#include "dsp/crossover.h"
#include "dsp/band_limiter.h"

class CompressorPanel {
public:
    void render(CompressorParams& params, ToneParams& tone, ReverbParams& reverb,
                CrossoverParams& crossover, BandLimiterParams& bandLimiter,
                float gainReductionDb);

    void loadFromConfig(const CompressorConfig& cfg, const ToneConfig& toneCfg,
                        const ReverbConfig& revCfg, const CrossoverConfig& xoverCfg,
                        const BandLimiterConfig& blCfg);

    float getThreshold() const { return threshold_; }
    float getAttackMs() const { return attackMs_; }
    float getReleaseMs() const { return releaseMs_; }
    float getRatio() const { return RATIOS[ratioIndex_]; }
    float getSidechainFreq() const { return sidechainFreq_; }
    float getMakeupGain() const { return makeupGain_; }
    float getVolumeDb() const { return volumeDb_; }
    float getPreGainDb() const { return preGainDb_; }
    float getKneeDb() const { return kneeDb_; }
    float getExpansionRatio() const { return expansionRatio_; }
    float getGateThresholdDb() const { return gateThresholdDb_; }

    float getBassFreq() const { return bassFreq_; }
    float getBassQ() const { return bassQ_; }
    float getBassGain() const { return bassGain_; }
    float getTrebleFreq() const { return trebleFreq_; }
    float getTrebleQ() const { return trebleQ_; }
    float getTrebleGain() const { return trebleGain_; }

    float getReverbDecayTime() const { return reverbDecayTime_; }
    float getReverbHiRatio() const { return reverbHiRatio_; }
    float getReverbDiffusion() const { return reverbDiffusion_; }
    float getReverbInitialDelay() const { return reverbInitialDelay_; }
    float getReverbDensity() const { return reverbDensity_; }
    float getReverbLpfFreq() const { return reverbLpfFreq_; }
    float getReverbHpfFreq() const { return reverbHpfFreq_; }
    float getReverbReverbDelay() const { return reverbReverbDelay_; }
    float getReverbBalance() const { return reverbBalance_; }

    float getXoverLowFreq() const { return xoverLowFreq_; }
    float getXoverHighFreq() const { return xoverHighFreq_; }
    int   getXoverHpfSlope() const { return SLOPES[xoverHpfSlopeIdx_]; }
    int   getXoverLpfSlope() const { return SLOPES[xoverLpfSlopeIdx_]; }
    float getXoverSubGain() const { return xoverSubGain_; }

    struct BLEntryState {
        bool active = false;
        float lowFreq = 20.0f;
        float highFreq = 70.0f;
        float limitDb = 0.0f;
    };
    const BLEntryState& getBLEntry(int i) const { return blEntries_[i]; }

private:
    float threshold_ = -20.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 100.0f;
    int ratioIndex_ = 3;
    float sidechainFreq_ = 20.0f;
    float makeupGain_ = 0.0f;
    float volumeDb_ = 0.0f;
    float preGainDb_ = 12.2f;
    float kneeDb_ = 0.0f;
    float expansionRatio_ = 1.0f;
    float gateThresholdDb_ = -90.0f;

    static constexpr float RATIOS[] = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 10.0f, 20.0f};
    static constexpr int NUM_RATIOS = 8;

    float bassFreq_ = 70.0f;
    float bassQ_ = 0.10f;
    float bassGain_ = 20.0f;
    float trebleFreq_ = 10000.0f;
    float trebleQ_ = 0.60f;
    float trebleGain_ = 20.0f;

    float reverbDecayTime_ = 0.9f;
    float reverbHiRatio_ = 0.7f;
    float reverbDiffusion_ = 0.9f;
    float reverbInitialDelay_ = 26.0f;
    float reverbDensity_ = 3.0f;
    float reverbLpfFreq_ = 11000.0f;
    float reverbHpfFreq_ = 90.0f;
    float reverbReverbDelay_ = 17.0f;
    float reverbBalance_ = 20.0f;

    float xoverLowFreq_ = 30.0f;
    float xoverHighFreq_ = 70.0f;
    int xoverHpfSlopeIdx_ = 2;  // index into SLOPES[] (default 24dB)
    int xoverLpfSlopeIdx_ = 2;
    float xoverSubGain_ = 6.0f;
    static constexpr int SLOPES[] = {6, 12, 24, 48};
    static constexpr int NUM_SLOPES = 4;

    BLEntryState blEntries_[MAX_BL_ENTRIES];
};
