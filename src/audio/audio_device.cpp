#include "miniaudio.h"
#include "audio_device.h"

AudioDeviceManager::AudioDeviceManager() = default;

AudioDeviceManager::~AudioDeviceManager() {
    if (pContext_) {
        ma_context_uninit(pContext_);
        delete pContext_;
        pContext_ = nullptr;
    }
}

bool AudioDeviceManager::init() {
    pContext_ = new ma_context;
    ma_backend backends[] = { ma_backend_wasapi };
    ma_result res = ma_context_init(backends, 1, nullptr, pContext_);
    if (res != MA_SUCCESS) {
        delete pContext_;
        pContext_ = nullptr;
        return false;
    }
    return true;
}

std::vector<AudioDeviceInfo> AudioDeviceManager::enumeratePlaybackDevices() {
    std::vector<AudioDeviceInfo> result;
    if (!pContext_) return result;

    ma_device_info* pDevices = nullptr;
    ma_uint32 count = 0;
    if (ma_context_get_devices(pContext_, &pDevices, &count, nullptr, nullptr) != MA_SUCCESS)
        return result;

    for (ma_uint32 i = 0; i < count; i++) {
        AudioDeviceInfo info;
        info.name = pDevices[i].name;
        info.index = (int)i;
        info.isDefault = pDevices[i].isDefault;
        result.push_back(info);
    }
    return result;
}
