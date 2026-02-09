#pragma once

#include <atomic>
#include <string>
#include <memory>
#include "dsp/dsp_chain.h"
#include "common/params.h"
#include "circular_buffer.h"

struct ma_device;
struct ma_context;

class AudioEngine {
public:
    enum class Status {
        Stopped,
        Starting,
        Running,
        ErrorInit,
        ErrorDevice,
        ErrorFormat
    };

    AudioEngine(DSPChain& dspChain, SharedParams& params);
    ~AudioEngine();

    bool start(int loopbackIdx, int playbackIdx);
    void stop();
    bool isRunning() const { return running_.load(std::memory_order_relaxed); }

    Status getStatus() const { return status_.load(std::memory_order_relaxed); }
    const std::string& getErrorDetail() const { return errorDetail_; }

    float getInputLevelL()  const { return inputLevelL_.load(std::memory_order_relaxed); }
    float getInputLevelR()  const { return inputLevelR_.load(std::memory_order_relaxed); }
    float getOutputLevelL() const { return outputLevelL_.load(std::memory_order_relaxed); }
    float getOutputLevelR() const { return outputLevelR_.load(std::memory_order_relaxed); }
    float getGainReduction() const { return dspChain_.getCompressor().getGainReduction(); }

    int getDebugSampleRate() const { return debugSampleRate_.load(std::memory_order_relaxed); }
    int getDebugChannels() const { return debugChannels_.load(std::memory_order_relaxed); }
    uint64_t getDebugFrameCount() const { return debugFrameCount_.load(std::memory_order_relaxed); }

    static const char* statusToString(Status s);

private:
    static void captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);
    static void playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);

    DSPChain& dspChain_;
    SharedParams& params_;

    ma_context* pContext_ = nullptr;
    ma_device* pCaptureDevice_ = nullptr;
    ma_device* pPlaybackDevice_ = nullptr;
    std::unique_ptr<CircularBuffer<float>> ringBuffer_;

    std::atomic<bool> running_{false};
    std::atomic<Status> status_{Status::Stopped};

    std::atomic<float> inputLevelL_{0.0f};
    std::atomic<float> inputLevelR_{0.0f};
    std::atomic<float> outputLevelL_{0.0f};
    std::atomic<float> outputLevelR_{0.0f};

    std::atomic<int> debugSampleRate_{0};
    std::atomic<int> debugChannels_{0};
    std::atomic<uint64_t> debugFrameCount_{0};

    std::string errorDetail_;
};
