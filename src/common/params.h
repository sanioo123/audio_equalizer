#pragma once
#include <atomic>
#include <cmath>
#include <vector>
#include <string>
#include "config_loader.h"
#include "dsp/reverb.h"
#include "dsp/crossover.h"
#include "dsp/band_limiter.h"

struct CompressorParams {
    std::atomic<float> volume{1.0f};
    std::atomic<float> attackMs{10.0f};
    std::atomic<float> releaseMs{100.0f};
    std::atomic<float> ratio{4.0f};
    std::atomic<float> thresholdDb{-20.0f};
    std::atomic<float> makeupGainDb{0.0f};
    std::atomic<float> sidechainFreqHz{0.0f};
    std::atomic<float> preGainDb{12.2f};
    std::atomic<float> kneeDb{0.0f};
    std::atomic<float> expansionRatio{1.0f};
    std::atomic<float> gateThresholdDb{-90.0f};
    std::atomic<bool>  enabled{true};
};

struct BandParam {
    int type = 3;
    float freq = 1000.0f;
    float q = 1.0f;
    std::atomic<float> gainDb{0.0f};

    BandParam() = default;
    BandParam(int t, float f, float qv, float g)
        : type(t), freq(f), q(qv), gainDb(g) {}
    BandParam(const BandParam& o)
        : type(o.type), freq(o.freq), q(o.q),
          gainDb(o.gainDb.load(std::memory_order_relaxed)) {}
    BandParam& operator=(const BandParam& o) {
        type = o.type;
        freq = o.freq;
        q = o.q;
        gainDb.store(o.gainDb.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }
};

struct EQParams {
    std::string configName;
    std::atomic<float> preamp{0.0f};
    std::vector<BandParam> bands;
    std::atomic<bool> enabled{true};

    int numBands() const { return (int)bands.size(); }

    void initFromConfig(const EQConfig& cfg) {
        configName = cfg.name;
        preamp.store(cfg.preamp, std::memory_order_relaxed);
        bands.clear();
        bands.reserve(cfg.bands.size());
        for (const auto& cb : cfg.bands) {
            bands.emplace_back(cb.type, cb.frequency, cb.q, cb.gain);
        }
    }
};

struct ToneParams {
    std::atomic<float> bassFreq{70.0f};
    std::atomic<float> bassQ{0.10f};
    std::atomic<float> bassGainDb{20.0f};
    std::atomic<bool>  bassEnabled{true};

    std::atomic<float> trebleFreq{10000.0f};
    std::atomic<float> trebleQ{0.60f};
    std::atomic<float> trebleGainDb{20.0f};
    std::atomic<bool>  trebleEnabled{true};
};

struct MultibandParams {
    std::atomic<bool> enabled{false};
    std::atomic<bool> autoBalance{true};
    std::atomic<float> autoBalanceSpeed{0.1f};
    std::atomic<float> compression{0.5f};
    std::atomic<float> outputGain{0.0f};
    std::atomic<float> exciterAmount{0.3f};
    std::atomic<float> subBassBoost{10.0f};
    std::atomic<float> subBassLowFreq{30.0f};
    std::atomic<float> subBassHighFreq{250.0f};
};

struct SharedParams {
    CompressorParams compressor;
    EQParams eq;
    ToneParams tone;
    ReverbParams reverb;
    CrossoverParams crossover;
    BandLimiterParams bandLimiter;
    MultibandParams multiband;
    std::atomic<bool> bypassAll{false};
    std::atomic<int>  outputDeviceIndex{0};
    std::atomic<bool> deviceChangeRequested{false};
    std::atomic<int>  blockSize{1024};

    void loadFromConfig(const AppConfig& cfg) {
        // EQ
        eq.initFromConfig(cfg.eq);

        // Compressor
        if (cfg.compressor.loaded) {
            compressor.enabled.store(cfg.compressor.enabled, std::memory_order_relaxed);
            compressor.thresholdDb.store(cfg.compressor.thresholdDb, std::memory_order_relaxed);
            compressor.ratio.store(cfg.compressor.ratio, std::memory_order_relaxed);
            compressor.attackMs.store(cfg.compressor.attackMs, std::memory_order_relaxed);
            compressor.releaseMs.store(cfg.compressor.releaseMs, std::memory_order_relaxed);
            compressor.sidechainFreqHz.store(cfg.compressor.sidechainFreqHz, std::memory_order_relaxed);
            compressor.makeupGainDb.store(cfg.compressor.makeupGainDb, std::memory_order_relaxed);
            float volLinear = std::pow(10.0f, cfg.compressor.volumeDb / 20.0f);
            compressor.volume.store(volLinear, std::memory_order_relaxed);
            compressor.preGainDb.store(cfg.compressor.preGainDb, std::memory_order_relaxed);
            compressor.kneeDb.store(cfg.compressor.kneeDb, std::memory_order_relaxed);
            compressor.expansionRatio.store(cfg.compressor.expansionRatio, std::memory_order_relaxed);
            compressor.gateThresholdDb.store(cfg.compressor.gateThresholdDb, std::memory_order_relaxed);
        }

        // Tone
        if (cfg.tone.loaded) {
            tone.bassFreq.store(cfg.tone.bassFreq, std::memory_order_relaxed);
            tone.bassQ.store(cfg.tone.bassQ, std::memory_order_relaxed);
            tone.bassGainDb.store(cfg.tone.bassGainDb, std::memory_order_relaxed);
            tone.bassEnabled.store(cfg.tone.bassEnabled, std::memory_order_relaxed);
            tone.trebleFreq.store(cfg.tone.trebleFreq, std::memory_order_relaxed);
            tone.trebleQ.store(cfg.tone.trebleQ, std::memory_order_relaxed);
            tone.trebleGainDb.store(cfg.tone.trebleGainDb, std::memory_order_relaxed);
            tone.trebleEnabled.store(cfg.tone.trebleEnabled, std::memory_order_relaxed);
        }

        // Crossover
        if (cfg.crossover.loaded) {
            crossover.enabled.store(cfg.crossover.enabled, std::memory_order_relaxed);
            crossover.lpfEnabled.store(cfg.crossover.lpfEnabled, std::memory_order_relaxed);
            crossover.lowFreq.store(cfg.crossover.lowFreq, std::memory_order_relaxed);
            crossover.highFreq.store(cfg.crossover.highFreq, std::memory_order_relaxed);
            crossover.hpfSlope.store(cfg.crossover.hpfSlope, std::memory_order_relaxed);
            crossover.lpfSlope.store(cfg.crossover.lpfSlope, std::memory_order_relaxed);
            crossover.subGainDb.store(cfg.crossover.subGainDb, std::memory_order_relaxed);
        }

        // Reverb
        if (cfg.reverb.loaded) {
            reverb.enabled.store(cfg.reverb.enabled, std::memory_order_relaxed);
            reverb.decayTime.store(cfg.reverb.decayTime, std::memory_order_relaxed);
            reverb.hiRatio.store(cfg.reverb.hiRatio, std::memory_order_relaxed);
            reverb.diffusion.store(cfg.reverb.diffusion, std::memory_order_relaxed);
            reverb.initialDelay.store(cfg.reverb.initialDelay, std::memory_order_relaxed);
            reverb.density.store(cfg.reverb.density, std::memory_order_relaxed);
            reverb.lpfFreq.store(cfg.reverb.lpfFreq, std::memory_order_relaxed);
            reverb.hpfFreq.store(cfg.reverb.hpfFreq, std::memory_order_relaxed);
            reverb.reverbDelay.store(cfg.reverb.reverbDelay, std::memory_order_relaxed);
            reverb.balance.store(cfg.reverb.balance, std::memory_order_relaxed);
        }

        // Band Limiter
        if (cfg.bandLimiter.loaded) {
            bandLimiter.enabled.store(cfg.bandLimiter.enabled, std::memory_order_relaxed);
            int count = std::min((int)cfg.bandLimiter.entries.size(), MAX_BL_ENTRIES);
            for (int i = 0; i < count; i++) {
                const auto& e = cfg.bandLimiter.entries[i];
                bandLimiter.entries[i].active.store(e.active, std::memory_order_relaxed);
                bandLimiter.entries[i].lowFreq.store(e.lowFreq, std::memory_order_relaxed);
                bandLimiter.entries[i].highFreq.store(e.highFreq, std::memory_order_relaxed);
                bandLimiter.entries[i].limitDb.store(e.limitDb, std::memory_order_relaxed);
            }
        }

        if (cfg.multiband.loaded) {
            multiband.enabled.store(cfg.multiband.enabled, std::memory_order_relaxed);
            multiband.autoBalance.store(cfg.multiband.autoBalance, std::memory_order_relaxed);
            multiband.autoBalanceSpeed.store(cfg.multiband.autoBalanceSpeed, std::memory_order_relaxed);
            multiband.compression.store(cfg.multiband.compression, std::memory_order_relaxed);
            multiband.outputGain.store(cfg.multiband.outputGain, std::memory_order_relaxed);
            multiband.exciterAmount.store(cfg.multiband.exciterAmount, std::memory_order_relaxed);
            multiband.subBassBoost.store(cfg.multiband.subBassBoost, std::memory_order_relaxed);
            multiband.subBassLowFreq.store(cfg.multiband.subBassLowFreq, std::memory_order_relaxed);
            multiband.subBassHighFreq.store(cfg.multiband.subBassHighFreq, std::memory_order_relaxed);
        }

        if (cfg.audio.loaded) {
            blockSize.store(cfg.audio.blockSize, std::memory_order_relaxed);
        }
    }
};
