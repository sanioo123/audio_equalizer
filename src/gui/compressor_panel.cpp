#include "compressor_panel.h"
#include "imgui.h"
#include <cmath>
#include <cstdio>

constexpr float CompressorPanel::RATIOS[];
constexpr int CompressorPanel::SLOPES[];

void CompressorPanel::loadFromConfig(const CompressorConfig& cfg, const ToneConfig& toneCfg,
                                     const ReverbConfig& revCfg, const CrossoverConfig& xoverCfg,
                                     const BandLimiterConfig& blCfg) {
    if (cfg.loaded) {
        threshold_ = cfg.thresholdDb;
        attackMs_ = cfg.attackMs;
        releaseMs_ = cfg.releaseMs;
        sidechainFreq_ = cfg.sidechainFreqHz;
        makeupGain_ = cfg.makeupGainDb;
        volumeDb_ = cfg.volumeDb;

        preGainDb_ = cfg.preGainDb;
        kneeDb_ = cfg.kneeDb;
        expansionRatio_ = cfg.expansionRatio;
        gateThresholdDb_ = cfg.gateThresholdDb;

        ratioIndex_ = 0;
        float minDiff = 9999.0f;
        for (int i = 0; i < NUM_RATIOS; i++) {
            float diff = std::abs(RATIOS[i] - cfg.ratio);
            if (diff < minDiff) {
                minDiff = diff;
                ratioIndex_ = i;
            }
        }
    }

    if (toneCfg.loaded) {
        bassFreq_ = toneCfg.bassFreq;
        bassQ_ = toneCfg.bassQ;
        bassGain_ = toneCfg.bassGainDb;
        trebleFreq_ = toneCfg.trebleFreq;
        trebleQ_ = toneCfg.trebleQ;
        trebleGain_ = toneCfg.trebleGainDb;
    }

    if (revCfg.loaded) {
        reverbDecayTime_ = revCfg.decayTime;
        reverbHiRatio_ = revCfg.hiRatio;
        reverbDiffusion_ = revCfg.diffusion;
        reverbInitialDelay_ = revCfg.initialDelay;
        reverbDensity_ = revCfg.density;
        reverbLpfFreq_ = revCfg.lpfFreq;
        reverbHpfFreq_ = revCfg.hpfFreq;
        reverbReverbDelay_ = revCfg.reverbDelay;
        reverbBalance_ = revCfg.balance;
    }

    if (xoverCfg.loaded) {
        xoverLowFreq_ = xoverCfg.lowFreq;
        xoverHighFreq_ = xoverCfg.highFreq;
        xoverSubGain_ = xoverCfg.subGainDb;
        xoverHpfSlopeIdx_ = 2; // default 24dB
        xoverLpfSlopeIdx_ = 2;
        for (int i = 0; i < NUM_SLOPES; i++) {
            if (SLOPES[i] == xoverCfg.hpfSlope) { xoverHpfSlopeIdx_ = i; break; }
        }
        for (int i = 0; i < NUM_SLOPES; i++) {
            if (SLOPES[i] == xoverCfg.lpfSlope) { xoverLpfSlopeIdx_ = i; break; }
        }
    }

    if (blCfg.loaded) {
        int count = std::min((int)blCfg.entries.size(), MAX_BL_ENTRIES);
        for (int i = 0; i < count; i++) {
            blEntries_[i].active = blCfg.entries[i].active;
            blEntries_[i].lowFreq = blCfg.entries[i].lowFreq;
            blEntries_[i].highFreq = blCfg.entries[i].highFreq;
            blEntries_[i].limitDb = blCfg.entries[i].limitDb;
        }
    }
}

void CompressorPanel::render(CompressorParams& params, ToneParams& tone, ReverbParams& reverb,
                             CrossoverParams& crossover, BandLimiterParams& bandLimiter,
                             float gainReductionDb) {
    ImGui::BeginChild("CompressorPanel", ImVec2(280, 0), true);
    ImGui::Text("COMPRESSOR");
    ImGui::Separator();

    bool enabled = params.enabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enabled##comp", &enabled)) {
        params.enabled.store(enabled, std::memory_order_relaxed);
    }

    ImGui::Spacing();

    // Threshold
    ImGui::Text("Threshold");
    if (ImGui::SliderFloat("##thresh", &threshold_, -80.0f, 0.0f, "%.1f dB")) {
        params.thresholdDb.store(threshold_, std::memory_order_relaxed);
    }

    // Ratio
    ImGui::Text("Ratio");
    const char* ratioLabels[] = {"1:1","2:1","3:1","4:1","6:1","8:1","10:1","20:1"};
    if (ImGui::Combo("##ratio", &ratioIndex_, ratioLabels, NUM_RATIOS)) {
        params.ratio.store(RATIOS[ratioIndex_], std::memory_order_relaxed);
    }

    // Attack
    ImGui::Text("Attack");
    if (ImGui::SliderFloat("##attack", &attackMs_, 0.1f, 200.0f, "%.1f ms",
                            ImGuiSliderFlags_Logarithmic)) {
        params.attackMs.store(attackMs_, std::memory_order_relaxed);
    }

    // Release
    ImGui::Text("Release");
    if (ImGui::SliderFloat("##release", &releaseMs_, 10.0f, 2000.0f, "%.0f ms",
                            ImGuiSliderFlags_Logarithmic)) {
        params.releaseMs.store(releaseMs_, std::memory_order_relaxed);
    }

    // Sidechain HPF
    ImGui::Text("Sidechain HPF");
    if (ImGui::SliderFloat("##sc_freq", &sidechainFreq_, 20.0f, 20000.0f, "%.0f Hz",
                            ImGuiSliderFlags_Logarithmic)) {
        params.sidechainFreqHz.store(sidechainFreq_, std::memory_order_relaxed);
    }

    // Pre-Gain
    ImGui::Text("Pre-Gain");
    if (ImGui::SliderFloat("##pregain", &preGainDb_, -20.0f, 40.0f, "%.1f dB")) {
        params.preGainDb.store(preGainDb_, std::memory_order_relaxed);
    }

    // Knee
    ImGui::Text("Knee");
    if (ImGui::SliderFloat("##knee", &kneeDb_, 0.0f, 30.0f, "%.1f dB")) {
        params.kneeDb.store(kneeDb_, std::memory_order_relaxed);
    }

    // Expansion Ratio
    ImGui::Text("Expansion Ratio");
    if (ImGui::SliderFloat("##expand", &expansionRatio_, 1.0f, 10.0f, "%.1f:1")) {
        params.expansionRatio.store(expansionRatio_, std::memory_order_relaxed);
    }

    // Noise Gate
    ImGui::Text("Noise Gate");
    if (ImGui::SliderFloat("##gate", &gateThresholdDb_, -96.0f, 0.0f, "%.1f dB")) {
        params.gateThresholdDb.store(gateThresholdDb_, std::memory_order_relaxed);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Makeup Gain
    ImGui::Text("Makeup Gain");
    if (ImGui::SliderFloat("##makeup", &makeupGain_, 0.0f, 60.0f, "%.1f dB")) {
        params.makeupGainDb.store(makeupGain_, std::memory_order_relaxed);
    }

    // Volume
    ImGui::Text("Volume");
    if (ImGui::SliderFloat("##vol", &volumeDb_, -60.0f, 12.0f, "%.1f dB")) {
        float linear = std::pow(10.0f, volumeDb_ / 20.0f);
        params.volume.store(linear, std::memory_order_relaxed);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Gain Reduction meter
    ImGui::Text("Gain Reduction: %.1f dB", gainReductionDb);
    float grNorm = gainReductionDb / 40.0f;
    if (grNorm > 1.0f) grNorm = 1.0f;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
    ImGui::ProgressBar(grNorm, ImVec2(-1, 14), "");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("TONE");
    ImGui::Separator();

    // Bass
    bool bassOn = tone.bassEnabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Bass##tone", &bassOn)) {
        tone.bassEnabled.store(bassOn, std::memory_order_relaxed);
    }
    if (bassOn) {
        ImGui::Text("Freq");
        if (ImGui::SliderFloat("##bass_freq", &bassFreq_, 20.0f, 500.0f, "%.0f Hz",
                                ImGuiSliderFlags_Logarithmic)) {
            tone.bassFreq.store(bassFreq_, std::memory_order_relaxed);
        }
        ImGui::Text("Gain");
        if (ImGui::SliderFloat("##bass_gain", &bassGain_, -20.0f, 120.0f, "%.1f dB")) {
            tone.bassGainDb.store(bassGain_, std::memory_order_relaxed);
        }
        ImGui::Text("Q");
        if (ImGui::SliderFloat("##bass_q", &bassQ_, 0.05f, 5.0f, "%.2f",
                                ImGuiSliderFlags_Logarithmic)) {
            tone.bassQ.store(bassQ_, std::memory_order_relaxed);
        }
    }

    ImGui::Spacing();

    // Treble
    bool trebleOn = tone.trebleEnabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Treble##tone", &trebleOn)) {
        tone.trebleEnabled.store(trebleOn, std::memory_order_relaxed);
    }
    if (trebleOn) {
        ImGui::Text("Freq");
        if (ImGui::SliderFloat("##treb_freq", &trebleFreq_, 1000.0f, 20000.0f, "%.0f Hz",
                                ImGuiSliderFlags_Logarithmic)) {
            tone.trebleFreq.store(trebleFreq_, std::memory_order_relaxed);
        }
        ImGui::Text("Gain");
        if (ImGui::SliderFloat("##treb_gain", &trebleGain_, -20.0f, 120.0f, "%.1f dB")) {
            tone.trebleGainDb.store(trebleGain_, std::memory_order_relaxed);
        }
        ImGui::Text("Q");
        if (ImGui::SliderFloat("##treb_q", &trebleQ_, 0.05f, 5.0f, "%.2f",
                                ImGuiSliderFlags_Logarithmic)) {
            tone.trebleQ.store(trebleQ_, std::memory_order_relaxed);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("CROSSOVER");
    ImGui::Separator();

    bool xoverOn = crossover.enabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enabled##xover", &xoverOn)) {
        crossover.enabled.store(xoverOn, std::memory_order_relaxed);
    }

    if (xoverOn) {
        const char* slopeLabels[] = {"6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct"};

        // HPF section (always active - defines sub cutoff)
        ImGui::Text("Sub Cutoff (HPF)");
        if (ImGui::SliderFloat("##xover_lo", &xoverLowFreq_, 20.0f, 200.0f, "%.0f Hz")) {
            crossover.lowFreq.store(xoverLowFreq_, std::memory_order_relaxed);
        }
        ImGui::Text("HPF Slope");
        if (ImGui::Combo("##xover_hpf_slope", &xoverHpfSlopeIdx_, slopeLabels, NUM_SLOPES)) {
            crossover.hpfSlope.store(SLOPES[xoverHpfSlopeIdx_], std::memory_order_relaxed);
        }

        ImGui::Spacing();

        // LPF section (optional - upper limit for sub band)
        bool lpfOn = crossover.lpfEnabled.load(std::memory_order_relaxed);
        if (ImGui::Checkbox("LPF##xover_lpf", &lpfOn)) {
            crossover.lpfEnabled.store(lpfOn, std::memory_order_relaxed);
        }
        if (lpfOn) {
            ImGui::Text("LPF Cutoff");
            if (ImGui::SliderFloat("##xover_hi", &xoverHighFreq_, 40.0f, 200.0f, "%.0f Hz")) {
                crossover.highFreq.store(xoverHighFreq_, std::memory_order_relaxed);
            }
            ImGui::Text("LPF Slope");
            if (ImGui::Combo("##xover_lpf_slope", &xoverLpfSlopeIdx_, slopeLabels, NUM_SLOPES)) {
                crossover.lpfSlope.store(SLOPES[xoverLpfSlopeIdx_], std::memory_order_relaxed);
            }
        }

        ImGui::Spacing();

        ImGui::Text("Sub Gain");
        if (ImGui::SliderFloat("##xover_gain", &xoverSubGain_, -12.0f, 30.0f, "%.1f dB")) {
            crossover.subGainDb.store(xoverSubGain_, std::memory_order_relaxed);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("BAND LIMITER");
    ImGui::Separator();

    bool blOn = bandLimiter.enabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enabled##bl", &blOn)) {
        bandLimiter.enabled.store(blOn, std::memory_order_relaxed);
    }

    if (blOn) {
        for (int i = 0; i < MAX_BL_ENTRIES; i++) {
            ImGui::PushID(i);
            BLEntryState& e = blEntries_[i];

            char header[32];
            snprintf(header, sizeof(header), "Band %d", i + 1);

            if (ImGui::Checkbox("##bl_active", &e.active)) {
                bandLimiter.entries[i].active.store(e.active, std::memory_order_relaxed);
            }
            ImGui::SameLine();
            ImGui::Text("%s", header);

            if (e.active) {
                // Low frequency
                ImGui::Text("From");
                if (ImGui::SliderFloat("##bl_lo", &e.lowFreq, 20.0f, 20000.0f, "%.0f Hz",
                                        ImGuiSliderFlags_Logarithmic)) {
                    bandLimiter.entries[i].lowFreq.store(e.lowFreq, std::memory_order_relaxed);
                }

                // High frequency
                ImGui::Text("To");
                if (ImGui::SliderFloat("##bl_hi", &e.highFreq, 20.0f, 20000.0f, "%.0f Hz",
                                        ImGuiSliderFlags_Logarithmic)) {
                    bandLimiter.entries[i].highFreq.store(e.highFreq, std::memory_order_relaxed);
                }

                // Limit dB
                ImGui::Text("Limit");
                if (ImGui::SliderFloat("##bl_limit", &e.limitDb, -60.0f, 40.0f, "%.1f dB")) {
                    bandLimiter.entries[i].limitDb.store(e.limitDb, std::memory_order_relaxed);
                }
            }

            ImGui::PopID();
            if (i < MAX_BL_ENTRIES - 1 && e.active) ImGui::Spacing();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("REVERB");
    ImGui::Separator();

    bool reverbOn = reverb.enabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enabled##reverb", &reverbOn)) {
        reverb.enabled.store(reverbOn, std::memory_order_relaxed);
    }

    if (reverbOn) {
        ImGui::Text("Rev Time");
        if (ImGui::SliderFloat("##rev_time", &reverbDecayTime_, 0.1f, 10.0f, "%.1f s",
                                ImGuiSliderFlags_Logarithmic)) {
            reverb.decayTime.store(reverbDecayTime_, std::memory_order_relaxed);
        }
        ImGui::Text("Hi Ratio");
        if (ImGui::SliderFloat("##rev_hiratio", &reverbHiRatio_, 0.0f, 1.0f, "%.2f")) {
            reverb.hiRatio.store(reverbHiRatio_, std::memory_order_relaxed);
        }
        ImGui::Text("Diffusion");
        if (ImGui::SliderFloat("##rev_diff", &reverbDiffusion_, 0.0f, 1.0f, "%.2f")) {
            reverb.diffusion.store(reverbDiffusion_, std::memory_order_relaxed);
        }
        ImGui::Text("Ini Delay");
        if (ImGui::SliderFloat("##rev_idly", &reverbInitialDelay_, 0.0f, 100.0f, "%.1f ms")) {
            reverb.initialDelay.store(reverbInitialDelay_, std::memory_order_relaxed);
        }
        ImGui::Text("Density");
        if (ImGui::SliderFloat("##rev_dens", &reverbDensity_, 0.0f, 12.0f, "%.1f")) {
            reverb.density.store(reverbDensity_, std::memory_order_relaxed);
        }
        ImGui::Text("LPF");
        if (ImGui::SliderFloat("##rev_lpf", &reverbLpfFreq_, 1000.0f, 20000.0f, "%.0f Hz",
                                ImGuiSliderFlags_Logarithmic)) {
            reverb.lpfFreq.store(reverbLpfFreq_, std::memory_order_relaxed);
        }
        ImGui::Text("HPF");
        if (ImGui::SliderFloat("##rev_hpf", &reverbHpfFreq_, 20.0f, 500.0f, "%.0f Hz",
                                ImGuiSliderFlags_Logarithmic)) {
            reverb.hpfFreq.store(reverbHpfFreq_, std::memory_order_relaxed);
        }
        ImGui::Text("Rev Delay");
        if (ImGui::SliderFloat("##rev_rdly", &reverbReverbDelay_, 0.0f, 100.0f, "%.1f ms")) {
            reverb.reverbDelay.store(reverbReverbDelay_, std::memory_order_relaxed);
        }
        ImGui::Text("Balance");
        if (ImGui::SliderFloat("##rev_bal", &reverbBalance_, 0.0f, 100.0f, "%.0f %%")) {
            reverb.balance.store(reverbBalance_, std::memory_order_relaxed);
        }
    }

    ImGui::EndChild();
}
