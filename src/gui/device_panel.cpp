#include "device_panel.h"
#include "imgui.h"

void DevicePanel::init(AudioDeviceManager& deviceMgr, AudioEngine& engine, SharedParams& params) {
    deviceMgr_ = &deviceMgr;
    engine_ = &engine;
    params_ = &params;
    refreshDevices();
}

void DevicePanel::loadFromConfig(const DeviceConfig& cfg) {
    if (!cfg.loaded) return;
    savedInputName_ = cfg.captureFrom;
    savedOutputName_ = cfg.playTo;
    // Re-match after loading
    refreshDevices();
}

void DevicePanel::refreshDevices() {
    if (!deviceMgr_) return;

    devices_ = deviceMgr_->enumeratePlaybackDevices();
    deviceNames_.clear();
    selectedInputIdx_ = 0;
    selectedOutputIdx_ = 0;

    for (size_t i = 0; i < devices_.size(); i++) {
        deviceNames_.push_back(devices_[i].name);
        if (devices_[i].isDefault) {
            selectedOutputIdx_ = (int)i;
        }
    }

    // Match saved device names from config
    if (!savedInputName_.empty()) {
        for (size_t i = 0; i < devices_.size(); i++) {
            if (devices_[i].name == savedInputName_) {
                selectedInputIdx_ = (int)i;
                break;
            }
        }
    }
    if (!savedOutputName_.empty()) {
        for (size_t i = 0; i < devices_.size(); i++) {
            if (devices_[i].name == savedOutputName_) {
                selectedOutputIdx_ = (int)i;
                break;
            }
        }
    }

    needsRefresh_ = false;
}

std::string DevicePanel::getSelectedInputName() const {
    if (selectedInputIdx_ >= 0 && selectedInputIdx_ < (int)deviceNames_.size())
        return deviceNames_[selectedInputIdx_];
    return "";
}

std::string DevicePanel::getSelectedOutputName() const {
    if (selectedOutputIdx_ >= 0 && selectedOutputIdx_ < (int)deviceNames_.size())
        return deviceNames_[selectedOutputIdx_];
    return "";
}

void DevicePanel::render() {
    if (needsRefresh_) refreshDevices();

    auto getter = [](void* data, int idx, const char** out) -> bool {
        auto* names = (std::vector<std::string>*)data;
        if (idx < 0 || idx >= (int)names->size()) return false;
        *out = (*names)[idx].c_str();
        return true;
    };

    ImGui::BeginChild("DeviceBar", ImVec2(0, 80), true);

    ImGui::Text("Capture from:");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    if (!deviceNames_.empty()) {
        ImGui::Combo("##input", &selectedInputIdx_, getter,
                      (void*)&deviceNames_, (int)deviceNames_.size());
    } else {
        ImGui::Text("(no devices)");
    }
    ImGui::PopItemWidth();

    ImGui::SameLine(0, 15);
    ImGui::Text("Play to:");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    if (!deviceNames_.empty()) {
        ImGui::Combo("##output", &selectedOutputIdx_, getter,
                      (void*)&deviceNames_, (int)deviceNames_.size());
    }
    ImGui::PopItemWidth();

    ImGui::SameLine(0, 10);
    if (ImGui::Button("Refresh")) needsRefresh_ = true;

    ImGui::SameLine(0, 15);
    ImGui::Text("Block Size:");
    ImGui::SameLine();
    ImGui::PushItemWidth(120);
    int blockSize = params_->blockSize.load(std::memory_order_relaxed);
    const char* blockSizes[] = { "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384" };
    int blockValues[] = { 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384 };
    int currentIdx = 4; // default 1024
    for (int i = 0; i < 9; i++) {
        if (blockValues[i] == blockSize) {
            currentIdx = i;
            break;
        }
    }
    if (ImGui::Combo("##blocksize", &currentIdx, blockSizes, 9)) {
        params_->blockSize.store(blockValues[currentIdx], std::memory_order_relaxed);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " Restart audio to apply");
    }
    ImGui::PopItemWidth();

    bool isRunning = engine_ && engine_->isRunning();
    if (isRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Stop", ImVec2(80, 0))) engine_->stop();
        ImGui::PopStyleColor();
    } else {
        bool canStart = !devices_.empty();
        if (canStart) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            if (ImGui::Button("Start", ImVec2(80, 0))) {
                engine_->start(selectedInputIdx_, selectedOutputIdx_);
            }
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine(0, 15);
    auto status = engine_->getStatus();
    const char* statusStr = AudioEngine::statusToString(status);
    bool isError = (status >= AudioEngine::Status::ErrorInit);
    if (isError) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    } else if (status == AudioEngine::Status::Running) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    }
    ImGui::Text("%s", statusStr);
    ImGui::PopStyleColor();

    if (isError) {
        const std::string& detail = engine_->getErrorDetail();
        if (!detail.empty()) {
            ImGui::SameLine(0, 8);
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "(%s)", detail.c_str());
        }
    }

    ImGui::EndChild();
}
