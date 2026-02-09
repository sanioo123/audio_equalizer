#pragma once
#include <string>
#include <vector>

struct AudioDeviceInfo {
    std::string name;
    int index = -1;
    bool isDefault = false;
};

struct ma_context;

class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    bool init();
    std::vector<AudioDeviceInfo> enumeratePlaybackDevices();

private:
    ma_context* pContext_ = nullptr;
};
