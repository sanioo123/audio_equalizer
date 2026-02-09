#pragma once
#include "compressor_panel.h"
#include "eq_panel.h"
#include "meter_panel.h"
#include "device_panel.h"
#include "multiband_panel.h"
#include "dx11_context.h"
#include "audio/audio_engine.h"
#include "audio/audio_device.h"
#include "dsp/dsp_chain.h"
#include "common/params.h"
#include "common/config_loader.h"

class GUIManager {
public:
    GUIManager(SharedParams& params, AudioEngine& engine, AudioDeviceManager& deviceMgr, DSPChain& dsp);

    void init();
    void render();

    void loadFromConfig(const AppConfig& cfg);

    CompressorPanel& getCompressorPanel() { return compressorPanel_; }
    DevicePanel& getDevicePanel() { return devicePanel_; }

private:
    void saveConfig();

    SharedParams& params_;
    AudioEngine& engine_;
    AudioDeviceManager& deviceMgr_;
    DSPChain& dsp_;

    CompressorPanel compressorPanel_;
    EQPanel eqPanel_;
    MeterPanel meterPanel_;
    DevicePanel devicePanel_;
    MultibandPanel multibandPanel_;

    std::string configPath_;
    bool showSaveOk_ = false;
    float saveOkTimer_ = 0.0f;
};
