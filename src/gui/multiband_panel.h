#pragma once
#include "common/params.h"
#include "dsp/dsp_chain.h"

class MultibandPanel {
public:
    MultibandPanel() = default;

    void init(SharedParams& params, DSPChain& dsp);
    void render();

private:
    SharedParams* params_ = nullptr;
    DSPChain* dsp_ = nullptr;
};
