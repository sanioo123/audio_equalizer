#include "multiband_panel.h"
#include "imgui.h"

void MultibandPanel::init(SharedParams& params, DSPChain& dsp) {
    params_ = &params;
    dsp_ = &dsp;
}

void MultibandPanel::render() {
    if (!params_ || !dsp_) return;

    ImGui::BeginChild("MultibandProcessor", ImVec2(0, 220), true);
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "MULTIBAND SPECTRAL PROCESSOR (250Hz-20kHz)");
    ImGui::Separator();

    // Enable checkbox
    bool enabled = params_->multiband.enabled.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enable Multiband", &enabled)) {
        params_->multiband.enabled.store(enabled, std::memory_order_relaxed);
    }

    if (!enabled) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Disabled)");
        ImGui::EndChild();
        return;
    }

    ImGui::Spacing();

    // Auto-Balance
    bool autoBalance = params_->multiband.autoBalance.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Auto-Balance Spectral", &autoBalance)) {
        params_->multiband.autoBalance.store(autoBalance, std::memory_order_relaxed);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically balances all frequencies (250Hz-20kHz) for even spectral distribution");
    }

    // Auto-Balance Speed
    if (autoBalance) {
        float speed = params_->multiband.autoBalanceSpeed.load(std::memory_order_relaxed);
        if (ImGui::SliderFloat("Balance Speed", &speed, 0.01f, 1.0f, "%.2f")) {
            params_->multiband.autoBalanceSpeed.store(speed, std::memory_order_relaxed);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("How fast the auto-balance adapts to spectral changes");
        }
    }

    ImGui::Spacing();

    // Compression
    float compression = params_->multiband.compression.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Multiband Compression", &compression, 0.0f, 1.0f, "%.2f")) {
        params_->multiband.compression.store(compression, std::memory_order_relaxed);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Compression amount for each frequency band (0=none, 1=maximum)");
    }

    // Exciter Amount
    float exciter = params_->multiband.exciterAmount.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Exciter / Clarity", &exciter, 0.0f, 1.0f, "%.2f")) {
        params_->multiband.exciterAmount.store(exciter, std::memory_order_relaxed);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adds harmonic excitement for enhanced clarity and presence");
    }

    // Sub-Bass Range
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Sub-Bass Control:");

    float subLowFreq = params_->multiband.subBassLowFreq.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Sub Low Freq", &subLowFreq, 20.0f, 100.0f, "%.0f Hz")) {
        params_->multiband.subBassLowFreq.store(subLowFreq, std::memory_order_relaxed);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Low frequency cutoff for sub-bass band");
    }

    float subHighFreq = params_->multiband.subBassHighFreq.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Sub High Freq", &subHighFreq, 100.0f, 500.0f, "%.0f Hz")) {
        params_->multiband.subBassHighFreq.store(subHighFreq, std::memory_order_relaxed);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("High frequency cutoff for sub-bass band");
    }

    float subBassBoost = params_->multiband.subBassBoost.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Sub-Bass Boost", &subBassBoost, 0.0f, 10.0f, "%.1f dB")) {
        params_->multiband.subBassBoost.store(subBassBoost, std::memory_order_relaxed);
    }
    if (ImGui::IsItemHovered()) {
        char tooltip[128];
        snprintf(tooltip, sizeof(tooltip), "Boosts %.0f-%.0fHz for extra punch", subLowFreq, subHighFreq);
        ImGui::SetTooltip("%s", tooltip);
    }

    // Output Gain
    float outputGain = params_->multiband.outputGain.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Output Gain", &outputGain, -12.0f, 12.0f, "%.1f dB")) {
        params_->multiband.outputGain.store(outputGain, std::memory_order_relaxed);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Display band energies
    ImGui::Text("Spectral Energy (Real-time FFT):");
    auto& multiband = dsp_->getMultiband();
    int numBands = multiband.getNumBands();

    for (int i = 0; i < numBands; i++) {
        const auto& band = multiband.getBand(i);

        char label[64];
        if (band.lowFreq < 1000) {
            snprintf(label, sizeof(label), "%.0f-%.0fHz", band.lowFreq, band.highFreq);
        } else if (band.highFreq < 1000) {
            snprintf(label, sizeof(label), "%.0f-%.0fHz", band.lowFreq, band.highFreq);
        } else {
            snprintf(label, sizeof(label), "%.1f-%.1fkHz", band.lowFreq/1000, band.highFreq/1000);
        }

        float energy = band.energy * 100.0f;  // Scale for display
        ImGui::ProgressBar(energy, ImVec2(-1, 0), label);

        if ((i + 1) % 2 == 0 && i + 1 < numBands) {
            ImGui::Spacing();
        }
    }

    ImGui::EndChild();
}
