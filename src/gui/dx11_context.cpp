#include "dx11_context.h"

bool DX11Context::init(HWND hWnd, int width, int height) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevels, 2, D3D11_SDK_VERSION,
        &sd, &pSwapChain_, &pd3dDevice_, &featureLevel, &pd3dContext_
    );

    if (FAILED(hr)) return false;

    createRenderTarget();
    return true;
}

void DX11Context::shutdown() {
    cleanupRenderTarget();
    if (pSwapChain_)  { pSwapChain_->Release();  pSwapChain_ = nullptr; }
    if (pd3dContext_) { pd3dContext_->Release(); pd3dContext_ = nullptr; }
    if (pd3dDevice_)  { pd3dDevice_->Release();  pd3dDevice_ = nullptr; }
}

void DX11Context::resize(int width, int height) {
    if (!pSwapChain_) return;
    cleanupRenderTarget();
    pSwapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    createRenderTarget();
}

void DX11Context::beginFrame() {
    float clearColor[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
    pd3dContext_->OMSetRenderTargets(1, &pRenderTarget_, nullptr);
    pd3dContext_->ClearRenderTargetView(pRenderTarget_, clearColor);
}

void DX11Context::endFrame() {
    pSwapChain_->Present(1, 0); // VSync
}

void DX11Context::createRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer) {
        pd3dDevice_->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTarget_);
        pBackBuffer->Release();
    }
}

void DX11Context::cleanupRenderTarget() {
    if (pRenderTarget_) {
        pRenderTarget_->Release();
        pRenderTarget_ = nullptr;
    }
}
