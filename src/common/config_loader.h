#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cmath>

struct ConfigBand {
    int type = 3;
    float frequency = 1000.0f;
    float q = 1.0f;
    float gain = 0.0f;
};

struct CompressorConfig {
    bool enabled = true;
    float thresholdDb = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float sidechainFreqHz = 0.0f;
    float makeupGainDb = 0.0f;
    float volumeDb = 0.0f;
    float preGainDb = 12.2f;
    float kneeDb = 0.0f;
    float expansionRatio = 1.0f;
    float gateThresholdDb = -90.0f;
    bool loaded = false;
};

struct ToneConfig {
    float bassFreq = 70.0f;
    float bassQ = 0.10f;
    float bassGainDb = 20.0f;
    bool bassEnabled = true;
    float trebleFreq = 10000.0f;
    float trebleQ = 0.60f;
    float trebleGainDb = 20.0f;
    bool trebleEnabled = true;
    bool loaded = false;
};

struct ReverbConfig {
    bool enabled = true;
    float decayTime = 0.9f;
    float hiRatio = 0.7f;
    float diffusion = 0.9f;
    float initialDelay = 26.0f;
    float density = 3.0f;
    float lpfFreq = 11000.0f;
    float hpfFreq = 90.0f;
    float reverbDelay = 17.0f;
    float balance = 20.0f;
    bool loaded = false;
};

struct CrossoverConfig {
    bool enabled = true;
    bool lpfEnabled = false;
    float lowFreq = 30.0f;
    float highFreq = 70.0f;
    int hpfSlope = 24;
    int lpfSlope = 24;
    float subGainDb = 6.0f;
    bool loaded = false;
};

struct BandLimiterEntryConfig {
    bool active = false;
    float lowFreq = 20.0f;
    float highFreq = 70.0f;
    float limitDb = 0.0f;
};

struct BandLimiterConfig {
    bool enabled = false;
    std::vector<BandLimiterEntryConfig> entries;
    bool loaded = false;
};

struct MultibandConfig {
    bool enabled = false;
    bool autoBalance = true;
    float autoBalanceSpeed = 0.1f;
    float compression = 0.5f;
    float outputGain = 0.0f;
    float exciterAmount = 0.3f;
    float subBassBoost = 10.0f;
    float subBassLowFreq = 30.0f;
    float subBassHighFreq = 250.0f;
    bool loaded = false;
};

struct AudioConfig {
    int blockSize = 1024;
    bool loaded = false;
};

struct DeviceConfig {
    std::string captureFrom;
    std::string playTo;
    bool loaded = false;
};

struct EQConfig {
    std::string name;
    float preamp = 0.0f;
    std::vector<ConfigBand> bands;
};

struct AppConfig {
    EQConfig eq;
    CompressorConfig compressor;
    ToneConfig tone;
    ReverbConfig reverb;
    CrossoverConfig crossover;
    BandLimiterConfig bandLimiter;
    MultibandConfig multiband;
    DeviceConfig devices;
    AudioConfig audio;
};

namespace config {

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline std::string extractStringValue(const std::string& text, const std::string& key) {
    size_t pos = text.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = text.find(':', pos);
    if (pos == std::string::npos) return "";
    size_t q1 = text.find('"', pos + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = text.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";
    return text.substr(q1 + 1, q2 - q1 - 1);
}

inline float extractFloatValue(const std::string& text, const std::string& key) {
    size_t pos = text.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0f;
    pos = text.find(':', pos);
    if (pos == std::string::npos) return 0.0f;
    std::string val = text.substr(pos + 1);
    size_t comma = val.find(',');
    if (comma != std::string::npos) val = val.substr(0, comma);
    val = trim(val);
    return std::strtof(val.c_str(), nullptr);
}

inline int extractIntValue(const std::string& text, const std::string& key) {
    size_t pos = text.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0;
    pos = text.find(':', pos);
    if (pos == std::string::npos) return 0;
    std::string val = text.substr(pos + 1);
    size_t comma = val.find(',');
    if (comma != std::string::npos) val = val.substr(0, comma);
    val = trim(val);
    return std::atoi(val.c_str());
}

inline bool extractBoolValue(const std::string& text, const std::string& key, bool defaultVal = false) {
    size_t pos = text.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;
    pos = text.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    std::string val = text.substr(pos + 1);
    size_t comma = val.find(',');
    if (comma != std::string::npos) val = val.substr(0, comma);
    val = trim(val);
    return (val == "true");
}

inline std::string extractObject(const std::string& text, const std::string& key) {
    size_t pos = text.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    size_t braceStart = text.find('{', pos);
    if (braceStart == std::string::npos) return "";
    int depth = 1;
    size_t i = braceStart + 1;
    while (i < text.size() && depth > 0) {
        if (text[i] == '{') depth++;
        else if (text[i] == '}') depth--;
        i++;
    }
    if (depth != 0) return "";
    return text.substr(braceStart, i - braceStart);
}

inline AppConfig loadConfig(const std::string& path) {
    AppConfig cfg;

    std::ifstream file(path);
    if (!file.is_open()) return cfg;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    cfg.eq.name = extractStringValue(content, "name");

    {
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.find("\"preamp\"") != std::string::npos) {
                cfg.eq.preamp = extractFloatValue(line, "preamp");
                break;
            }
        }
    }

    size_t bandsStart = content.find("\"bands\"");
    if (bandsStart != std::string::npos) {
        bandsStart = content.find('[', bandsStart);
        size_t bandsEnd = std::string::npos;
        if (bandsStart != std::string::npos) {
            int depth = 1;
            size_t i = bandsStart + 1;
            while (i < content.size() && depth > 0) {
                if (content[i] == '[') depth++;
                else if (content[i] == ']') depth--;
                i++;
            }
            if (depth == 0) bandsEnd = i - 1;
        }

        if (bandsStart != std::string::npos && bandsEnd != std::string::npos) {
            std::string bandsStr = content.substr(bandsStart + 1, bandsEnd - bandsStart - 1);
            size_t pos = 0;
            while (pos < bandsStr.size()) {
                size_t objStart = bandsStr.find('{', pos);
                if (objStart == std::string::npos) break;
                size_t objEnd = bandsStr.find('}', objStart);
                if (objEnd == std::string::npos) break;

                std::string obj = bandsStr.substr(objStart, objEnd - objStart + 1);
                int type = extractIntValue(obj, "type");

                if (type != 0) {
                    ConfigBand band;
                    band.type = type;
                    band.frequency = extractFloatValue(obj, "frequency");
                    band.q = extractFloatValue(obj, "q");
                    band.gain = extractFloatValue(obj, "gain");
                    cfg.eq.bands.push_back(band);
                }
                pos = objEnd + 1;
            }
        }
    }

    std::string compObj = extractObject(content, "compressor");
    if (!compObj.empty()) {
        cfg.compressor.loaded = true;
        cfg.compressor.enabled = extractBoolValue(compObj, "enabled", true);
        if (compObj.find("\"threshold\"") != std::string::npos)
            cfg.compressor.thresholdDb = extractFloatValue(compObj, "threshold");
        if (compObj.find("\"ratio\"") != std::string::npos)
            cfg.compressor.ratio = std::max(1.0f, extractFloatValue(compObj, "ratio"));
        if (compObj.find("\"attackMs\"") != std::string::npos)
            cfg.compressor.attackMs = std::max(0.01f, extractFloatValue(compObj, "attackMs"));
        if (compObj.find("\"releaseMs\"") != std::string::npos)
            cfg.compressor.releaseMs = std::max(0.01f, extractFloatValue(compObj, "releaseMs"));
        if (compObj.find("\"sidechainFreqHz\"") != std::string::npos)
            cfg.compressor.sidechainFreqHz = extractFloatValue(compObj, "sidechainFreqHz");
        if (compObj.find("\"makeupGainDb\"") != std::string::npos)
            cfg.compressor.makeupGainDb = extractFloatValue(compObj, "makeupGainDb");
        if (compObj.find("\"volumeDb\"") != std::string::npos)
            cfg.compressor.volumeDb = extractFloatValue(compObj, "volumeDb");
        if (compObj.find("\"preGainDb\"") != std::string::npos)
            cfg.compressor.preGainDb = extractFloatValue(compObj, "preGainDb");
        if (compObj.find("\"kneeDb\"") != std::string::npos)
            cfg.compressor.kneeDb = extractFloatValue(compObj, "kneeDb");
        if (compObj.find("\"expansionRatio\"") != std::string::npos)
            cfg.compressor.expansionRatio = std::max(1.0f, extractFloatValue(compObj, "expansionRatio"));
        if (compObj.find("\"gateThresholdDb\"") != std::string::npos)
            cfg.compressor.gateThresholdDb = extractFloatValue(compObj, "gateThresholdDb");
    }

    std::string revObj = extractObject(content, "reverb");
    if (!revObj.empty()) {
        cfg.reverb.loaded = true;
        cfg.reverb.enabled = extractBoolValue(revObj, "enabled", true);
        if (revObj.find("\"decayTime\"") != std::string::npos)
            cfg.reverb.decayTime = extractFloatValue(revObj, "decayTime");
        if (revObj.find("\"hiRatio\"") != std::string::npos)
            cfg.reverb.hiRatio = extractFloatValue(revObj, "hiRatio");
        if (revObj.find("\"diffusion\"") != std::string::npos)
            cfg.reverb.diffusion = extractFloatValue(revObj, "diffusion");
        if (revObj.find("\"initialDelay\"") != std::string::npos)
            cfg.reverb.initialDelay = extractFloatValue(revObj, "initialDelay");
        if (revObj.find("\"density\"") != std::string::npos)
            cfg.reverb.density = extractFloatValue(revObj, "density");
        if (revObj.find("\"lpfFreq\"") != std::string::npos)
            cfg.reverb.lpfFreq = extractFloatValue(revObj, "lpfFreq");
        if (revObj.find("\"hpfFreq\"") != std::string::npos)
            cfg.reverb.hpfFreq = extractFloatValue(revObj, "hpfFreq");
        if (revObj.find("\"reverbDelay\"") != std::string::npos)
            cfg.reverb.reverbDelay = extractFloatValue(revObj, "reverbDelay");
        if (revObj.find("\"balance\"") != std::string::npos)
            cfg.reverb.balance = extractFloatValue(revObj, "balance");
    }

    std::string xoverObj = extractObject(content, "crossover");
    if (!xoverObj.empty()) {
        cfg.crossover.loaded = true;
        cfg.crossover.enabled = extractBoolValue(xoverObj, "enabled", true);
        cfg.crossover.lpfEnabled = extractBoolValue(xoverObj, "lpfEnabled", false);
        if (xoverObj.find("\"lowFreq\"") != std::string::npos)
            cfg.crossover.lowFreq = extractFloatValue(xoverObj, "lowFreq");
        if (xoverObj.find("\"highFreq\"") != std::string::npos)
            cfg.crossover.highFreq = extractFloatValue(xoverObj, "highFreq");
        if (xoverObj.find("\"hpfSlope\"") != std::string::npos)
            cfg.crossover.hpfSlope = extractIntValue(xoverObj, "hpfSlope");
        if (xoverObj.find("\"lpfSlope\"") != std::string::npos)
            cfg.crossover.lpfSlope = extractIntValue(xoverObj, "lpfSlope");
        // Backwards compat: single "slope" sets both
        if (xoverObj.find("\"slope\"") != std::string::npos &&
            xoverObj.find("\"hpfSlope\"") == std::string::npos) {
            int s = extractIntValue(xoverObj, "slope");
            cfg.crossover.hpfSlope = s;
            cfg.crossover.lpfSlope = s;
        }
        if (xoverObj.find("\"subGainDb\"") != std::string::npos)
            cfg.crossover.subGainDb = extractFloatValue(xoverObj, "subGainDb");
    }

    std::string blObj = extractObject(content, "bandLimiter");
    if (!blObj.empty()) {
        cfg.bandLimiter.loaded = true;
        cfg.bandLimiter.enabled = extractBoolValue(blObj, "enabled", false);
        size_t entriesStart = blObj.find("\"entries\"");
        if (entriesStart != std::string::npos) {
            entriesStart = blObj.find('[', entriesStart);
            if (entriesStart != std::string::npos) {
                size_t entriesEnd = blObj.find(']', entriesStart);
                if (entriesEnd != std::string::npos) {
                    std::string arr = blObj.substr(entriesStart + 1, entriesEnd - entriesStart - 1);
                    size_t pos = 0;
                    while (pos < arr.size()) {
                        size_t objS = arr.find('{', pos);
                        if (objS == std::string::npos) break;
                        size_t objE = arr.find('}', objS);
                        if (objE == std::string::npos) break;
                        std::string obj = arr.substr(objS, objE - objS + 1);
                        BandLimiterEntryConfig ec;
                        ec.active = extractBoolValue(obj, "active", false);
                        ec.lowFreq = extractFloatValue(obj, "lowFreq");
                        ec.highFreq = extractFloatValue(obj, "highFreq");
                        ec.limitDb = extractFloatValue(obj, "limitDb");
                        cfg.bandLimiter.entries.push_back(ec);
                        pos = objE + 1;
                    }
                }
            }
        }
    }

    std::string toneObj = extractObject(content, "tone");
    if (!toneObj.empty()) {
        cfg.tone.loaded = true;
        cfg.tone.bassFreq = extractFloatValue(toneObj, "bassFreq");
        cfg.tone.bassQ = extractFloatValue(toneObj, "bassQ");
        cfg.tone.bassGainDb = extractFloatValue(toneObj, "bassGainDb");
        cfg.tone.bassEnabled = extractBoolValue(toneObj, "bassEnabled", true);
        cfg.tone.trebleFreq = extractFloatValue(toneObj, "trebleFreq");
        cfg.tone.trebleQ = extractFloatValue(toneObj, "trebleQ");
        cfg.tone.trebleGainDb = extractFloatValue(toneObj, "trebleGainDb");
        cfg.tone.trebleEnabled = extractBoolValue(toneObj, "trebleEnabled", true);
    }

    std::string mbObj = extractObject(content, "multiband");
    if (!mbObj.empty()) {
        cfg.multiband.loaded = true;
        cfg.multiband.enabled = extractBoolValue(mbObj, "enabled", false);
        cfg.multiband.autoBalance = extractBoolValue(mbObj, "autoBalance", true);
        if (mbObj.find("\"autoBalanceSpeed\"") != std::string::npos)
            cfg.multiband.autoBalanceSpeed = extractFloatValue(mbObj, "autoBalanceSpeed");
        if (mbObj.find("\"compression\"") != std::string::npos)
            cfg.multiband.compression = extractFloatValue(mbObj, "compression");
        if (mbObj.find("\"outputGain\"") != std::string::npos)
            cfg.multiband.outputGain = extractFloatValue(mbObj, "outputGain");
        if (mbObj.find("\"exciterAmount\"") != std::string::npos)
            cfg.multiband.exciterAmount = extractFloatValue(mbObj, "exciterAmount");
        if (mbObj.find("\"subBassBoost\"") != std::string::npos)
            cfg.multiband.subBassBoost = extractFloatValue(mbObj, "subBassBoost");
        if (mbObj.find("\"subBassLowFreq\"") != std::string::npos)
            cfg.multiband.subBassLowFreq = extractFloatValue(mbObj, "subBassLowFreq");
        if (mbObj.find("\"subBassHighFreq\"") != std::string::npos)
            cfg.multiband.subBassHighFreq = extractFloatValue(mbObj, "subBassHighFreq");
    }

    std::string devObj = extractObject(content, "devices");
    if (!devObj.empty()) {
        cfg.devices.loaded = true;
        cfg.devices.captureFrom = extractStringValue(devObj, "captureFrom");
        cfg.devices.playTo = extractStringValue(devObj, "playTo");
    }

    std::string audioObj = extractObject(content, "audio");
    if (!audioObj.empty()) {
        cfg.audio.loaded = true;
        cfg.audio.blockSize = extractIntValue(audioObj, "blockSize");
        if (cfg.audio.blockSize < 64) cfg.audio.blockSize = 64;
        if (cfg.audio.blockSize > 16384) cfg.audio.blockSize = 16384;
    }

    return cfg;
}

inline std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

inline bool saveConfig(const std::string& path, const AppConfig& cfg) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "\t\"name\": \"" << jsonEscape(cfg.eq.name) << "\",\n";
    file << "\t\"preamp\": " << cfg.eq.preamp << ",\n";
    file << "\t\"parametric\": true,\n";

    file << "\t\"bands\": [\n";
    for (size_t i = 0; i < cfg.eq.bands.size(); i++) {
        const auto& b = cfg.eq.bands[i];
        file << "\t\t{";
        file << " \"type\": " << b.type;
        file << ", \"channels\": 0";
        file << ", \"frequency\": " << b.frequency;
        file << ", \"q\": " << b.q;
        file << ", \"gain\": " << b.gain;
        file << ", \"color\": 0";
        file << " }";
        if (i + 1 < cfg.eq.bands.size()) file << ",";
        file << "\n";
    }
    file << "\t],\n";

    file << "\t\"compressor\": {\n";
    file << "\t\t\"enabled\": " << (cfg.compressor.enabled ? "true" : "false") << ",\n";
    file << "\t\t\"threshold\": " << cfg.compressor.thresholdDb << ",\n";
    file << "\t\t\"ratio\": " << cfg.compressor.ratio << ",\n";
    file << "\t\t\"attackMs\": " << cfg.compressor.attackMs << ",\n";
    file << "\t\t\"releaseMs\": " << cfg.compressor.releaseMs << ",\n";
    file << "\t\t\"sidechainFreqHz\": " << cfg.compressor.sidechainFreqHz << ",\n";
    file << "\t\t\"makeupGainDb\": " << cfg.compressor.makeupGainDb << ",\n";
    file << "\t\t\"volumeDb\": " << cfg.compressor.volumeDb << ",\n";
    file << "\t\t\"preGainDb\": " << cfg.compressor.preGainDb << ",\n";
    file << "\t\t\"kneeDb\": " << cfg.compressor.kneeDb << ",\n";
    file << "\t\t\"expansionRatio\": " << cfg.compressor.expansionRatio << ",\n";
    file << "\t\t\"gateThresholdDb\": " << cfg.compressor.gateThresholdDb << "\n";
    file << "\t},\n";

    file << "\t\"reverb\": {\n";
    file << "\t\t\"enabled\": " << (cfg.reverb.enabled ? "true" : "false") << ",\n";
    file << "\t\t\"decayTime\": " << cfg.reverb.decayTime << ",\n";
    file << "\t\t\"hiRatio\": " << cfg.reverb.hiRatio << ",\n";
    file << "\t\t\"diffusion\": " << cfg.reverb.diffusion << ",\n";
    file << "\t\t\"initialDelay\": " << cfg.reverb.initialDelay << ",\n";
    file << "\t\t\"density\": " << cfg.reverb.density << ",\n";
    file << "\t\t\"lpfFreq\": " << cfg.reverb.lpfFreq << ",\n";
    file << "\t\t\"hpfFreq\": " << cfg.reverb.hpfFreq << ",\n";
    file << "\t\t\"reverbDelay\": " << cfg.reverb.reverbDelay << ",\n";
    file << "\t\t\"balance\": " << cfg.reverb.balance << "\n";
    file << "\t},\n";

    file << "\t\"crossover\": {\n";
    file << "\t\t\"enabled\": " << (cfg.crossover.enabled ? "true" : "false") << ",\n";
    file << "\t\t\"lpfEnabled\": " << (cfg.crossover.lpfEnabled ? "true" : "false") << ",\n";
    file << "\t\t\"lowFreq\": " << cfg.crossover.lowFreq << ",\n";
    file << "\t\t\"highFreq\": " << cfg.crossover.highFreq << ",\n";
    file << "\t\t\"hpfSlope\": " << cfg.crossover.hpfSlope << ",\n";
    file << "\t\t\"lpfSlope\": " << cfg.crossover.lpfSlope << ",\n";
    file << "\t\t\"subGainDb\": " << cfg.crossover.subGainDb << "\n";
    file << "\t},\n";

    file << "\t\"bandLimiter\": {\n";
    file << "\t\t\"enabled\": " << (cfg.bandLimiter.enabled ? "true" : "false") << ",\n";
    file << "\t\t\"entries\": [\n";
    for (size_t i = 0; i < cfg.bandLimiter.entries.size(); i++) {
        const auto& e = cfg.bandLimiter.entries[i];
        file << "\t\t\t{";
        file << " \"active\": " << (e.active ? "true" : "false");
        file << ", \"lowFreq\": " << e.lowFreq;
        file << ", \"highFreq\": " << e.highFreq;
        file << ", \"limitDb\": " << e.limitDb;
        file << " }";
        if (i + 1 < cfg.bandLimiter.entries.size()) file << ",";
        file << "\n";
    }
    file << "\t\t]\n";
    file << "\t},\n";

    file << "\t\"tone\": {\n";
    file << "\t\t\"bassFreq\": " << cfg.tone.bassFreq << ",\n";
    file << "\t\t\"bassQ\": " << cfg.tone.bassQ << ",\n";
    file << "\t\t\"bassGainDb\": " << cfg.tone.bassGainDb << ",\n";
    file << "\t\t\"bassEnabled\": " << (cfg.tone.bassEnabled ? "true" : "false") << ",\n";
    file << "\t\t\"trebleFreq\": " << cfg.tone.trebleFreq << ",\n";
    file << "\t\t\"trebleQ\": " << cfg.tone.trebleQ << ",\n";
    file << "\t\t\"trebleGainDb\": " << cfg.tone.trebleGainDb << ",\n";
    file << "\t\t\"trebleEnabled\": " << (cfg.tone.trebleEnabled ? "true" : "false") << "\n";
    file << "\t},\n";

    file << "\t\"multiband\": {\n";
    file << "\t\t\"enabled\": " << (cfg.multiband.enabled ? "true" : "false") << ",\n";
    file << "\t\t\"autoBalance\": " << (cfg.multiband.autoBalance ? "true" : "false") << ",\n";
    file << "\t\t\"autoBalanceSpeed\": " << cfg.multiband.autoBalanceSpeed << ",\n";
    file << "\t\t\"compression\": " << cfg.multiband.compression << ",\n";
    file << "\t\t\"outputGain\": " << cfg.multiband.outputGain << ",\n";
    file << "\t\t\"exciterAmount\": " << cfg.multiband.exciterAmount << ",\n";
    file << "\t\t\"subBassBoost\": " << cfg.multiband.subBassBoost << ",\n";
    file << "\t\t\"subBassLowFreq\": " << cfg.multiband.subBassLowFreq << ",\n";
    file << "\t\t\"subBassHighFreq\": " << cfg.multiband.subBassHighFreq << "\n";
    file << "\t},\n";

    file << "\t\"devices\": {\n";
    file << "\t\t\"captureFrom\": \"" << jsonEscape(cfg.devices.captureFrom) << "\",\n";
    file << "\t\t\"playTo\": \"" << jsonEscape(cfg.devices.playTo) << "\"\n";
    file << "\t},\n";

    file << "\t\"audio\": {\n";
    file << "\t\t\"blockSize\": " << cfg.audio.blockSize << "\n";
    file << "\t}\n";

    file << "}\n";
    file.close();
    return true;
}

} // namespace config
