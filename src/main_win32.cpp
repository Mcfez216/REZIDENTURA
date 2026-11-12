// main_win32.cpp - the Windows program: opens the window (Win32), draws with DirectX 11, and calls
// App::frame() once per frame. Everything about the game itself is in ui.cpp and sim_*.cpp.
// Adapted from Dear ImGui's example_win32_directx11.
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ui.h"
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <windows.h>

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static bool g_occluded = false;
static UINT g_resizeW = 0, g_resizeH = 0;
static int g_wake = 0;                     // frames still to draw before the program may idle

static void createRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_dev->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
static void cleanupRenderTarget() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }

static bool createDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof sd);
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL level;
    const D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT r = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &level, &g_ctx);
    if (r == DXGI_ERROR_UNSUPPORTED)        // no graphics card (remote desktop, virtual machine): use the software renderer
        r = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &level, &g_ctx);
    if (r != S_OK) return false;
    createRenderTarget();
    return true;
}
static void cleanupDevice() {
    cleanupRenderTarget();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    g_wake = 4;
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) return 0;
            g_resizeW = (UINT)LOWORD(lParam);
            g_resizeH = (UINT)HIWORD(lParam);
            return 0;
        case WM_GETMINMAXINFO: { MINMAXINFO* m = (MINMAXINFO*)lParam; m->ptMinTrackSize.x = 1100; m->ptMinTrackSize.y = 650; return 0; }
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) return 0;       // no ALT application menu
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static std::string utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s((size_t)n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, wndProc, 0L, 0L, hInst, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"RezidenturaWindow", nullptr};
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Rezidentura", WS_OVERLAPPEDWINDOW, 60, 40, 1500, 900, nullptr, nullptr, wc.hInstance, nullptr);
    if (!createDevice(hwnd)) {
        cleanupDevice();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        MessageBoxW(nullptr, L"DirectX 11 could not be started on this computer.", L"Rezidentura", MB_ICONERROR);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;               // the layout is fixed; nothing to remember between runs
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // --- where the program lives: saves and the manual sit beside it
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dirW(exe);
    dirW = dirW.substr(0, dirW.find_last_of(L"\\/"));
    std::string dir = utf8(dirW);

    // --- a typewriter face from Windows itself: Consolas, else Courier New, else ImGui's built-in font
    float scale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    float px = std::floor(16.f * scale);
    wchar_t win[MAX_PATH];
    GetWindowsDirectoryW(win, MAX_PATH);
    bool fontOk = false;
    for (const wchar_t* name : {L"\\Fonts\\consola.ttf", L"\\Fonts\\cour.ttf", L"\\Fonts\\lucon.ttf"}) {
        std::wstring path = std::wstring(win) + name;
        if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        if (io.Fonts->AddFontFromFileTTF(utf8(path).c_str(), px)) { fontOk = true; break; }
    }
    if (!fontOk) { io.Fonts->AddFontDefault(); px = 13; }

    App app;
    app.S = px / 15.f;
    app.theme();
    app.saveDir = dir + "\\saves";
    {
        std::ifstream f(std::filesystem::path(dirW + L"\\manual.txt"), std::ios::binary);
        if (f) app.manual.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    bool done = false;
    while (!done) {
        // When nothing is happening, sleep until Windows has something for us instead of redrawing 60 times a second.
        if (g_wake <= 0) MsgWaitForMultipleObjects(0, nullptr, FALSE, 250, QS_ALLINPUT);
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        if (g_occluded && g_swap->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) { Sleep(10); continue; }
        g_occluded = false;
        if (g_resizeW != 0 && g_resizeH != 0) {
            cleanupRenderTarget();
            g_swap->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN, 0);
            g_resizeW = g_resizeH = 0;
            createRenderTarget();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.frame();
        ImGui::Render();
        const float clear[4] = {0.86f, 0.84f, 0.77f, 1.0f};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_swap->Present(1, 0);
        g_occluded = (hr == DXGI_STATUS_OCCLUDED);
        if (g_wake > 0) g_wake--;
        if (io.WantTextInput) g_wake = 2;   // keep the text cursor blinking
        if (app.wantQuit) done = true;
    }
    if (app.inGame && app.sim.g.phase == PH_PLAY) app.autosave();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanupDevice();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
