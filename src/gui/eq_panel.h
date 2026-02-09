#pragma once
#include "common/params.h"
#include <vector>
#include <string>

class EQPanel {
public:
    void render(EQParams& params);

private:
    std::vector<float> bandGains_;
    std::vector<std::string> freqLabels_;
    bool initialized_ = false;

    void initFromParams(EQParams& params);
    static std::string formatFreq(float hz);
    static const char* filterTypeName(int type);
};
