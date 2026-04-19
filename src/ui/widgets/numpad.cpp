#include "numpad.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fx/animation.hpp"
#include "../state/mod_state.hpp"
#include "../i18n/lang.hpp"

#include <imgui_internal.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace ui::widgets::numpad {

namespace {

struct Pending {
    bool   has = false;
    char   key[96] = {0};
    char   title[48] = {0};
    char   current[24] = {0};
    int    max_len = 10;
};

std::mutex g_lock;
Pending    g_pending;

void copy_str(char* dst, size_t cap, const char* src) {
    if (dst == nullptr || cap == 0) return;
    if (src == nullptr) { dst[0] = '\0'; return; }
    size_t n = std::strlen(src);
    if (n >= cap) n = cap - 1;
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

void draw_filled_btn(ImDrawList* dl, const ImVec2& p, const ImVec2& sz,
                     ImU32 bg, ImU32 border, bool pressed) {
    const ImVec2 shadow_off = pressed ? ImVec2(1.0f, 1.0f)
        : ImVec2(scale::dp(tokens::kShadowOffsetThin), scale::dp(tokens::kShadowOffsetThin));
    dl->AddRectFilled(ImVec2(p.x + shadow_off.x, p.y + shadow_off.y),
                      ImVec2(p.x + sz.x + shadow_off.x, p.y + sz.y + shadow_off.y),
                      tokens::kBlack);
    ImVec2 fp = p;
    if (pressed) { fp.x += 2.0f; fp.y += 2.0f; }
    dl->AddRectFilled(fp, ImVec2(fp.x + sz.x, fp.y + sz.y), bg);
    dl->AddRect(fp, ImVec2(fp.x + sz.x, fp.y + sz.y), border, 0.0f, 0,
                scale::dp(tokens::kBorderThin));
}

bool key_button(const char* id, const ImVec2& p, const ImVec2& sz,
                const char* label, ImFont* font, ImU32 bg, ImU32 fg) {
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton(id, sz);
    const bool active = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    draw_filled_btn(dl, p, sz, bg, tokens::kBlack, active);
    if (font != nullptr && label != nullptr && *label != 0) {
        const float fs = font->LegacySize;
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, label);
        ImVec2 origin = p;
        if (active) { origin.x += 2.0f; origin.y += 2.0f; }
        dl->AddText(font, fs,
                    ImVec2(origin.x + (sz.x - ts.x) * 0.5f,
                           origin.y + (sz.y - ts.y) * 0.5f),
                    fg, label);
    }
    return clicked;
}

}

void open(State& s, const char* state_key, const char* title,
          const char* current, int max_len) {
    s.open = true;
    s.closing = false;
    s.phase = 0.0f;
    copy_str(s.state_key, sizeof(s.state_key), state_key);
    copy_str(s.title, sizeof(s.title), title);
    copy_str(s.buffer, sizeof(s.buffer), current);
    s.max_len = max_len > 0 ? max_len : 10;
    if (s.max_len > static_cast<int>(sizeof(s.buffer)) - 1) {
        s.max_len = static_cast<int>(sizeof(s.buffer)) - 1;
    }
    if (static_cast<int>(std::strlen(s.buffer)) > s.max_len) {
        s.buffer[s.max_len] = '\0';
    }
}

void close(State& s) {
    if (!s.open) return;
    s.closing = true;
}

bool is_visible(const State& s) { return s.open || s.closing || s.phase > 0.0025f; }

void request_open(const char* state_key, const char* title,
                  const char* current, int max_len) {
    std::lock_guard<std::mutex> guard(g_lock);
    g_pending.has = true;
    copy_str(g_pending.key, sizeof(g_pending.key), state_key);
    copy_str(g_pending.title, sizeof(g_pending.title), title);
    copy_str(g_pending.current, sizeof(g_pending.current), current);
    g_pending.max_len = max_len > 0 ? max_len : 10;
}

bool consume_request(char* out_key, size_t key_cap,
                     char* out_title, size_t title_cap,
                     char* out_current, size_t cur_cap,
                     int* out_max_len) {
    std::lock_guard<std::mutex> guard(g_lock);
    if (!g_pending.has) return false;
    g_pending.has = false;
    copy_str(out_key, key_cap, g_pending.key);
    copy_str(out_title, title_cap, g_pending.title);
    copy_str(out_current, cur_cap, g_pending.current);
    if (out_max_len != nullptr) *out_max_len = g_pending.max_len;
    return true;
}

void render(State& s,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt) {
    const bool visible = s.open || s.closing || s.phase > 0.0025f;
    if (!visible) return;

    const float target = (s.open && !s.closing) ? 1.0f : 0.0f;
    s.phase = fx::anim::spring_to(s.phase, target, dt, 22.0f);
    if (s.closing && s.phase <= 0.01f) {
        s.open = false;
        s.closing = false;
        s.phase = 0.0f;
        return;
    }

    const float t = fx::anim::clamp01(s.phase);
    const float te = (s.open && !s.closing)
                       ? fx::anim::ease_out_back(t)
                       : fx::anim::ease_in_quad(t);
    const float alpha = fx::anim::ease_out_cubic(t);

    ImGui::SetNextWindowPos(viewport_min, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport_max.x - viewport_min.x,
                                    viewport_max.y - viewport_min.y),
                             ImGuiCond_Always);
    if (s.phase < 0.5f) ImGui::SetNextWindowFocus();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##numpad_overlay", nullptr, flags)) {
        const bool interactive = (s.open && !s.closing) && t >= 0.92f;
        if (!interactive) ImGui::BeginDisabled();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 dim = IM_COL32(0, 0, 0, static_cast<int>(0xC8 * alpha));
        dl->AddRectFilled(viewport_min, viewport_max, dim);

        const float vw = viewport_max.x - viewport_min.x;
        const float vh = viewport_max.y - viewport_min.y;

        const float panel_w = std::min(scale::dp(264.0f), vw - scale::dp(32.0f));
        const float gap = scale::dp(6.0f);
        const float key_size = (panel_w - gap * 4.0f) / 3.0f;
        const float pad = scale::dp(12.0f);
        const float display_h = scale::dp(40.0f);
        const float footer_h = scale::dp(36.0f);
        const float panel_h = pad * 2.0f + display_h
                              + scale::dp(10.0f) + key_size * 4.0f + gap * 3.0f
                              + scale::dp(10.0f) + footer_h;

        const float y_offset = (1.0f - te) * scale::dp(40.0f);
        const float px = viewport_min.x + (vw - panel_w) * 0.5f;
        const float py = viewport_min.y + (vh - panel_h) * 0.5f + y_offset;
        const ImVec2 panel_min(px, py);
        const ImVec2 panel_max(px + panel_w, py + panel_h);

        dl->AddRectFilled(ImVec2(panel_min.x + scale::dp(3.0f), panel_min.y + scale::dp(3.0f)),
                          ImVec2(panel_max.x + scale::dp(3.0f), panel_max.y + scale::dp(3.0f)),
                          tokens::kBlack);
        dl->AddRectFilled(panel_min, panel_max, tokens::kCharcoal);
        dl->AddRect(panel_min, panel_max, tokens::kToxic, 0.0f, 0,
                    scale::dp(tokens::kBorderThin));

        ImFont* mono_f  = fonts::get(fonts::Face::HeroMd);
        ImFont* btn_f   = fonts::get(fonts::Face::BrutalistMd);

        const ImVec2 disp_pos(panel_min.x + pad, panel_min.y + pad);
        const ImVec2 disp_sz(panel_w - pad * 2.0f, display_h);
        dl->AddRectFilled(disp_pos,
                          ImVec2(disp_pos.x + disp_sz.x, disp_pos.y + disp_sz.y),
                          tokens::kBlack);
        dl->AddRect(disp_pos,
                    ImVec2(disp_pos.x + disp_sz.x, disp_pos.y + disp_sz.y),
                    tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));

        const bool has_val = s.buffer[0] != '\0';
        const char* shown = has_val ? s.buffer : "----";
        ImU32 shown_col = has_val ? tokens::kToxic : tokens::kOffWhiteDim;
        if (mono_f != nullptr) {
            const float fs = mono_f->LegacySize * 0.82f;
            ImVec2 ts = mono_f->CalcTextSizeA(fs, FLT_MAX, 0.0f, shown);
            dl->AddText(mono_f, fs,
                        ImVec2(disp_pos.x + (disp_sz.x - ts.x) * 0.5f,
                               disp_pos.y + (disp_sz.y - ts.y) * 0.5f),
                        shown_col, shown);
        }

        const float keys_top = disp_pos.y + disp_sz.y + scale::dp(10.0f);
        const float keys_left = panel_min.x + (panel_w - (key_size * 3.0f + gap * 2.0f)) * 0.5f;

        struct Cell { const char* label; int row; int col; int kind; };
        const Cell cells[] = {
            {"1", 0, 0, 0}, {"2", 0, 1, 0}, {"3", 0, 2, 0},
            {"4", 1, 0, 0}, {"5", 1, 1, 0}, {"6", 1, 2, 0},
            {"7", 2, 0, 0}, {"8", 2, 1, 0}, {"9", 2, 2, 0},
            {"CLR", 3, 0, 1}, {"0", 3, 1, 0}, {"DEL", 3, 2, 2},
        };

        for (const Cell& cl : cells) {
            const ImVec2 p(keys_left + cl.col * (key_size + gap),
                           keys_top + cl.row * (key_size + gap));
            const ImVec2 sz(key_size, key_size);
            ImU32 bg = tokens::kCardBg;
            ImU32 fg = tokens::kOffWhite;
            if (cl.kind == 1) { bg = tokens::kHazardOrange; fg = tokens::kBlack; }
            else if (cl.kind == 2) { bg = tokens::kErrorRed; fg = tokens::kBlack; }
            char id[32]; std::snprintf(id, sizeof(id), "##np_%s", cl.label);
            const char* cl_lbl = (cl.kind == 1 || cl.kind == 2)
                                  ? ui::i18n::t(cl.label)
                                  : cl.label;
            if (key_button(id, p, sz, cl_lbl, btn_f, bg, fg)) {
                if (cl.kind == 0) {
                    int len = static_cast<int>(std::strlen(s.buffer));
                    if (len < s.max_len &&
                        len + 1 < static_cast<int>(sizeof(s.buffer))) {
                        if (!(len == 0 && cl.label[0] == '0' && s.max_len > 1)) {
                            s.buffer[len] = cl.label[0];
                            s.buffer[len + 1] = '\0';
                        }
                    }
                } else if (cl.kind == 1) {
                    s.buffer[0] = '\0';
                } else if (cl.kind == 2) {
                    int len = static_cast<int>(std::strlen(s.buffer));
                    if (len > 0) s.buffer[len - 1] = '\0';
                }
            }
        }

        const float footer_top = keys_top + key_size * 4.0f + gap * 3.0f + scale::dp(10.0f);
        const float footer_w = (panel_w - pad * 2.0f - gap) * 0.5f;

        const ImVec2 cancel_pos(panel_min.x + pad, footer_top);
        const ImVec2 ok_pos(cancel_pos.x + footer_w + gap, footer_top);
        const ImVec2 fsz(footer_w, footer_h);

        if (key_button("##np_cancel", cancel_pos, fsz, ui::i18n::t("CANCEL"), btn_f,
                       tokens::kCardBg, tokens::kOffWhite)) {
            close(s);
        }
        if (key_button("##np_ok", ok_pos, fsz, ui::i18n::t("OK"), btn_f,
                       tokens::kToxic, tokens::kBlack)) {
            ui::state::set_string(s.state_key, s.buffer);
            close(s);
        }
        if (!interactive) ImGui::EndDisabled();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

}
