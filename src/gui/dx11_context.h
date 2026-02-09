#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

class DX11Context {
public:
    bool init(HWND hWnd, int width, int height);
    void shutdown();
    void resize(int width, int height);

    void beginFrame();
    void endFrame();

    ID3D11Device*           getDevice()       { return pd3dDevice_; }
    ID3D11DeviceContext*    getDeviceContext() { return pd3dContext_; }
    ID3D11RenderTargetView* getRenderTarget() { return pRenderTarget_; }

private:
    void createRenderTarget();
    void cleanupRenderTarget();

    ID3D11Device*            pd3dDevice_ = nullptr;
    ID3D11DeviceContext*     pd3dContext_ = nullptr;
    IDXGISwapChain*          pSwapChain_ = nullptr;
    ID3D11RenderTargetView*  pRenderTarget_ = nullptr;
};
