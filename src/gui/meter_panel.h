#pragma once
#include "audio/audio_engine.h"

class MeterPanel {
public:
    void render(AudioEngine& engine);

private:
    float smoothValue(float current, float target, float speed);

    float displayInputL_ = 0.0f;
    float displayInputR_ = 0.0f;
    float displayOutputL_ = 0.0f;
    float displayOutputR_ = 0.0f;
    float displayGR_ = 0.0f;
};
