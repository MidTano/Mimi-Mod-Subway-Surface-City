#include "color_picker.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../fx/animation.hpp"
#include "../state/mod_state.hpp"
#include "../i18n/lang.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ui::widgets::color_picker {

namespace {

void unpack(ImU32 c, int& r, int& g, int& b) {
    r = (c >> IM_COL32_R_SHIFT) & 0xFF;
    g = (c >> IM_COL32_G_SHIFT) & 0xFF;
    b = (c >> IM_COL32_B_SHIFT) & 0xFF;
}

ImU32 pack(int r, int g, int b) {
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return IM_COL32(r, g, b, 0xFF);
}

void rgb_to_hsv(float r, float g, float b, float& h, float& sv, float& v) {
    const float mx = std::fmax(r, std::fmax(g, b));
    const float mn = std::fmin(r, std::fmin(g, b));
    const float d = mx - mn;
    v = mx;
    sv = mx <= 0.0f ? 0.0f : (d / mx);
    if (d <= 0.0f) { h = 0.0f; return; }
    if (mx == r)       h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (mx == g)  h = (b - r) / d + 2.0f;
    else               h = (r - g) / d + 4.0f;
    h /= 6.0f;
}

void hsv_to_rgb(float h, float sv, float v, float& r, float& g, float& b) {
    if (sv <= 0.0f) { r = g = b = v; return; }
    h = std::fmod(h < 0.0f ? h + 1.0f : h, 1.0f);
    const float hh = h * 6.0f;
    const int   i  = static_cast<int>(std::floor(hh));
    const float f  = hh - i;
    const float p  = v * (1.0f - sv);
    const float q  = v * (1.0f - sv * f);
    const float t  = v * (1.0f - sv * (1.0f - f));
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

struct SliderResult {
    bool changed = false;
    float value = 0.0f;
};

SliderResult brutalist_slider(ImDrawList* dl, const char* id, const ImVec2& pos, float width,
                              float value, ImU32 track_bg, ImU32 fill_col, const char* label,
                              const char* value_str)
{
    ImFont* lbl_f = fonts::get(fonts::Face::MonoXs);
    const float lbl_fs = lbl_f ? lbl_f->LegacySize : scale::dp(10.0f);
    const float h_label = lbl_fs + scale::dp(3.0f);
    const float track_h = scale::dp(12.0f);

    if (lbl_f && label) dl->AddText(lbl_f, lbl_fs, ImVec2(pos.x, pos.y), tokens::kOffWhite, label);
    if (lbl_f && value_str) {
        ImVec2 vs = lbl_f->CalcTextSizeA(lbl_fs, FLT_MAX, 0.0f, value_str);
        dl->AddText(lbl_f, lbl_fs, ImVec2(pos.x + width - vs.x, pos.y), tokens::kToxic, value_str);
    }

    const ImVec2 tmin(pos.x, pos.y + h_label);
    const ImVec2 tmax(pos.x + width, tmin.y + track_h);
    ImGui::SetCursorScreenPos(tmin);
    ImGui::InvisibleButton(id, ImVec2(width, track_h + scale::dp(8.0f)));
    const bool active = ImGui::IsItemActive();

    SliderResult r;
    r.value = value;
    if (active) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        float p = (m.x - tmin.x) / (tmax.x - tmin.x);
        if (p < 0.0f) p = 0.0f; if (p > 1.0f) p = 1.0f;
        r.value = p;
        r.changed = true;
    }

    dl->AddRectFilled(tmin, tmax, tokens::kBlack);
    dl->AddRect(tmin, tmax, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));

    const float fill_w = (tmax.x - tmin.x) * r.value;
    if (track_bg & IM_COL32_A_MASK) {
        dl->AddRectFilled(tmin, tmax, track_bg);
    }
    dl->AddRectFilled(tmin, ImVec2(tmin.x + fill_w, tmax.y), fill_col);

    const float kx = tmin.x + fill_w;
    const float ky = (tmin.y + tmax.y) * 0.5f;
    const float ksz = scale::dp(16.0f);
    dl->AddRectFilled(ImVec2(kx - ksz * 0.5f, ky - ksz * 0.5f),
                      ImVec2(kx + ksz * 0.5f, ky + ksz * 0.5f),
                      tokens::kBlack);
    dl->AddRect(ImVec2(kx - ksz * 0.5f, ky - ksz * 0.5f),
                ImVec2(kx + ksz * 0.5f, ky + ksz * 0.5f),
                tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));
    return r;
}

void draw_sv_box(ImDrawList* dl, const ImVec2& pmin, const ImVec2& pmax, float hue) {
    float hr, hg, hb;
    hsv_to_rgb(hue, 1.0f, 1.0f, hr, hg, hb);
    const ImU32 hue_col = IM_COL32(static_cast<int>(hr * 255), static_cast<int>(hg * 255),
                                   static_cast<int>(hb * 255), 0xFF);
    dl->AddRectFilledMultiColor(pmin, pmax,
                                IM_COL32(0xFF, 0xFF, 0xFF, 0xFF), hue_col,
                                hue_col, IM_COL32(0xFF, 0xFF, 0xFF, 0xFF));
    dl->AddRectFilledMultiColor(pmin, pmax,
                                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
                                IM_COL32(0, 0, 0, 0xFF), IM_COL32(0, 0, 0, 0xFF));
    dl->AddRect(pmin, pmax, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));
}

void draw_hue_bar(ImDrawList* dl, const ImVec2& pmin, const ImVec2& pmax) {
    const int segments = 12;
    const float dy = (pmax.y - pmin.y) / segments;
    for (int i = 0; i < segments; ++i) {
        float r1, g1, b1, r2, g2, b2;
        hsv_to_rgb(static_cast<float>(i) / segments, 1.0f, 1.0f, r1, g1, b1);
        hsv_to_rgb(static_cast<float>(i + 1) / segments, 1.0f, 1.0f, r2, g2, b2);
        const ImU32 c1 = IM_COL32(static_cast<int>(r1 * 255), static_cast<int>(g1 * 255), static_cast<int>(b1 * 255), 0xFF);
        const ImU32 c2 = IM_COL32(static_cast<int>(r2 * 255), static_cast<int>(g2 * 255), static_cast<int>(b2 * 255), 0xFF);
        dl->AddRectFilledMultiColor(ImVec2(pmin.x, pmin.y + dy * i),
                                    ImVec2(pmax.x, pmin.y + dy * (i + 1)),
                                    c1, c1, c2, c2);
    }
    dl->AddRect(pmin, pmax, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));
}

}

void open(State& s, const char* state_key, const char* title, ImU32 initial) {
    s.open = true;
    std::snprintf(s.state_key, sizeof(s.state_key), "%s", state_key ? state_key : "");
    std::snprintf(s.title, sizeof(s.title), "%s", title ? title : "COLOR");
    s.initial = initial;
    s.current = initial;
}

void close(State& s) { s.open = false; }
bool is_visible(const State& s) { return s.open || s.phase > 0.0025f; }

namespace {
bool g_req_pending = false;
char g_req_key[96] = {0};
char g_req_title[48] = {0};
ImU32 g_req_initial = 0;
}

void request_open(const char* state_key, const char* title, ImU32 initial) {
    g_req_pending = true;
    std::snprintf(g_req_key, sizeof(g_req_key), "%s", state_key ? state_key : "");
    std::snprintf(g_req_title, sizeof(g_req_title), "%s", title ? title : "COLOR");
    g_req_initial = initial;
}

bool consume_request(char* out_key, size_t key_cap, char* out_title, size_t title_cap, ImU32* out_initial) {
    if (!g_req_pending) return false;
    if (out_key && key_cap) std::snprintf(out_key, key_cap, "%s", g_req_key);
    if (out_title && title_cap) std::snprintf(out_title, title_cap, "%s", g_req_title);
    if (out_initial) *out_initial = g_req_initial;
    g_req_pending = false;
    return true;
}

void render(State& s, const ImVec2& viewport_min, const ImVec2& viewport_max, float dt) {
    const float target = s.open ? 1.0f : 0.0f;
    s.phase = fx::anim::spring_to(s.phase, target, dt, 22.0f);
    if (!s.open && s.phase <= 0.0025f) return;

    const float t = fx::anim::clamp01(s.phase);
    const float te = s.open ? fx::anim::ease_out_back(t) : fx::anim::ease_in_quad(t);
    const float alpha = fx::anim::ease_out_cubic(t);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(viewport_min, viewport_max,
                      IM_COL32(0, 0, 0, static_cast<int>(0xC8 * alpha)));

    const float panel_w = std::min(scale::dp(360.0f), (viewport_max.x - viewport_min.x) - scale::dp(24.0f));
    const float panel_h = scale::dp(500.0f);
    const float y_offset = (1.0f - te) * scale::dp(36.0f);
    const float x_left = (viewport_min.x + viewport_max.x) * 0.5f - panel_w * 0.5f;
    const float y_top  = (viewport_min.y + viewport_max.y) * 0.5f - panel_h * 0.5f + y_offset;

    const ImVec2 pmin(x_left, y_top);
    const ImVec2 pmax(x_left + panel_w, y_top + panel_h);

    const float shadow = scale::dp(4.0f) * alpha;
    dl->AddRectFilled(ImVec2(pmin.x + shadow, pmin.y + shadow),
                      ImVec2(pmax.x + shadow, pmax.y + shadow), tokens::kBlack);
    dl->AddRectFilled(pmin, pmax, tokens::kCharcoal);
    dl->AddRect(pmin, pmax, tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThick));
    dl->AddRectFilled(ImVec2(pmin.x, pmin.y), ImVec2(pmax.x, pmin.y + scale::dp(4.0f)),
                      tokens::kToxic);

    const bool interactive = s.open && t >= 0.92f;

    ImGui::SetNextWindowPos(viewport_min);
    ImGui::SetNextWindowSize(ImVec2(viewport_max.x - viewport_min.x, viewport_max.y - viewport_min.y));
    if (s.open && s.phase < 0.5f) ImGui::SetNextWindowFocus();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::Begin("##color_picker_panel", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoSavedSettings);

    if (!interactive) ImGui::BeginDisabled();

    bool backdrop_clicked = false;

    const float pad = scale::dp(14.0f);
    float cur_x = pmin.x + pad;
    float cur_y = pmin.y + pad + scale::dp(2.0f);

    ImFont* hf = fonts::get(fonts::Face::BrutalistMd);
    const float hfs = hf ? hf->LegacySize : scale::dp(15.0f);
    if (hf) dl->AddText(hf, hfs, ImVec2(cur_x, cur_y), tokens::kToxic, ui::i18n::t(s.title));
    cur_y += hfs + scale::dp(12.0f);

    int r, g, b; unpack(s.current, r, g, b);
    float fr = r / 255.0f, fg = g / 255.0f, fb = b / 255.0f;
    float h, sv, v; rgb_to_hsv(fr, fg, fb, h, sv, v);

    const float sv_w = panel_w - pad * 2.0f - scale::dp(32.0f) - scale::dp(12.0f);
    const float sv_h = scale::dp(140.0f);
    const ImVec2 sv_min(cur_x, cur_y);
    const ImVec2 sv_max(cur_x + sv_w, cur_y + sv_h);
    draw_sv_box(dl, sv_min, sv_max, h);

    ImGui::SetCursorScreenPos(sv_min);
    ImGui::InvisibleButton("##sv_area", ImVec2(sv_w, sv_h));
    if (ImGui::IsItemActive()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        float sx = (m.x - sv_min.x) / sv_w;
        float sy = (m.y - sv_min.y) / sv_h;
        if (sx < 0.0f) sx = 0.0f; if (sx > 1.0f) sx = 1.0f;
        if (sy < 0.0f) sy = 0.0f; if (sy > 1.0f) sy = 1.0f;
        sv = sx;
        v  = 1.0f - sy;
        hsv_to_rgb(h, sv, v, fr, fg, fb);
        s.current = pack(static_cast<int>(fr * 255), static_cast<int>(fg * 255), static_cast<int>(fb * 255));
    }

    const float crx = sv_min.x + sv * sv_w;
    const float cry = sv_min.y + (1.0f - v) * sv_h;
    const float r_inner = scale::dp(5.0f);
    const float r_outer = scale::dp(7.0f);
    dl->AddCircle(ImVec2(crx, cry), r_outer, tokens::kBlack, 20, scale::dp(2.0f));
    dl->AddCircle(ImVec2(crx, cry), r_inner, tokens::kOffWhite, 16, scale::dp(1.5f));

    const float hue_x = sv_max.x + scale::dp(12.0f);
    const float hue_w = scale::dp(24.0f);
    const ImVec2 hue_min(hue_x, cur_y);
    const ImVec2 hue_max(hue_x + hue_w, cur_y + sv_h);
    draw_hue_bar(dl, hue_min, hue_max);
    ImGui::SetCursorScreenPos(hue_min);
    ImGui::InvisibleButton("##hue_area", ImVec2(hue_w, sv_h));
    if (ImGui::IsItemActive()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        float hy = (m.y - hue_min.y) / sv_h;
        if (hy < 0.0f) hy = 0.0f; if (hy > 1.0f) hy = 1.0f;
        h = hy;
        hsv_to_rgb(h, sv, v, fr, fg, fb);
        s.current = pack(static_cast<int>(fr * 255), static_cast<int>(fg * 255), static_cast<int>(fb * 255));
    }
    const float hue_y_knob = hue_min.y + h * sv_h;
    dl->AddRect(ImVec2(hue_min.x - scale::dp(3.0f), hue_y_knob - scale::dp(3.0f)),
                ImVec2(hue_max.x + scale::dp(3.0f), hue_y_knob + scale::dp(3.0f)),
                tokens::kBlack, 0.0f, 0, scale::dp(2.0f));
    dl->AddRect(ImVec2(hue_min.x - scale::dp(2.0f), hue_y_knob - scale::dp(2.0f)),
                ImVec2(hue_max.x + scale::dp(2.0f), hue_y_knob + scale::dp(2.0f)),
                tokens::kToxic, 0.0f, 0, scale::dp(1.5f));

    cur_y = sv_max.y + scale::dp(14.0f);

    char rbuf[8], gbuf[8], bbuf[8];
    std::snprintf(rbuf, sizeof(rbuf), "%d", r);
    std::snprintf(gbuf, sizeof(gbuf), "%d", g);
    std::snprintf(bbuf, sizeof(bbuf), "%d", b);

    const float slider_w = panel_w - pad * 2.0f;

    auto do_slider = [&](const char* id, const char* label, float cur,
                         ImU32 fill_col, int channel_idx) {
        char vb[8]; std::snprintf(vb, sizeof(vb), "%d", static_cast<int>(cur * 255 + 0.5f));
        SliderResult sr = brutalist_slider(dl, id, ImVec2(cur_x, cur_y), slider_w,
                                           cur, 0, fill_col, label, vb);
        if (sr.changed) {
            int val = static_cast<int>(sr.value * 255 + 0.5f);
            if (channel_idx == 0) r = val;
            else if (channel_idx == 1) g = val;
            else if (channel_idx == 2) b = val;
            s.current = pack(r, g, b);
        }
        cur_y += scale::dp(12.0f) + scale::dp(8.0f) + scale::dp(10.0f);
    };

    do_slider("##slider_r", "R", fr, IM_COL32(0xFF, 0x33, 0x55, 0xFF), 0);
    do_slider("##slider_g", "G", fg, IM_COL32(0x44, 0xFF, 0x88, 0xFF), 1);
    do_slider("##slider_b", "B", fb, IM_COL32(0x33, 0xAA, 0xFF, 0xFF), 2);

    const float preview_h = scale::dp(40.0f);
    const ImVec2 prev_min(cur_x, cur_y);
    const ImVec2 prev_max(cur_x + slider_w, cur_y + preview_h);
    unpack(s.current, r, g, b);
    dl->AddRectFilled(prev_min, prev_max, s.current);
    dl->AddRect(prev_min, prev_max, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));
    ImFont* mf = fonts::get(fonts::Face::MonoSm);
    const float mfs = mf ? mf->LegacySize : scale::dp(12.0f);
    char hex[16]; std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
    if (mf) {
        ImVec2 hsz = mf->CalcTextSizeA(mfs, FLT_MAX, 0.0f, hex);
        const float bg_pad = scale::dp(6.0f);
        const ImVec2 bgmn(prev_max.x - hsz.x - bg_pad * 2.0f - scale::dp(6.0f),
                           prev_min.y + (preview_h - hsz.y - bg_pad * 2.0f) * 0.5f);
        const ImVec2 bgmx(bgmn.x + hsz.x + bg_pad * 2.0f, bgmn.y + hsz.y + bg_pad * 2.0f);
        dl->AddRectFilled(bgmn, bgmx, tokens::kBlack);
        dl->AddRect(bgmn, bgmx, tokens::kToxic, 0.0f, 0, scale::dp(1.5f));
        dl->AddText(mf, mfs, ImVec2(bgmn.x + bg_pad, bgmn.y + bg_pad), tokens::kToxic, hex);
    }
    cur_y += preview_h + scale::dp(12.0f);

    static const ImU32 presets[] = {
        IM_COL32(0xCC, 0xFF, 0x00, 0xFF),
        IM_COL32(0xFF, 0x33, 0x55, 0xFF),
        IM_COL32(0x00, 0xE5, 0xFF, 0xFF),
        IM_COL32(0xF7, 0xA9, 0x00, 0xFF),
        IM_COL32(0xE5, 0xE5, 0xE5, 0xFF),
        IM_COL32(0xA1, 0x92, 0xFF, 0xFF),
    };
    const int n_presets = static_cast<int>(sizeof(presets) / sizeof(presets[0]));
    const float sw_sz = (slider_w - (n_presets - 1) * scale::dp(8.0f)) / n_presets;
    for (int i = 0; i < n_presets; ++i) {
        const float px = cur_x + i * (sw_sz + scale::dp(8.0f));
        const ImVec2 smin(px, cur_y);
        const ImVec2 smax(px + sw_sz, cur_y + sw_sz);
        char pid[32]; std::snprintf(pid, sizeof(pid), "##preset_%d", i);
        ImGui::SetCursorScreenPos(smin);
        ImGui::InvisibleButton(pid, ImVec2(sw_sz, sw_sz));
        if (ImGui::IsItemClicked()) {
            s.current = presets[i];
        }
        const bool sel = (presets[i] == s.current);
        const float so = scale::dp(3.0f);
        dl->AddRectFilled(ImVec2(smin.x + so, smin.y + so),
                          ImVec2(smax.x + so, smax.y + so), tokens::kBlack);
        dl->AddRectFilled(smin, smax, presets[i]);
        dl->AddRect(smin, smax, sel ? tokens::kToxic : tokens::kBlack, 0.0f, 0,
                    scale::dp(sel ? 3.0f : 2.0f));
    }
    cur_y += sw_sz + scale::dp(16.0f);

    const float btn_gap = scale::dp(14.0f);
    const float btn_h = scale::dp(44.0f);
    const float shadow_dp = scale::dp(3.0f);
    const float inset = scale::dp(2.0f);
    const float btn_w = (slider_w - btn_gap - shadow_dp) * 0.5f;

    auto draw_btn = [&](const char* id_str, const char* label,
                        float bx, float by, bool primary) -> bool {
        const ImVec2 mn(bx, by);
        const ImVec2 mx(bx + btn_w, by + btn_h);
        ImGui::SetCursorScreenPos(mn);
        ImGui::InvisibleButton(id_str, ImVec2(btn_w, btn_h));
        const bool a = ImGui::IsItemActive();
        const bool c = ImGui::IsItemClicked();
        const float so = a ? inset : shadow_dp;
        dl->AddRectFilled(ImVec2(mn.x + so, mn.y + so),
                          ImVec2(mx.x + so, mx.y + so), tokens::kBlack);
        const ImVec2 bmn = a ? ImVec2(mn.x + inset, mn.y + inset) : mn;
        const ImVec2 bmx = a ? ImVec2(mx.x + inset, mx.y + inset) : mx;
        const ImU32 bg = primary ? tokens::kToxic : IM_COL32(0x20, 0x20, 0x20, 0xFF);
        const ImU32 border = primary ? tokens::kBlack : tokens::kToxic;
        const ImU32 text = primary ? tokens::kBlack : tokens::kToxic;
        dl->AddRectFilled(bmn, bmx, bg);
        dl->AddRect(bmn, bmx, border, 0.0f, 0,
                    scale::dp(primary ? tokens::kBorderThick : tokens::kBorderThin));
        if (hf) {
            ImVec2 lsz = hf->CalcTextSizeA(hfs, FLT_MAX, 0.0f, label);
            dl->AddText(hf, hfs,
                        ImVec2(bmn.x + (btn_w - lsz.x) * 0.5f, bmn.y + (btn_h - lsz.y) * 0.5f),
                        text, label);
        }
        return c;
    };

    const bool cancel_clicked = draw_btn("##cp_cancel", ui::i18n::t("CANCEL"), cur_x, cur_y, false);
    const bool apply_clicked  = draw_btn("##cp_apply", ui::i18n::t("APPLY"),
                                          cur_x + btn_w + btn_gap, cur_y, true);

    if (interactive && ImGui::IsMouseClicked(0) &&
        !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive()) {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const bool inside_panel =
            mp.x >= pmin.x && mp.x <= pmax.x && mp.y >= pmin.y && mp.y <= pmax.y;
        if (!inside_panel) backdrop_clicked = true;
    }

    if (!interactive) ImGui::EndDisabled();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    if (apply_clicked) {
        if (s.state_key[0]) state::set_int(s.state_key, static_cast<int>(s.current));
        s.open = false;
    }
    if (cancel_clicked || backdrop_clicked) {
        if (s.state_key[0]) state::set_int(s.state_key, static_cast<int>(s.initial));
        s.current = s.initial;
        s.open = false;
    }
}

}
