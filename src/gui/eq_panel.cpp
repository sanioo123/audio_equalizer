#include "eq_panel.h"
#include "imgui.h"
#include <cstdio>
#include <cmath>

std::string EQPanel::formatFreq(float hz) {
    if (hz >= 1000.0f) {
        float khz = hz / 1000.0f;
        if (khz == std::floor(khz))
            return std::to_string((int)khz) + "k";
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1fk", khz);
        return buf;
    }
    return std::to_string((int)hz);
}

const char* EQPanel::filterTypeName(int type) {
    switch (type) {
        case 1: return "HS";   // High Shelf
        case 2: return "LS";   // Low Shelf
        case 3: return "PK";   // Peaking
        case 4: return "BP";   // Band Pass
        case 5: return "HP";   // High Pass
        case 6: return "LP";   // Low Pass
        default: return "??";
    }
}

void EQPanel::initFromParams(EQParams& params) {
    int n = params.numBands();
    bandGains_.resize(n);
    freqLabels_.resize(n);
    for (int i = 0; i < n; i++) {
        bandGains_[i] = params.bands[i].gainDb.load(std::memory_order_relaxed);
        freqLabels_[i] = formatFreq(params.bands[i].freq);
    }
    initialized_ = true;
}

void EQPanel::render(EQParams& params) {
    if (!initialized_ || (int)bandGains_.size() != params.numBands()) {
        initFromParams(params);
    }

    int nBands = params.numBands();

    // Calculate panel width based on number of bands
    float sliderW = 28.0f;
    float spacing = 4.0f;
    float panelPadding = 20.0f;
    float panelWidth = nBands * (sliderW + spacing) + panelPadding;
    if (panelWidth < 200.0f) panelWidth = 200.0f;

    ImGui::BeginChild("EQPanel", ImVec2(panelWidth, 0), true);

    // Header with config name
    if (!params.configName.empty()) {
        ImGui::Text("EQ: %s", params.configName.c_str());
    } else {
        ImGui::Text("PARAMETRIC EQ");
    }
    ImGui::Separator();

    // Controls row
    bool enabled = params.enabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enabled##eq", &enabled)) {
        params.enabled.store(enabled, std::memory_order_relaxed);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        for (int i = 0; i < nBands; i++) {
            bandGains_[i] = params.bands[i].gainDb.load(std::memory_order_relaxed);
        }
    }

    // Preamp display
    ImGui::SameLine();
    float preamp = params.preamp.load(std::memory_order_relaxed);
    ImGui::TextDisabled("Pre: %.1f dB", preamp);

    if (nBands == 0) {
        ImGui::Spacing();
        ImGui::Text("No bands loaded. Check config.json");
        ImGui::EndChild();
        return;
    }

    ImGui::Spacing();

    float sliderHeight = ImGui::GetContentRegionAvail().y - 50.0f;
    if (sliderHeight < 80.0f) sliderHeight = 80.0f;

    for (int i = 0; i < nBands; i++) {
        if (i > 0) ImGui::SameLine(0, spacing);

        ImGui::BeginGroup();

        char id[16];
        snprintf(id, sizeof(id), "##eq%d", i);

        // Vertical slider
        if (ImGui::VSliderFloat(id, ImVec2(sliderW, sliderHeight), &bandGains_[i],
                                 -15.0f, 15.0f, "")) {
            params.bands[i].gainDb.store(bandGains_[i], std::memory_order_relaxed);
        }

        // Tooltip with full info on hover
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s %.0fHz Q:%.1f\n%.1f dB",
                filterTypeName(params.bands[i].type),
                params.bands[i].freq,
                params.bands[i].q,
                bandGains_[i]);
        }

        // Gain value label
        char valBuf[16];
        snprintf(valBuf, sizeof(valBuf), "%.0f", bandGains_[i]);
        float textWidth = ImGui::CalcTextSize(valBuf).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderW - textWidth) * 0.5f);
        ImGui::Text("%s", valBuf);

        // Frequency label
        float freqW = ImGui::CalcTextSize(freqLabels_[i].c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderW - freqW) * 0.5f);
        ImGui::TextDisabled("%s", freqLabels_[i].c_str());

        ImGui::EndGroup();
    }

    ImGui::EndChild();
}
