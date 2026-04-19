#include "menu.h"

#include "ui/menu_root.hpp"
#include "ui/scale.hpp"
#include "ui/fonts/fonts.hpp"
#include "core/runtime/runtime_state.hpp"

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

#include <GLES3/gl3.h>

#include <chrono>
#include <atomic>

namespace Menu {

namespace {

using Clock = std::chrono::steady_clock;
Clock::time_point g_last_time;
bool g_imgui_ctx_created = false;
bool g_opengl_initialized = false;
std::atomic<float> g_pending_touch_x{-1.0f};
std::atomic<float> g_pending_touch_y{-1.0f};
std::atomic<int> g_pending_touch_action{-1};
std::atomic<float> g_view_width{1080.0f};
std::atomic<float> g_view_height{1920.0f};
std::atomic<bool> g_wants_capture{false};

bool init_imgui_once(float display_w, float display_h) {
    if (g_imgui_ctx_created) return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.MouseDrawCursor = false;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.DisplaySize = ImVec2(display_w, display_h);

    const float density = (display_w >= 1080.0f) ? (display_w / 440.0f) : 2.5f;
    ::ui::scale::set_density(density);

    ::ui::fonts::load_defaults(density);

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.0f);
    style.WindowPadding = ImVec2(0, 0);
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;

    g_imgui_ctx_created = true;
    g_last_time = Clock::now();
    return true;
}

bool init_opengl_once() {
    if (g_opengl_initialized) return true;
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        return false;
    }
    g_opengl_initialized = true;
    return true;
}

GLuint g_desat_prog = 0;
GLuint g_desat_vao = 0;
GLuint g_desat_tex = 0;
int g_desat_tex_w = 0;
int g_desat_tex_h = 0;
GLint g_desat_loc_amount = -1;
GLint g_desat_loc_tex = -1;
bool g_desat_failed = false;

GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool init_desat() {
    if (g_desat_prog) return true;
    if (g_desat_failed) return false;

    const char* vs_src =
        "#version 300 es\n"
        "out vec2 v_uv;\n"
        "void main() {\n"
        "    vec2 verts[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
        "    vec2 p = verts[gl_VertexID];\n"
        "    v_uv = (p + 1.0) * 0.5;\n"
        "    gl_Position = vec4(p, 0.0, 1.0);\n"
        "}\n";

    const char* fs_src =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D u_tex;\n"
        "uniform float u_amount;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec3 c = texture(u_tex, v_uv).rgb;\n"
        "    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
        "    vec3 gray = vec3(luma);\n"
        "    vec3 desat = mix(c, gray, u_amount);\n"
        "    float darken = mix(1.0, 0.72, u_amount);\n"
        "    float contrast = mix(1.0, 1.18, u_amount);\n"
        "    vec3 outc = (desat - 0.5) * contrast + 0.5;\n"
        "    outc *= darken;\n"
        "    fragColor = vec4(outc, 1.0);\n"
        "}\n";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) {
        g_desat_failed = true;
        return false;
    }
    g_desat_prog = glCreateProgram();
    glAttachShader(g_desat_prog, vs);
    glAttachShader(g_desat_prog, fs);
    glLinkProgram(g_desat_prog);
    GLint linked = 0;
    glGetProgramiv(g_desat_prog, GL_LINK_STATUS, &linked);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!linked) {
        glDeleteProgram(g_desat_prog);
        g_desat_prog = 0;
        g_desat_failed = true;
        return false;
    }
    g_desat_loc_amount = glGetUniformLocation(g_desat_prog, "u_amount");
    g_desat_loc_tex    = glGetUniformLocation(g_desat_prog, "u_tex");

    glGenVertexArrays(1, &g_desat_vao);
    glGenTextures(1, &g_desat_tex);
    glBindTexture(GL_TEXTURE_2D, g_desat_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void apply_desat_pass(float amount, int w, int h) {
    if (amount <= 0.001f || w <= 0 || h <= 0) return;
    if (!init_desat()) return;

    GLint prev_prog = 0;     glGetIntegerv(GL_CURRENT_PROGRAM, &prev_prog);
    GLint prev_vao = 0;      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
    GLint prev_active_tex = 0; glGetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_tex);
    GLint prev_tex2d = 0;    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex2d);
    GLboolean was_blend   = glIsEnabled(GL_BLEND);
    GLboolean was_depth   = glIsEnabled(GL_DEPTH_TEST);
    GLboolean was_cull    = glIsEnabled(GL_CULL_FACE);
    GLboolean was_scissor = glIsEnabled(GL_SCISSOR_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_desat_tex);
    if (w != g_desat_tex_w || h != g_desat_tex_h) {
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, w, h, 0);
        g_desat_tex_w = w;
        g_desat_tex_h = h;
    } else {
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    glUseProgram(g_desat_prog);
    glUniform1i(g_desat_loc_tex, 0);
    glUniform1f(g_desat_loc_amount, amount);
    glBindVertexArray(g_desat_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glBindVertexArray(prev_vao);
    glUseProgram(prev_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, prev_tex2d);
    glActiveTexture(static_cast<GLenum>(prev_active_tex));
    if (was_blend)   glEnable(GL_BLEND);   else glDisable(GL_BLEND);
    if (was_depth)   glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (was_cull)    glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    if (was_scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
}

}

MenuRenderer& MenuRenderer::getInstance() {
    static MenuRenderer instance;
    return instance;
}

MenuRenderer::MenuRenderer() = default;

void MenuRenderer::initialize() {
    if (initialized) return;
    ::ui::menu_root::initialize(menuState);
    initialized = true;
}

void MenuRenderer::shutdown() {
    if (g_opengl_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        g_opengl_initialized = false;
    }
    if (g_imgui_ctx_created) {
        ImGui::DestroyContext();
        g_imgui_ctx_created = false;
    }
    initialized = false;
}

void MenuRenderer::render() {
    if (!initialized) initialize();

    GLint vp[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, vp);
    const float dw = static_cast<float>(vp[2] > 0 ? vp[2] : 1080);
    const float dh = static_cast<float>(vp[3] > 0 ? vp[3] : 1920);

    if (!init_imgui_once(dw, dh)) return;
    if (!init_opengl_once())     return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dw, dh);

    const auto now = Clock::now();
    const float dt = std::chrono::duration<float>(now - g_last_time).count();
    g_last_time = now;
    io.DeltaTime = (dt > 0.0f && dt < 0.2f) ? dt : (1.0f / 60.0f);

    const int act = g_pending_touch_action.exchange(-1);
    const float tx = g_pending_touch_x.load();
    const float ty = g_pending_touch_y.load();
    const float vw = g_view_width.load();
    const float vh = g_view_height.load();
    if (act >= 0 && tx >= 0.0f && ty >= 0.0f && vw > 0.0f && vh > 0.0f) {
        const float scaled_x = (tx / vw) * dw;
        const float scaled_y = (ty / vh) * dh;
        io.AddMousePosEvent(scaled_x, scaled_y);
        if (act == 0 || act == 5) {
            io.AddMouseButtonEvent(0, true);
        } else if (act == 1 || act == 3 || act == 6) {
            io.AddMouseButtonEvent(0, false);
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ::ui::menu_root::render(menuState, io.DisplaySize, io.DeltaTime);

    ImGui::Render();

    g_wants_capture.store(menuState.menu_visible && io.WantCaptureMouse);

    apply_desat_pass(runtime::state().dim_amount.load(), vp[2], vp[3]);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MenuRenderer::toggleVisibility() {
    menuState.menu_visible = !menuState.menu_visible;
}

bool MenuRenderer::isVisible() const {
    return menuState.menu_visible;
}

bool MenuRenderer::wantsCaptureInput() const {
    return g_wants_capture.load();
}

void MenuRenderer::pushTouchEvent(int action, float x, float y, int pointerCount, int viewWidth, int viewHeight) {
    (void)pointerCount;
    if (viewWidth > 0 && viewHeight > 0) {
        g_view_width.store(static_cast<float>(viewWidth));
        g_view_height.store(static_cast<float>(viewHeight));
    }
    g_pending_touch_action.store(action);
    g_pending_touch_x.store(x);
    g_pending_touch_y.store(y);
}

}
