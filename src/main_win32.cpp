// main_win32.cpp - the Windows program: opens the window (Win32), draws with DirectX 11, and calls
// App::frame() once per frame. Everything about the game itself is in ui.cpp and sim_*.cpp.
// Adapted from Dear ImGui's example_win32_directx11 (docking branch: screens lifted off the desk
// become real windows of their own once they leave the main one, so they can go to a second monitor).
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ui.h"
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <knownfolders.h>
#include <shellapi.h>
#include <shlobj.h>

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

static App* g_app = nullptr;
static bool g_launcher = true;               // the window is the small launcher, not the desk

static LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    g_wake = 4;
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_NCHITTEST: {
            // the launcher has no frame: it is dragged by its top strip, wherever nothing there is a button
            LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
            if (hit == HTCLIENT && g_launcher && g_app) {
                POINT p = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
                ScreenToClient(hWnd, &p);
                if (g_app->launcherDragAt((float)p.x, (float)p.y)) return HTCAPTION;
            }
            return hit;
        }
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) return 0;
            g_resizeW = (UINT)LOWORD(lParam);
            g_resizeH = (UINT)HIWORD(lParam);
            return 0;
        case WM_GETMINMAXINFO: if (!g_launcher) { MINMAXINFO* m = (MINMAXINFO*)lParam; m->ptMinTrackSize.x = 1100; m->ptMinTrackSize.y = 650; return 0; } break;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) return 0;       // no ALT application menu
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static std::wstring wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w((size_t)n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

/** Full screen without borders, on whichever monitor the window is on - and back again. */
static void setFullscreen(HWND hwnd, bool on) {
    static WINDOWPLACEMENT before = {sizeof(WINDOWPLACEMENT)};
    DWORD style = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
    if (on) {
        MONITORINFO mi = {sizeof(mi)};
        if (!GetWindowPlacement(hwnd, &before) || !GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) return;
        SetWindowLongW(hwnd, GWL_STYLE, (LONG)(style & ~(DWORD)(WS_OVERLAPPEDWINDOW | WS_MAXIMIZE)));      // (the placement remembers "maximised")
        SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLongW(hwnd, GWL_STYLE, (LONG)(style | WS_OVERLAPPEDWINDOW));
        SetWindowPlacement(hwnd, &before);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

/** The launcher: a small frameless window in the middle of the screen. The desk: an ordinary window, maximised. */
static void setLauncher(HWND hwnd, bool on, ImVec2 size) {
    if (on) {
        ShowWindow(hwnd, SW_RESTORE);
        SetWindowLongW(hwnd, GWL_STYLE, (LONG)(WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE));
        MONITORINFO mi = {sizeof(mi)};
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        int w = (int)size.x, h = (int)size.y;
        int x = (mi.rcWork.left + mi.rcWork.right - w) / 2, y = (mi.rcWork.top + mi.rcWork.bottom - h) / 2;
        SetWindowPos(hwnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        // (sized to the work area first, so that the desk fills the screen even where "maximise" is not honoured)
        MONITORINFO mi = {sizeof(mi)};
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        SetWindowLongW(hwnd, GWL_STYLE, (LONG)(WS_OVERLAPPEDWINDOW | WS_VISIBLE));
        SetWindowPos(hwnd, nullptr, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    }
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
    // the icon: resource 1 in rezidentura.rc, in the large and the small size
    HICON big = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    HICON small = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, wndProc, 0L, 0L, hInst, big ? big : LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"RezidenturaWindow", small};
    RegisterClassExW(&wc);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;               // set below, once we know where the saved games are
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // windows may be dropped on one another to share a frame
    io.ConfigWindowsMoveFromTitleBarOnly = true;           // a window moves by its title bar, not by a stray drag on a document
    io.ConfigViewportsNoTaskBarIcon = true;                // one program, one button on the taskbar
    io.ConfigDockingTransparentPayload = true;

    // --- where the program lives: saves and the manual sit beside it
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dirW(exe);
    dirW = dirW.substr(0, dirW.find_last_of(L"\\/"));
    std::string dir = utf8(dirW);

    App app;
    // saved games go to the user's "Saved Games" folder; a file called portable.txt beside the program keeps them beside it instead
    app.saveDir = dir + "\\saves";
    if (GetFileAttributesW((dirW + L"\\portable.txt").c_str()) == INVALID_FILE_ATTRIBUTES) {
        PWSTR known = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &known)) && known) {
            app.saveDir = utf8(std::wstring(known) + L"\\Rezidentura");
            CoTaskMemFree(known);
        }
    }
    app.loadSettings();
    app.iniPath = app.saveDir + "\\windows.ini";
    std::filesystem::create_directories(std::filesystem::path(wide(app.saveDir)));
    app.viewportsOk = true;
    app.openFile = [](const std::string& path) { ShellExecuteW(nullptr, L"open", wide(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL); };
    if (GetFileAttributesW((dirW + L"\\Officer's Handbook.pdf").c_str()) != INVALID_FILE_ATTRIBUTES) app.pdfPath = dir + "\\Officer's Handbook.pdf";

    // --- how big everything is drawn: the desk is laid out for 1600 x 1000 pixels and scaled to the screen
    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY), &mi);
    float sw = (float)(mi.rcWork.right - mi.rcWork.left), sh = (float)(mi.rcWork.bottom - mi.rcWork.top);
    float scale = std::min(sw / 1600.f, (sh - 30) / 1000.f);
    scale = std::max(0.6f, std::min(2.6f, scale * app.cfg.uiScale));      // "Size of everything" in the settings
    app.S = scale;
    g_app = &app;
    // --- the window: it opens as the launcher, and becomes the desk when a career is begun or loaded
    ImVec2 ls = app.launcherSize();
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Rezidentura", WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX, (mi.rcWork.left + mi.rcWork.right - (int)ls.x) / 2,
                              (mi.rcWork.top + mi.rcWork.bottom - (int)ls.y) / 2, (int)ls.x, (int)ls.y, nullptr, nullptr, wc.hInstance, nullptr);
    if (!createDevice(hwnd)) {
        cleanupDevice();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        MessageBoxW(nullptr, L"DirectX 11 could not be started on this computer.", L"Rezidentura", MB_ICONERROR);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    app.theme();
    // --- the game's own typefaces (in the fonts folder beside the program); Consolas if they are missing
    wchar_t win[MAX_PATH];
    GetWindowsDirectoryW(win, MAX_PATH);
    gfx::loadFonts(dir + "\\fonts", scale, utf8(std::wstring(win) + L"\\Fonts\\consola.ttf"));
    // --- pictures: the program paints them, DirectX keeps them
    gfx::texApi.create = [](const Img& im) -> ImTextureID {
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = (UINT)im.w; td.Height = (UINT)im.h; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = im.px.data(); sd.SysMemPitch = (UINT)im.w * 4;
        ID3D11Texture2D* t = nullptr;
        if (im.w <= 0 || im.h <= 0 || FAILED(g_dev->CreateTexture2D(&td, &sd, &t))) return (ImTextureID)0;
        D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
        vd.Format = DXGI_FORMAT_R8G8B8A8_UNORM; vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; vd.Texture2D.MipLevels = 1;
        ID3D11ShaderResourceView* v = nullptr;
        g_dev->CreateShaderResourceView(t, &vd, &v);
        t->Release();
        return (ImTextureID)v;
    };
    gfx::texApi.destroy = [](ImTextureID id) { if (id) ((ID3D11ShaderResourceView*)id)->Release(); };

    {
        std::ifstream f(std::filesystem::path(dirW + L"\\manual.txt"), std::ios::binary);
        if (f) app.manual.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    bool done = false, full = false;
    std::string iniNow;
    while (!done) {
        // When nothing is happening, sleep until Windows has something for us instead of redrawing 60 times a second.
        if (g_wake <= 0) MsgWaitForMultipleObjects(0, nullptr, FALSE, 250, QS_ALLINPUT);
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            g_wake = 4;                     // (a window on another monitor has its own procedure; anything at all wakes us)
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        // Minimised or covered: nothing to draw - unless a screen is in a window of its own somewhere, which still wants drawing.
        if (g_occluded && ImGui::GetPlatformIO().Viewports.Size <= 1 && g_swap->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) { Sleep(10); continue; }
        g_occluded = false;
        if (g_resizeW != 0 && g_resizeH != 0) {
            cleanupRenderTarget();
            g_swap->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN, 0);
            g_resizeW = g_resizeH = 0;
            createRenderTarget();
        }
        // launcher or desk: the window follows the game
        if (g_launcher != !app.inGame) {
            if (full) { setFullscreen(hwnd, false); full = false; }
            g_launcher = !app.inGame;
            setLauncher(hwnd, g_launcher, app.launcherSize());
        }
        if (app.wantMinimise) { app.wantMinimise = false; ShowWindow(hwnd, SW_MINIMIZE); }
        // the settings that touch the window system, followed as they change
        if (app.cfg.multiMonitor) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; else io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        std::string wantIni = app.cfg.rememberWindows ? app.iniPath : std::string();
        if (wantIni != iniNow) {
            iniNow = wantIni;
            io.IniFilename = iniNow.empty() ? nullptr : iniNow.c_str();      // (read at the first frame, written as things move)
        }
        if ((app.cfg.fullscreen && !g_launcher) != full) { full = app.cfg.fullscreen && !g_launcher; setFullscreen(hwnd, full); }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.frame();
        ImGui::Render();
        const float clear[4] = {0.16f, 0.12f, 0.09f, 1.0f};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {       // the windows that have left the main one
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
        HRESULT hr = g_swap->Present(1, 0);
        g_occluded = (hr == DXGI_STATUS_OCCLUDED);
        if (g_wake > 0) g_wake--;
        if (io.WantTextInput) g_wake = 2;   // keep the text cursor blinking
        if (app.wantQuit) done = true;
    }
    if (app.inGame && app.sim.g.phase == PH_PLAY) app.autosave();
    app.saveSettings();
    gfx::forgetTextures("");

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanupDevice();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
