#define MA_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#include "miniaudio.h"

#include "audio_engine.h"
#include "dsp/dsp_common.h"
#include <cmath>
#include <cstring>
#include <algorithm>

const char* AudioEngine::statusToString(Status s) {
    switch (s) {
        case Status::Stopped:     return "Stopped";
        case Status::Starting:    return "Starting...";
        case Status::Running:     return "Running";
        case Status::ErrorInit:   return "Error: Init failed";
        case Status::ErrorDevice: return "Error: Device failed";
        case Status::ErrorFormat: return "Error: Format failed";
    }
    return "Unknown";
}

AudioEngine::AudioEngine(DSPChain& dspChain, SharedParams& params)
    : dspChain_(dspChain), params_(params) {}

AudioEngine::~AudioEngine() { stop(); }

void AudioEngine::captureCallback(ma_device* pDevice, void* pOutput,
                                   const void* pInput, unsigned int frameCount) {
    AudioEngine* self = (AudioEngine*)pDevice->pUserData;
    if (!self || !pInput || frameCount == 0) return;

    const float* in = (const float*)pInput;
    int nCh = (int)pDevice->capture.channels;
    float sr = (float)pDevice->sampleRate;
    size_t totalSamples = (size_t)frameCount * nCh;

    float peakL = 0.0f, peakR = 0.0f;
    for (unsigned int i = 0; i < frameCount; i += 32) {
        float l = std::abs(in[i * nCh]);
        if (l > peakL) peakL = l;
        if (nCh > 1) {
            float r = std::abs(in[i * nCh + 1]);
            if (r > peakR) peakR = r;
        }
    }

    const float decay = 0.98f;
    float currentInL = self->inputLevelL_.load(std::memory_order_relaxed);
    float currentInR = self->inputLevelR_.load(std::memory_order_relaxed);
    currentInL = std::max(peakL, currentInL * decay);
    currentInR = std::max(peakR, currentInR * decay);
    self->inputLevelL_.store(currentInL, std::memory_order_relaxed);
    self->inputLevelR_.store(currentInR, std::memory_order_relaxed);

    float stackBuf[8192];
    float* buf = stackBuf;
    bool heap = false;
    if (totalSamples > 8192) {
        buf = new float[totalSamples];
        heap = true;
    }
    std::memcpy(buf, in, totalSamples * sizeof(float));
    self->dspChain_.process(buf, frameCount, nCh, sr);

    float peakOutL = 0.0f, peakOutR = 0.0f;
    for (unsigned int i = 0; i < frameCount; i += 32) {
        float l = std::abs(buf[i * nCh]);
        if (l > peakOutL) peakOutL = l;
        if (nCh > 1) {
            float r = std::abs(buf[i * nCh + 1]);
            if (r > peakOutR) peakOutR = r;
        }
    }

    float currentOutL = self->outputLevelL_.load(std::memory_order_relaxed);
    float currentOutR = self->outputLevelR_.load(std::memory_order_relaxed);
    currentOutL = std::max(peakOutL, currentOutL * decay);
    currentOutR = std::max(peakOutR, currentOutR * decay);
    self->outputLevelL_.store(currentOutL, std::memory_order_relaxed);
    self->outputLevelR_.store(currentOutR, std::memory_order_relaxed);

    self->ringBuffer_->write(buf, totalSamples);
    if (heap) delete[] buf;
    self->debugFrameCount_.fetch_add(frameCount, std::memory_order_relaxed);
}

void AudioEngine::playbackCallback(ma_device* pDevice, void* pOutput,
                                    const void* pInput, unsigned int frameCount) {
    AudioEngine* self = (AudioEngine*)pDevice->pUserData;
    float* out = (float*)pOutput;
    int nCh = (int)pDevice->playback.channels;
    size_t totalSamples = (size_t)frameCount * nCh;

    if (!self || !self->ringBuffer_) {
        std::memset(out, 0, totalSamples * sizeof(float));
        return;
    }
    if (!self->ringBuffer_->read(out, totalSamples)) {
        std::memset(out, 0, totalSamples * sizeof(float));
    }
}

bool AudioEngine::start(int loopbackIdx, int playbackIdx) {
    if (running_.load()) return false;
    status_.store(Status::Starting);
    errorDetail_.clear();
    debugFrameCount_.store(0);

    pContext_ = new ma_context;
    ma_backend backends[] = { ma_backend_wasapi };
    ma_context_config ctxCfg = ma_context_config_init();
    ma_result res = ma_context_init(backends, 1, &ctxCfg, pContext_);
    if (res != MA_SUCCESS) {
        errorDetail_ = "WASAPI context init (code " + std::to_string((int)res) + ")";
        delete pContext_; pContext_ = nullptr;
        status_.store(Status::ErrorInit);
        return false;
    }

    ma_device_info* pPlaybackDevices = nullptr;
    ma_uint32 playbackCount = 0;
    ma_device_info* pCaptureDevices = nullptr;
    ma_uint32 captureCount = 0;
    ma_context_get_devices(pContext_, &pPlaybackDevices, &playbackCount,
                                      &pCaptureDevices, &captureCount);

    ringBuffer_ = std::make_unique<CircularBuffer<float>>(48000 * 2 * 2);

    ma_device_config capCfg = ma_device_config_init(ma_device_type_loopback);
    if (loopbackIdx >= 0 && loopbackIdx < (int)playbackCount) {
        capCfg.playback.pDeviceID = &pPlaybackDevices[loopbackIdx].id;
    }
    capCfg.capture.format     = ma_format_f32;
    capCfg.capture.channels   = 2;
    capCfg.playback.format    = ma_format_f32;
    capCfg.playback.channels  = 2;
    capCfg.sampleRate         = 48000;
    capCfg.periodSizeInFrames = params_.blockSize.load(std::memory_order_relaxed);
    capCfg.dataCallback       = &AudioEngine::captureCallback;
    capCfg.pUserData          = this;
    capCfg.performanceProfile = ma_performance_profile_low_latency;

    pCaptureDevice_ = new ma_device;
    res = ma_device_init(pContext_, &capCfg, pCaptureDevice_);
    if (res != MA_SUCCESS) {
        errorDetail_ = "Loopback init (code " + std::to_string((int)res) + ")";
        delete pCaptureDevice_; pCaptureDevice_ = nullptr;
        ma_context_uninit(pContext_); delete pContext_; pContext_ = nullptr;
        ringBuffer_.reset();
        status_.store(Status::ErrorDevice);
        return false;
    }

    ma_device_config playCfg = ma_device_config_init(ma_device_type_playback);
    if (playbackIdx >= 0 && playbackIdx < (int)playbackCount) {
        playCfg.playback.pDeviceID = &pPlaybackDevices[playbackIdx].id;
    }
    playCfg.playback.format     = ma_format_f32;
    playCfg.playback.channels   = 2;
    playCfg.sampleRate          = 48000;
    playCfg.periodSizeInFrames  = params_.blockSize.load(std::memory_order_relaxed);
    playCfg.dataCallback        = &AudioEngine::playbackCallback;
    playCfg.pUserData           = this;
    playCfg.performanceProfile  = ma_performance_profile_low_latency;

    pPlaybackDevice_ = new ma_device;
    res = ma_device_init(pContext_, &playCfg, pPlaybackDevice_);
    if (res != MA_SUCCESS) {
        errorDetail_ = "Playback init (code " + std::to_string((int)res) + ")";
        ma_device_uninit(pCaptureDevice_);
        delete pCaptureDevice_; pCaptureDevice_ = nullptr;
        delete pPlaybackDevice_; pPlaybackDevice_ = nullptr;
        ma_context_uninit(pContext_); delete pContext_; pContext_ = nullptr;
        ringBuffer_.reset();
        status_.store(Status::ErrorDevice);
        return false;
    }

    debugSampleRate_.store((int)pCaptureDevice_->sampleRate);
    debugChannels_.store((int)pCaptureDevice_->capture.channels);

    res = ma_device_start(pPlaybackDevice_);
    if (res != MA_SUCCESS) {
        errorDetail_ = "Playback start (code " + std::to_string((int)res) + ")";
        ma_device_uninit(pCaptureDevice_);
        ma_device_uninit(pPlaybackDevice_);
        delete pCaptureDevice_; pCaptureDevice_ = nullptr;
        delete pPlaybackDevice_; pPlaybackDevice_ = nullptr;
        ma_context_uninit(pContext_); delete pContext_; pContext_ = nullptr;
        ringBuffer_.reset();
        status_.store(Status::ErrorDevice);
        return false;
    }

    res = ma_device_start(pCaptureDevice_);
    if (res != MA_SUCCESS) {
        errorDetail_ = "Loopback start (code " + std::to_string((int)res) + ")";
        ma_device_stop(pPlaybackDevice_);
        ma_device_uninit(pCaptureDevice_);
        ma_device_uninit(pPlaybackDevice_);
        delete pCaptureDevice_; pCaptureDevice_ = nullptr;
        delete pPlaybackDevice_; pPlaybackDevice_ = nullptr;
        ma_context_uninit(pContext_); delete pContext_; pContext_ = nullptr;
        ringBuffer_.reset();
        status_.store(Status::ErrorDevice);
        return false;
    }

    running_.store(true);
    status_.store(Status::Running);
    return true;
}

void AudioEngine::stop() {
    if (!running_.load()) return;

    if (pCaptureDevice_) {
        ma_device_stop(pCaptureDevice_);
        ma_device_uninit(pCaptureDevice_);
        delete pCaptureDevice_;
        pCaptureDevice_ = nullptr;
    }
    if (pPlaybackDevice_) {
        ma_device_stop(pPlaybackDevice_);
        ma_device_uninit(pPlaybackDevice_);
        delete pPlaybackDevice_;
        pPlaybackDevice_ = nullptr;
    }
    if (pContext_) {
        ma_context_uninit(pContext_);
        delete pContext_;
        pContext_ = nullptr;
    }

    ringBuffer_.reset();
    inputLevelL_.store(0); inputLevelR_.store(0);
    outputLevelL_.store(0); outputLevelR_.store(0);
    running_.store(false);
    status_.store(Status::Stopped);
}
