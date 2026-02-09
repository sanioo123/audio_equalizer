#include "meter_panel.h"
#include "imgui.h"
#include "dsp/dsp_common.h"
#include <algorithm>
#include <cmath>

float MeterPanel::smoothValue(float current, float target, float speed) {
    if (target > current) return target;
    return current + (target - current) * speed;
}

void MeterPanel::render(AudioEngine& engine) {
    ImGui::BeginChild("MeterPanel", ImVec2(0, 0), true);
    ImGui::Text("METERS");
    ImGui::Separator();

    float fallSpeed = 0.15f;

    float inL = engine.getInputLevelL();
    float inR = engine.getInputLevelR();
    float outL = engine.getOutputLevelL();
    float outR = engine.getOutputLevelR();
    float gr = engine.getGainReduction();

    displayInputL_ = smoothValue(displayInputL_, inL, fallSpeed);
    displayInputR_ = smoothValue(displayInputR_, inR, fallSpeed);
    displayOutputL_ = smoothValue(displayOutputL_, outL, fallSpeed);
    displayOutputR_ = smoothValue(displayOutputR_, outR, fallSpeed);
    displayGR_ = smoothValue(displayGR_, gr, fallSpeed);

    auto dbNorm = [](float linear) -> float {
        float db = dsp::linearToDb(linear);
        return std::max(0.0f, std::min(1.0f, (db + 60.0f) / 60.0f));
    };

    ImGui::Spacing();
    ImGui::Text("INPUT");
    ImGui::Text("L"); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    ImGui::ProgressBar(dbNorm(displayInputL_), ImVec2(-1, 12), "");
    ImGui::PopStyleColor();
    ImGui::Text("R"); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    ImGui::ProgressBar(dbNorm(displayInputR_), ImVec2(-1, 12), "");
    ImGui::PopStyleColor();
    ImGui::Text("%.1f / %.1f dB", dsp::linearToDb(displayInputL_), dsp::linearToDb(displayInputR_));

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("OUTPUT");
    ImGui::Text("L"); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
    ImGui::ProgressBar(dbNorm(displayOutputL_), ImVec2(-1, 12), "");
    ImGui::PopStyleColor();
    ImGui::Text("R"); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
    ImGui::ProgressBar(dbNorm(displayOutputR_), ImVec2(-1, 12), "");
    ImGui::PopStyleColor();
    ImGui::Text("%.1f / %.1f dB", dsp::linearToDb(displayOutputL_), dsp::linearToDb(displayOutputR_));

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("GAIN REDUCTION");
    float grNorm = displayGR_ / 40.0f;
    if (grNorm > 1.0f) grNorm = 1.0f;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.4f, 0.1f, 1.0f));
    ImGui::ProgressBar(grNorm, ImVec2(-1, 16), "");
    ImGui::PopStyleColor();
    ImGui::Text("%.1f dB", displayGR_);

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextDisabled("DEBUG");
    ImGui::Text("Rate: %d Hz  Ch: %d", engine.getDebugSampleRate(), engine.getDebugChannels());
    ImGui::Text("Frames: %u", (unsigned)engine.getDebugFrameCount());

    ImGui::EndChild();
}
