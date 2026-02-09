#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <d3d11.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "gui/dx11_context.h"
#include "gui/gui_manager.h"
#include "audio/audio_engine.h"
#include "audio/audio_device.h"
#include "dsp/dsp_chain.h"
#include "common/params.h"
#include "common/config_loader.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static DX11Context* g_pDX11 = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pDX11 && wParam != SIZE_MINIMIZED) {
            g_pDX11->resize((int)LOWORD(lParam), (int)HIWORD(lParam));
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"AudioEqualizerClass";
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowW(
        wc.lpszClassName, L"Audio Equalizer & Compressor",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1100, 700,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    DX11Context dx11;
    g_pDX11 = &dx11;
    if (!dx11.init(hWnd, 1100, 700)) {
        MessageBoxW(hWnd, L"Failed to initialize DirectX 11", L"Error", MB_OK);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ChildRounding = 4.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(dx11.getDevice(), dx11.getDeviceContext());

    AudioDeviceManager deviceMgr;
    deviceMgr.init();

    SharedParams params;

    AppConfig appConfig;
    {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir(exePath);
        size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) exeDir = exeDir.substr(0, lastSlash + 1);
        else exeDir = ".\\";

        appConfig = config::loadConfig(exeDir + "config.json");
        params.loadFromConfig(appConfig);
    }

    DSPChain dspChain(params);
    AudioEngine audioEngine(dspChain, params);
    GUIManager gui(params, audioEngine, deviceMgr, dspChain);
    gui.init();
    gui.loadFromConfig(appConfig);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        gui.render();

        ImGui::Render();
        dx11.beginFrame();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        dx11.endFrame();
    }

    audioEngine.stop();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    dx11.shutdown();
    g_pDX11 = nullptr;

    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    CoUninitialize();
    return 0;
}
