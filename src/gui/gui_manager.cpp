#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "gui_manager.h"
#include "imgui.h"
#include <cmath>

GUIManager::GUIManager(SharedParams& params, AudioEngine& engine, AudioDeviceManager& deviceMgr, DSPChain& dsp)
    : params_(params), engine_(engine), deviceMgr_(deviceMgr), dsp_(dsp) {}

void GUIManager::init() {
    devicePanel_.init(deviceMgr_, engine_, params_);
    multibandPanel_.init(params_, dsp_);

    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    configPath_ = exePath;
    size_t lastSlash = configPath_.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        configPath_ = configPath_.substr(0, lastSlash + 1);
    else
        configPath_ = ".\\";
    configPath_ += "config.json";
}

void GUIManager::loadFromConfig(const AppConfig& cfg) {
    compressorPanel_.loadFromConfig(cfg.compressor, cfg.tone, cfg.reverb, cfg.crossover, cfg.bandLimiter);
    devicePanel_.loadFromConfig(cfg.devices);
}

void GUIManager::saveConfig() {
    AppConfig cfg;

    cfg.eq.name = params_.eq.configName;
    cfg.eq.preamp = params_.eq.preamp.load(std::memory_order_relaxed);
    for (int i = 0; i < params_.eq.numBands(); i++) {
        ConfigBand cb;
        cb.type = params_.eq.bands[i].type;
        cb.frequency = params_.eq.bands[i].freq;
        cb.q = params_.eq.bands[i].q;
        cb.gain = params_.eq.bands[i].gainDb.load(std::memory_order_relaxed);
        cfg.eq.bands.push_back(cb);
    }

    cfg.compressor.enabled = params_.compressor.enabled.load(std::memory_order_relaxed);
    cfg.compressor.thresholdDb = compressorPanel_.getThreshold();
    cfg.compressor.ratio = compressorPanel_.getRatio();
    cfg.compressor.attackMs = compressorPanel_.getAttackMs();
    cfg.compressor.releaseMs = compressorPanel_.getReleaseMs();
    cfg.compressor.sidechainFreqHz = compressorPanel_.getSidechainFreq();
    cfg.compressor.makeupGainDb = compressorPanel_.getMakeupGain();
    cfg.compressor.volumeDb = compressorPanel_.getVolumeDb();
    cfg.compressor.preGainDb = compressorPanel_.getPreGainDb();
    cfg.compressor.kneeDb = compressorPanel_.getKneeDb();
    cfg.compressor.expansionRatio = compressorPanel_.getExpansionRatio();
    cfg.compressor.gateThresholdDb = compressorPanel_.getGateThresholdDb();

    cfg.tone.bassFreq = compressorPanel_.getBassFreq();
    cfg.tone.bassQ = compressorPanel_.getBassQ();
    cfg.tone.bassGainDb = compressorPanel_.getBassGain();
    cfg.tone.bassEnabled = params_.tone.bassEnabled.load(std::memory_order_relaxed);
    cfg.tone.trebleFreq = compressorPanel_.getTrebleFreq();
    cfg.tone.trebleQ = compressorPanel_.getTrebleQ();
    cfg.tone.trebleGainDb = compressorPanel_.getTrebleGain();
    cfg.tone.trebleEnabled = params_.tone.trebleEnabled.load(std::memory_order_relaxed);

    cfg.crossover.enabled = params_.crossover.enabled.load(std::memory_order_relaxed);
    cfg.crossover.lpfEnabled = params_.crossover.lpfEnabled.load(std::memory_order_relaxed);
    cfg.crossover.lowFreq = compressorPanel_.getXoverLowFreq();
    cfg.crossover.highFreq = compressorPanel_.getXoverHighFreq();
    cfg.crossover.hpfSlope = compressorPanel_.getXoverHpfSlope();
    cfg.crossover.lpfSlope = compressorPanel_.getXoverLpfSlope();
    cfg.crossover.subGainDb = compressorPanel_.getXoverSubGain();

    cfg.bandLimiter.enabled = params_.bandLimiter.enabled.load(std::memory_order_relaxed);
    for (int i = 0; i < MAX_BL_ENTRIES; i++) {
        const auto& e = compressorPanel_.getBLEntry(i);
        BandLimiterEntryConfig ec;
        ec.active = e.active;
        ec.lowFreq = e.lowFreq;
        ec.highFreq = e.highFreq;
        ec.limitDb = e.limitDb;
        cfg.bandLimiter.entries.push_back(ec);
    }

    cfg.reverb.enabled = params_.reverb.enabled.load(std::memory_order_relaxed);
    cfg.reverb.decayTime = compressorPanel_.getReverbDecayTime();
    cfg.reverb.hiRatio = compressorPanel_.getReverbHiRatio();
    cfg.reverb.diffusion = compressorPanel_.getReverbDiffusion();
    cfg.reverb.initialDelay = compressorPanel_.getReverbInitialDelay();
    cfg.reverb.density = compressorPanel_.getReverbDensity();
    cfg.reverb.lpfFreq = compressorPanel_.getReverbLpfFreq();
    cfg.reverb.hpfFreq = compressorPanel_.getReverbHpfFreq();
    cfg.reverb.reverbDelay = compressorPanel_.getReverbReverbDelay();
    cfg.reverb.balance = compressorPanel_.getReverbBalance();

    cfg.multiband.enabled = params_.multiband.enabled.load(std::memory_order_relaxed);
    cfg.multiband.autoBalance = params_.multiband.autoBalance.load(std::memory_order_relaxed);
    cfg.multiband.autoBalanceSpeed = params_.multiband.autoBalanceSpeed.load(std::memory_order_relaxed);
    cfg.multiband.compression = params_.multiband.compression.load(std::memory_order_relaxed);
    cfg.multiband.outputGain = params_.multiband.outputGain.load(std::memory_order_relaxed);
    cfg.multiband.exciterAmount = params_.multiband.exciterAmount.load(std::memory_order_relaxed);
    cfg.multiband.subBassBoost = params_.multiband.subBassBoost.load(std::memory_order_relaxed);
    cfg.multiband.subBassLowFreq = params_.multiband.subBassLowFreq.load(std::memory_order_relaxed);
    cfg.multiband.subBassHighFreq = params_.multiband.subBassHighFreq.load(std::memory_order_relaxed);

    cfg.devices.captureFrom = devicePanel_.getSelectedInputName();
    cfg.devices.playTo = devicePanel_.getSelectedOutputName();

    cfg.audio.blockSize = params_.blockSize.load(std::memory_order_relaxed);

    if (config::saveConfig(configPath_, cfg)) {
        showSaveOk_ = true;
        saveOkTimer_ = 2.0f;
    }
}

void GUIManager::render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                              ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##main", nullptr, flags);

    devicePanel_.render();

    ImGui::Spacing();

    bool bypass = params_.bypassAll.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Bypass All DSP", &bypass)) {
        params_.bypassAll.store(bypass, std::memory_order_relaxed);
    }

    ImGui::SameLine(0, 20);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
    if (ImGui::Button("Save Config")) {
        saveConfig();
    }
    ImGui::PopStyleColor();

    if (showSaveOk_) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Saved!");
        saveOkTimer_ -= ImGui::GetIO().DeltaTime;
        if (saveOkTimer_ <= 0.0f) showSaveOk_ = false;
    }

    ImGui::Spacing();

    multibandPanel_.render();

    ImGui::Spacing();

    float gr = engine_.getGainReduction();
    compressorPanel_.render(params_.compressor, params_.tone, params_.reverb, params_.crossover,
                            params_.bandLimiter, gr);

    ImGui::SameLine();
    eqPanel_.render(params_.eq);

    ImGui::SameLine();
    meterPanel_.render(engine_);

    ImGui::End();
}
