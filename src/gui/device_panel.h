#pragma once
#include "audio/audio_device.h"
#include "audio/audio_engine.h"
#include "common/config_loader.h"
#include "common/params.h"
#include <vector>
#include <string>

class DevicePanel {
public:
    DevicePanel() = default;
    void init(AudioDeviceManager& deviceMgr, AudioEngine& engine, SharedParams& params);
    void render();

    void loadFromConfig(const DeviceConfig& cfg);

    // Getters for save
    std::string getSelectedInputName() const;
    std::string getSelectedOutputName() const;

private:
    AudioDeviceManager* deviceMgr_ = nullptr;
    AudioEngine* engine_ = nullptr;
    SharedParams* params_ = nullptr;

    std::vector<AudioDeviceInfo> devices_;
    std::vector<std::string> deviceNames_;
    int selectedInputIdx_ = 0;
    int selectedOutputIdx_ = 0;

    std::string savedInputName_;
    std::string savedOutputName_;

    bool needsRefresh_ = true;
    void refreshDevices();
};
