#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include "ui/menu_root.hpp"
#include "ui/scale.hpp"
#include "ui/state/mod_state.hpp"

#include <cstdint>
#include <string>

namespace il2cpp {
    float get_current_player_speed() { return 0.0f; }
    float get_current_run_distance() { return 0.0f; }
    float get_run_time_seconds()     { return 0.0f; }
    std::uint32_t get_effective_seed() { return 0u; }
}

namespace platform::android {
    bool has_java_vm() { return false; }
    void request_seed_input(const std::string&) {}
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#include <tchar.h>

#include <cmath>

struct WglWindow { HDC hDC; };

static HGLRC g_hRC = nullptr;
static WglWindow g_window{};
static int g_width = 0;
static int g_height = 0;
static ui::menu_root::State g_menu_state{};

static bool create_device_wgl(HWND hWnd, WglWindow* data);
static void cleanup_device_wgl(HWND hWnd, WglWindow* data);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

int main(int, char**) {
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = {sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
                      nullptr, nullptr, nullptr, nullptr, L"MimiMod Desktop Viewer", nullptr};
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"MimiMod // Desktop Viewer",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                nullptr, nullptr, wc.hInstance, nullptr);

    if (!create_device_wgl(hwnd, &g_window)) {
        cleanup_device_wgl(hwnd, &g_window);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_window.hDC, g_hRC);

    float dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    if (dpi < 0.5f) dpi = 1.0f;

    ::SetWindowPos(hwnd, nullptr, 100, 100,
                   static_cast<int>(560 * dpi), static_cast<int>(960 * dpi),
                   SWP_NOZORDER | SWP_NOACTIVATE);
    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    RECT rc; ::GetClientRect(hwnd, &rc);
    g_width = rc.right - rc.left;
    g_height = rc.bottom - rc.top;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ui::scale::set_density(dpi);
    ui::fonts::load_defaults(dpi);
    ui::menu_root::initialize(g_menu_state);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0, 0);
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;

    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init("#version 130");

    float current_dpi = dpi;

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        if (::IsIconic(hwnd)) { ::Sleep(10); continue; }

        RECT live; ::GetClientRect(hwnd, &live);
        const int live_w = live.right - live.left;
        const int live_h = live.bottom - live.top;
        if (live_w > 0 && live_h > 0) { g_width = live_w; g_height = live_h; }

        const float new_dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
        if (new_dpi > 0.5f && std::fabs(new_dpi - current_dpi) > 0.01f) {
            current_dpi = new_dpi;
            ui::scale::set_density(current_dpi);
            ui::fonts::load_defaults(current_dpi);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const float dt = io.DeltaTime;
        io.DisplaySize = ImVec2(static_cast<float>(g_width), static_cast<float>(g_height));
        const ImVec2 vs(static_cast<float>(g_width), static_cast<float>(g_height));

        const float t = static_cast<float>(::GetTickCount64()) * 0.001f;
        const float speed = 15.0f + std::sin(t * 0.7f) * 8.0f;
        ui::state::set_float("hud.live_speed", speed);
        ui::state::set_int("hud.live_dist", static_cast<int>(t * 18.0f));
        ui::state::set_float("hud.live_time", t);

        ui::menu_root::render(g_menu_state, vs, dt);

        ImGui::Render();
        glViewport(0, 0, g_width, g_height);
        glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ::SwapBuffers(g_window.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanup_device_wgl(hwnd, &g_window);
    if (g_hRC) wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

static bool create_device_wgl(HWND hWnd, WglWindow* data) {
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0) return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE) return false;
    ::ReleaseDC(hWnd, hDc);
    data->hDC = ::GetDC(hWnd);
    if (!g_hRC) g_hRC = wglCreateContext(data->hDC);
    return true;
}

static void cleanup_device_wgl(HWND hWnd, WglWindow* data) {
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                g_width = LOWORD(lParam);
                g_height = HIWORD(lParam);
            }
            return 0;
        case WM_DPICHANGED: {
            const RECT* r = reinterpret_cast<const RECT*>(lParam);
            if (r) {
                ::SetWindowPos(hWnd, nullptr, r->left, r->top,
                               r->right - r->left, r->bottom - r->top,
                               SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
