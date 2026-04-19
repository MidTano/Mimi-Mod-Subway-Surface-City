#include "cards.hpp"
#include "primitives.hpp"
#include "color_picker.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../fx/animation.hpp"
#include "../fx/glitch.hpp"
#include "../state/mod_state.hpp"
#include "../i18n/lang.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace ui::widgets::cards {

namespace {

float g_card_gap_dp = 12.0f;

inline bool card_culled(const ImVec2& pos, float h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 cmin = dl->GetClipRectMin();
    const ImVec2 cmax = dl->GetClipRectMax();
    const float margin = 32.0f;
    return (pos.y + h < cmin.y - margin) || (pos.y > cmax.y + margin);
}

void fmt_value(char* buf, size_t sz, float v, const char* fmt) {
    if (!fmt || !*fmt) { std::snprintf(buf, sz, "%.2f", v); return; }
    if (std::strcmp(fmt, "%") == 0) {
        std::snprintf(buf, sz, "%d%%", static_cast<int>(std::round(v * 100.0f)));
    } else if (std::strcmp(fmt, "x") == 0) {
        std::snprintf(buf, sz, "%.2fx", v);
    } else if (std::strcmp(fmt, "s") == 0) {
        std::snprintf(buf, sz, "%.1fs", v);
    } else if (std::strcmp(fmt, "m") == 0) {
        std::snprintf(buf, sz, "%.1fm", v);
    } else if (std::strcmp(fmt, "\xC2\xB0") == 0 || std::strcmp(fmt, "deg") == 0) {
        std::snprintf(buf, sz, "%.0f\xC2\xB0", v);
    } else {
        std::snprintf(buf, sz, "%.2f%s", v, fmt);
    }
}

void draw_card_bg(ImDrawList* dl, const ImVec2& pos, const ImVec2& size, bool pressed) {
    const ImVec2 shadow_off = pressed ? ImVec2(1.0f, 1.0f) : ImVec2(scale::dp(tokens::kShadowOffsetThin), scale::dp(tokens::kShadowOffsetThin));
    dl->AddRectFilled(ImVec2(pos.x + shadow_off.x, pos.y + shadow_off.y),
                      ImVec2(pos.x + size.x + shadow_off.x, pos.y + size.y + shadow_off.y),
                      tokens::kBlack);
    ImVec2 fp = pos;
    if (pressed) { fp.x += 2.0f; fp.y += 2.0f; }
    dl->AddRectFilled(fp, ImVec2(fp.x + size.x, fp.y + size.y), tokens::kCardBg);
    dl->AddRect(fp, ImVec2(fp.x + size.x, fp.y + size.y), tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));
}

void draw_icon_box(ImDrawList* dl, const ImVec2& pos, float size, const char* icon_utf8) {
    dl->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), tokens::kToxic);
    dl->AddRect(pos, ImVec2(pos.x + size, pos.y + size), tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));
    if (!icon_utf8 || !*icon_utf8) return;
    ImFont* icon_font = fonts::get(fonts::Face::IconLg);
    if (!icon_font) return;
    const float fs = icon_font->LegacySize;
    ImVec2 isz = icon_font->CalcTextSizeA(fs, FLT_MAX, 0.0f, icon_utf8);
    const ImVec2 ip(pos.x + (size - isz.x) * 0.5f, pos.y + (size - isz.y) * 0.5f);
    dl->AddText(icon_font, fs, ip, tokens::kBlack, icon_utf8);
}

ImU32 color_lerp(ImU32 a, ImU32 b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    ImU8 ar = (a >> IM_COL32_R_SHIFT) & 0xFF, ag = (a >> IM_COL32_G_SHIFT) & 0xFF, ab = (a >> IM_COL32_B_SHIFT) & 0xFF, aa = (a >> IM_COL32_A_SHIFT) & 0xFF;
    ImU8 br = (b >> IM_COL32_R_SHIFT) & 0xFF, bg = (b >> IM_COL32_G_SHIFT) & 0xFF, bb = (b >> IM_COL32_B_SHIFT) & 0xFF, ba = (b >> IM_COL32_A_SHIFT) & 0xFF;
    ImU8 r = static_cast<ImU8>(ar + (br - ar) * t);
    ImU8 g = static_cast<ImU8>(ag + (bg - ag) * t);
    ImU8 bl= static_cast<ImU8>(ab + (bb - ab) * t);
    ImU8 al= static_cast<ImU8>(aa + (ba - aa) * t);
    return IM_COL32(r, g, bl, al);
}

void draw_switch(ImDrawList* dl, const ImVec2& pos, float phase) {
    const float w = scale::dp(52.0f);
    const float h = scale::dp(28.0f);
    const float t = fx::anim::clamp01(phase);
    const float te = fx::anim::ease_out_cubic(t);
    const ImU32 off_bg = IM_COL32(0x1A, 0x1A, 0x1A, 0xFF);
    const ImU32 bg = color_lerp(off_bg, tokens::kToxic, t);
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), bg);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));

    const float knob_sz = scale::dp(22.0f);
    const float pad = (h - knob_sz) * 0.5f;
    const float kx_off = pos.x + pad;
    const float kx_on  = pos.x + w - knob_sz - pad;
    const float kx = kx_off + (kx_on - kx_off) * te;
    const float ky = pos.y + (h - knob_sz) * 0.5f;
    const ImU32 kbg  = color_lerp(tokens::kOffWhite, tokens::kBlack, t);
    const ImU32 kbor = color_lerp(tokens::kBlack, tokens::kToxic, t);
    dl->AddRectFilled(ImVec2(kx, ky), ImVec2(kx + knob_sz, ky + knob_sz), kbg);
    dl->AddRect(ImVec2(kx, ky), ImVec2(kx + knob_sz, ky + knob_sz), kbor, 0.0f, 0, scale::dp(tokens::kBorderThin));
}

float& get_anim_state(const char* key, float initial) {
    ImGuiID id = ImGui::GetID(key);
    float* ref = ImGui::GetStateStorage()->GetFloatRef(id, initial);
    return *ref;
}

void truncate_to_width(char* dst, size_t dst_sz, const char* src, ImFont* font, float fs, float max_w) {
    if (!src) { if (dst_sz) dst[0] = '\0'; return; }
    if (!font) { std::snprintf(dst, dst_sz, "%s", src); return; }
    ImVec2 full = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, src);
    if (full.x <= max_w) { std::snprintf(dst, dst_sz, "%s", src); return; }
    const char* ellipsis = "...";
    ImVec2 esz = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, ellipsis);
    const size_t src_len = std::strlen(src);
    size_t lo = 0, hi = src_len;
    while (lo < hi) {
        size_t mid = (lo + hi + 1) / 2;
        char tmp[256];
        if (mid >= sizeof(tmp) - 4) { hi = mid - 1; continue; }
        std::memcpy(tmp, src, mid);
        tmp[mid] = '\0';
        ImVec2 sz = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, tmp);
        if (sz.x + esz.x <= max_w) lo = mid;
        else hi = mid - 1;
    }
    char out[256];
    if (lo >= sizeof(out) - 4) lo = sizeof(out) - 4;
    std::memcpy(out, src, lo);
    out[lo] = '\0';
    std::snprintf(dst, dst_sz, "%s%s", out, ellipsis);
}

void draw_chevron_right(ImDrawList* dl, const ImVec2& center, float size, ImU32 col, float thick) {
    const float r = size * 0.5f;
    dl->AddLine(ImVec2(center.x - r * 0.4f, center.y - r),
                ImVec2(center.x + r * 0.6f, center.y), col, thick);
    dl->AddLine(ImVec2(center.x + r * 0.6f, center.y),
                ImVec2(center.x - r * 0.4f, center.y + r), col, thick);
}

void draw_title_subtitle(ImDrawList* dl, float x, float y_top, float h_container, float max_w,
                        const char* title, const char* subtitle,
                        ImU32 title_col, ImU32 subtitle_col) {
    title    = ui::i18n::t(title);
    if (subtitle != nullptr) subtitle = ui::i18n::t(subtitle);
    ImFont* t_font = fonts::get(fonts::Face::BrutalistMd);
    ImFont* s_font = fonts::get(fonts::Face::MonoXs);
    const float ts = t_font ? t_font->LegacySize : scale::dp(15.0f);
    const float ss_raw = s_font ? s_font->LegacySize : scale::dp(11.0f);
    const float ss = ss_raw - scale::dp(2.0f);
    const bool has_sub = subtitle && *subtitle;
    const float gap = has_sub ? scale::dp(3.0f) : 0.0f;
    const float total_h = ts + (has_sub ? ss + gap : 0.0f);
    const float y = y_top + (h_container - total_h) * 0.5f;
    char title_buf[128], sub_buf[160];
    truncate_to_width(title_buf, sizeof(title_buf), title, t_font, ts, max_w);
    truncate_to_width(sub_buf, sizeof(sub_buf), subtitle ? subtitle : "", s_font, ss, max_w);
    if (t_font) dl->AddText(t_font, ts, ImVec2(x, y), title_col, title_buf);
    if (s_font && has_sub) dl->AddText(s_font, ss, ImVec2(x, y + ts + gap), subtitle_col, sub_buf);
}

}

float card_gap() { return scale::dp(g_card_gap_dp); }

float card_toggle(const ImVec2& pos, float width,
                  const char* state_key,
                  const char* icon_utf8,
                  const char* title,
                  const char* subtitle)
{
    const float pad = scale::dp(tokens::kCardPadding);
    const float icon_sz = scale::dp(40.0f);
    const float row_gap = scale::dp(12.0f);
    const float sw_w = scale::dp(52.0f);
    const float sw_h = scale::dp(28.0f);
    const float h = pad * 2.0f + (icon_sz > sw_h ? icon_sz : sw_h);

    if (card_culled(pos, h)) return h;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(state_key, ImVec2(width, h));
    const bool pressed = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked();

    draw_card_bg(dl, pos, ImVec2(width, h), pressed);
    const ImVec2 body_pos = pressed ? ImVec2(pos.x + 2.0f, pos.y + 2.0f) : pos;

    const ImVec2 icon_pos(body_pos.x + pad, body_pos.y + (h - icon_sz) * 0.5f);
    draw_icon_box(dl, icon_pos, icon_sz, icon_utf8);

    const float tx = icon_pos.x + icon_sz + row_gap;
    const float tw = (body_pos.x + width - pad - sw_w - row_gap) - tx;
    draw_title_subtitle(dl, tx, body_pos.y + pad, h - pad * 2.0f, tw, title, subtitle,
                        tokens::kOffWhite, tokens::kOffWhiteDim);

    const ImVec2 sw_pos(body_pos.x + width - pad - sw_w, body_pos.y + (h - sw_h) * 0.5f);
    const bool on = state::get_bool(state_key, false);
    char anim_id[96]; std::snprintf(anim_id, sizeof(anim_id), "anim_sw_%s", state_key);
    float& phase = get_anim_state(anim_id, on ? 1.0f : 0.0f);
    const float target = on ? 1.0f : 0.0f;
    const float dt = ImGui::GetIO().DeltaTime;
    phase = fx::anim::spring_to(phase, target, dt, 12.0f);
    draw_switch(dl, sw_pos, phase);

    if (clicked) {
        const bool new_v = !on;
        state::set_bool(state_key, new_v);
        const char* tt = ui::i18n::t(title ? title : state_key);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "%s %s", tt, ui::i18n::t(new_v ? "ON" : "OFF"));
        fx::glitch::notify(icon_utf8, buf);
    }
    return h;
}

float card_slider(const ImVec2& pos, float width,
                  const char* state_key,
                  const char* icon_utf8,
                  const char* title,
                  float min_v, float max_v, float step, float def_v,
                  const char* fmt)
{
    if (!state::has(state_key)) state::set_float(state_key, def_v);
    float v = state::get_float(state_key, def_v);

    const float pad = scale::dp(tokens::kCardPadding);
    ImFont* t_font = fonts::get(fonts::Face::BrutalistMd);
    ImFont* mono_sm = fonts::get(fonts::Face::MonoSm);
    ImFont* mono_lbl = fonts::get(fonts::Face::MonoSm);
    ImFont* icon_font = fonts::get(fonts::Face::IconMd);
    const float ts = t_font ? t_font->LegacySize : scale::dp(15.0f);
    const float ms = mono_sm ? mono_sm->LegacySize : scale::dp(12.0f);
    const float lbl_s = mono_lbl ? mono_lbl->LegacySize : scale::dp(12.0f);
    const float header_h = (ts > ms ? ts : ms) + scale::dp(4.0f);
    const float track_h = scale::dp(10.0f);
    const float label_h = lbl_s + scale::dp(2.0f);
    const float h = pad * 2.0f + header_h + scale::dp(10.0f) + track_h + scale::dp(6.0f) + label_h;

    if (card_culled(pos, h)) return h;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char btn_id[64]; std::snprintf(btn_id, sizeof(btn_id), "##slider_%s", state_key);

    const ImVec2 track_min(pos.x + pad, pos.y + pad + header_h + scale::dp(8.0f));
    const ImVec2 track_max(pos.x + width - pad, track_min.y + track_h);

    ImGui::SetCursorScreenPos(ImVec2(track_min.x, track_min.y - scale::dp(10.0f)));
    ImGui::InvisibleButton(btn_id, ImVec2(track_max.x - track_min.x, track_h + scale::dp(20.0f)));
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    if (active) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        float p = (m.x - track_min.x) / (track_max.x - track_min.x);
        if (p < 0.0f) p = 0.0f; if (p > 1.0f) p = 1.0f;
        float new_v = min_v + p * (max_v - min_v);
        if (step > 0.0f) new_v = std::round(new_v / step) * step;
        if (new_v < min_v) new_v = min_v; if (new_v > max_v) new_v = max_v;
        state::set_float(state_key, new_v);
        v = new_v;
    }

    draw_card_bg(dl, pos, ImVec2(width, h), false);

    if (icon_font && icon_utf8) {
        const float isz = icon_font->LegacySize;
        ImVec2 msz = icon_font->CalcTextSizeA(isz, FLT_MAX, 0.0f, icon_utf8);
        dl->AddText(icon_font, isz,
                    ImVec2(pos.x + pad, pos.y + pad + (header_h - msz.y) * 0.5f),
                    tokens::kToxic, icon_utf8);
    }
    if (t_font) {
        const float icon_w = icon_font ? icon_font->LegacySize + scale::dp(6.0f) : 0.0f;
        dl->AddText(t_font, ts, ImVec2(pos.x + pad + icon_w, pos.y + pad), tokens::kOffWhite, ui::i18n::t(title));
    }

    char val_buf[32];
    fmt_value(val_buf, sizeof(val_buf), v, fmt);
    if (mono_sm) {
        ImVec2 vsz = mono_sm->CalcTextSizeA(ms, FLT_MAX, 0.0f, val_buf);
        const float vx = pos.x + width - pad - vsz.x - scale::dp(8.0f);
        const float vy = pos.y + pad + (header_h - vsz.y) * 0.5f;
        const ImVec2 bg_min(vx - scale::dp(8.0f), vy - scale::dp(3.0f));
        const ImVec2 bg_max(vx + vsz.x + scale::dp(8.0f), vy + vsz.y + scale::dp(3.0f));
        dl->AddRectFilled(bg_min, bg_max, tokens::kBlack);
        dl->AddRect(bg_min, bg_max, tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));
        dl->AddText(mono_sm, ms, ImVec2(vx, vy), tokens::kToxic, val_buf);
    }

    dl->AddRectFilled(track_min, track_max, tokens::kBlack);
    dl->AddRect(track_min, track_max, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));

    const float denom = (max_v - min_v);
    const float pct = denom != 0.0f ? ((v - min_v) / denom) : 0.0f;
    const float fill_w = (track_max.x - track_min.x) * pct;
    dl->AddRectFilled(track_min, ImVec2(track_min.x + fill_w, track_max.y), tokens::kToxic);

    char knob_anim_id[96]; std::snprintf(knob_anim_id, sizeof(knob_anim_id), "anim_knob_%s", state_key);
    float& knob_phase = get_anim_state(knob_anim_id, 0.0f);
    const float knob_target = (active || hovered) ? 1.0f : 0.0f;
    knob_phase = fx::anim::spring_to(knob_phase, knob_target, ImGui::GetIO().DeltaTime, 16.0f);

    const float knob_base = scale::dp(18.0f);
    const float knob_sz = knob_base + scale::dp(4.0f) * knob_phase;
    const float kx = track_min.x + fill_w;
    const float ky = (track_min.y + track_max.y) * 0.5f;
    if (knob_phase > 0.01f) {
        const float halo = scale::dp(12.0f) * knob_phase;
        dl->AddRectFilled(ImVec2(kx - knob_sz * 0.5f - halo, ky - knob_sz * 0.5f - halo),
                          ImVec2(kx + knob_sz * 0.5f + halo, ky + knob_sz * 0.5f + halo),
                          IM_COL32(0xCC, 0xFF, 0x00, static_cast<int>(0x33 * knob_phase)));
    }
    dl->AddRectFilled(ImVec2(kx - knob_sz * 0.5f, ky - knob_sz * 0.5f),
                      ImVec2(kx + knob_sz * 0.5f, ky + knob_sz * 0.5f),
                      tokens::kBlack);
    dl->AddRect(ImVec2(kx - knob_sz * 0.5f, ky - knob_sz * 0.5f),
                ImVec2(kx + knob_sz * 0.5f, ky + knob_sz * 0.5f),
                tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));

    if (mono_lbl) {
        char min_buf[32], max_buf[32];
        fmt_value(min_buf, sizeof(min_buf), min_v, fmt);
        fmt_value(max_buf, sizeof(max_buf), max_v, fmt);
        const float y = track_max.y + scale::dp(6.0f);
        dl->AddText(mono_lbl, lbl_s, ImVec2(track_min.x, y), tokens::kOffWhiteDim, min_buf);
        ImVec2 msz = mono_lbl->CalcTextSizeA(lbl_s, FLT_MAX, 0.0f, max_buf);
        dl->AddText(mono_lbl, lbl_s, ImVec2(track_max.x - msz.x, y), tokens::kOffWhiteDim, max_buf);
    }

    return h;
}

float card_segmented(const ImVec2& pos, float width,
                     const char* state_key,
                     const char* title,
                     const char* const* options, int option_count,
                     bool silent)
{
    const float pad = scale::dp(tokens::kCardPadding);
    ImFont* t_font = fonts::get(fonts::Face::BrutalistMd);
    ImFont* pill_font = fonts::get(fonts::Face::BrutalistXs);
    const float ts = t_font ? t_font->LegacySize : scale::dp(14.0f);
    const float ps = pill_font ? pill_font->LegacySize : scale::dp(10.0f);

    const float pill_pad_x = scale::dp(10.0f);
    const float pill_pad_y = scale::dp(4.0f);
    const float pill_h = ps + pill_pad_y * 2.0f;
    const float pill_gap = scale::dp(8.0f);

    if (!state::has(state_key)) state::set_int(state_key, 0);
    int sel = state::get_int(state_key, 0);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    float cur_x = pos.x + pad;
    float cur_y = pos.y + pad + ts + scale::dp(8.0f);
    const float end_x = pos.x + width - pad;
    int rows = 1;
    for (int i = 0; i < option_count; ++i) {
        const char* opt_tr = ui::i18n::t(options[i]);
        ImVec2 sz = pill_font ? pill_font->CalcTextSizeA(ps, FLT_MAX, 0.0f, opt_tr)
                              : ImGui::CalcTextSize(opt_tr);
        const float pw = sz.x + pill_pad_x * 2.0f;
        if (cur_x + pw > end_x && i > 0) {
            cur_x = pos.x + pad;
            cur_y += pill_h + pill_gap;
            rows++;
        }
        cur_x += pw + pill_gap;
    }
    const float h = pad * 2.0f + ts + scale::dp(8.0f) + rows * pill_h + (rows - 1) * pill_gap;

    if (card_culled(pos, h)) return h;

    draw_card_bg(dl, pos, ImVec2(width, h), false);

    if (t_font) {
        dl->AddText(t_font, ts, ImVec2(pos.x + pad, pos.y + pad), tokens::kOffWhite, ui::i18n::t(title));
    }

    cur_x = pos.x + pad;
    cur_y = pos.y + pad + ts + scale::dp(8.0f);
    for (int i = 0; i < option_count; ++i) {
        const char* opt_tr = ui::i18n::t(options[i]);
        ImVec2 sz = pill_font ? pill_font->CalcTextSizeA(ps, FLT_MAX, 0.0f, opt_tr)
                              : ImGui::CalcTextSize(opt_tr);
        const float pw = sz.x + pill_pad_x * 2.0f;
        if (cur_x + pw > end_x && i > 0) {
            cur_x = pos.x + pad;
            cur_y += pill_h + pill_gap;
        }
        char pid[64]; std::snprintf(pid, sizeof(pid), "##pill_%s_%d", state_key, i);
        ImGui::SetCursorScreenPos(ImVec2(cur_x, cur_y));
        ImGui::InvisibleButton(pid, ImVec2(pw, pill_h));
        const bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            sel = i;
            state::set_int(state_key, sel);
            if (!silent) {
                char buf[160];
                const char* tt = ui::i18n::t(title ? title : state_key);
                std::snprintf(buf, sizeof(buf), "%s %s", tt, ui::i18n::t(options[i]));
                fx::glitch::notify(icons::k_tune, buf);
            }
        }
        const bool active = (i == sel);
        char anim_id[96]; std::snprintf(anim_id, sizeof(anim_id), "anim_pill_%s_%d", state_key, i);
        float& phase = get_anim_state(anim_id, active ? 1.0f : 0.0f);
        phase = fx::anim::spring_to(phase, active ? 1.0f : 0.0f, ImGui::GetIO().DeltaTime, 14.0f);

        const ImVec2 p_min(cur_x, cur_y);
        const ImVec2 p_max(cur_x + pw, cur_y + pill_h);
        const float shadow_t = fx::anim::clamp01(phase * 1.2f);
        if (shadow_t > 0.01f) {
            const float so = scale::dp(2.0f) * shadow_t;
            dl->AddRectFilled(ImVec2(p_min.x + so, p_min.y + so),
                              ImVec2(p_max.x + so, p_max.y + so),
                              IM_COL32(0, 0, 0, static_cast<int>(0xFF * shadow_t)));
        }
        const ImU32 fill_off = hovered ? IM_COL32(0xCC, 0xFF, 0x00, 0x26) : IM_COL32(0, 0, 0, 0);
        const ImU32 fill = color_lerp(fill_off, tokens::kToxic, phase);
        dl->AddRectFilled(p_min, p_max, fill);
        dl->AddRect(p_min, p_max, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));
        if (pill_font) {
            const ImU32 txt_col = color_lerp(tokens::kOffWhite, tokens::kBlack, phase);
            dl->AddText(pill_font, ps,
                        ImVec2(cur_x + pill_pad_x, cur_y + pill_pad_y),
                        txt_col, opt_tr);
        }
        cur_x += pw + pill_gap;
    }

    return h;
}

float card_color(const ImVec2& pos, float width,
                 const char* state_key,
                 const char* title,
                 ImU32 default_color)
{
    if (!state::has(state_key)) state::set_int(state_key, static_cast<int>(default_color));
    ImU32 color = static_cast<ImU32>(state::get_int(state_key, static_cast<int>(default_color)));

    const float pad = scale::dp(tokens::kCardPadding);
    const float sw_sz = scale::dp(40.0f);
    const float gap = scale::dp(12.0f);
    const float h = pad * 2.0f + sw_sz;

    if (card_culled(pos, h)) return h;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    char btn_id[64]; std::snprintf(btn_id, sizeof(btn_id), "##color_%s", state_key);
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(btn_id, ImVec2(width, h));
    const bool pressed = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked();
    if (clicked) {
        color_picker::request_open(state_key, title, color);
    }

    draw_card_bg(dl, pos, ImVec2(width, h), pressed);
    const ImVec2 body = pressed ? ImVec2(pos.x + 2, pos.y + 2) : pos;

    const ImVec2 sw_pos(body.x + pad, body.y + pad);
    dl->AddRectFilled(sw_pos, ImVec2(sw_pos.x + sw_sz, sw_pos.y + sw_sz), color);
    dl->AddRect(sw_pos, ImVec2(sw_pos.x + sw_sz, sw_pos.y + sw_sz), tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));

    ImFont* t_font = fonts::get(fonts::Face::BrutalistMd);
    ImFont* hex_font = fonts::get(fonts::Face::MonoSm);
    const float ts = t_font ? t_font->LegacySize : scale::dp(14.0f);
    const float hs = hex_font ? hex_font->LegacySize : scale::dp(10.0f);
    const float total_h = ts + hs + scale::dp(2.0f);
    const float y = body.y + (h - total_h) * 0.5f;
    const float tx = sw_pos.x + sw_sz + gap;
    if (t_font) dl->AddText(t_font, ts, ImVec2(tx, y), tokens::kOffWhite, ui::i18n::t(title));

    char hex_buf[12];
    const ImU8 r = (color >> IM_COL32_R_SHIFT) & 0xFF;
    const ImU8 g = (color >> IM_COL32_G_SHIFT) & 0xFF;
    const ImU8 b = (color >> IM_COL32_B_SHIFT) & 0xFF;
    std::snprintf(hex_buf, sizeof(hex_buf), "#%02X%02X%02X", r, g, b);
    if (hex_font) dl->AddText(hex_font, hs, ImVec2(tx, y + ts + scale::dp(2.0f)), tokens::kToxic, hex_buf);

    const float cv_sz = scale::dp(18.0f);
    const ImVec2 cv(body.x + width - pad - cv_sz * 0.5f, body.y + h * 0.5f);
    draw_chevron_right(dl, cv, cv_sz, tokens::kToxic, scale::dp(2.0f));

    return h;
}

float card_nav(const ImVec2& pos, float width,
               const char* state_key,
               const char* icon_utf8,
               const char* title,
               const char* subtitle,
               bool* out_clicked)
{
    const float pad = scale::dp(14.0f);
    ImFont* t_font = fonts::get(fonts::Face::BrutalistMd);
    ImFont* s_font = fonts::get(fonts::Face::MonoSm);
    const float ts = t_font ? t_font->LegacySize : scale::dp(15.0f);
    const float ss = s_font ? s_font->LegacySize : scale::dp(12.0f);
    const float h = pad * 2.0f + (ts + (subtitle && *subtitle ? ss + scale::dp(3.0f) : 0.0f));

    if (card_culled(pos, h)) {
        if (out_clicked) *out_clicked = false;
        return h;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    char btn_id[64]; std::snprintf(btn_id, sizeof(btn_id), "##nav_%s", state_key ? state_key : title);
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(btn_id, ImVec2(width, h));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    const ImU32 border = hovered ? tokens::kToxic : tokens::kToxicDim;
    const ImU32 bg = hovered ? tokens::kToxicFaint : IM_COL32(0, 0, 0, 0);
    if (bg & IM_COL32_A_MASK) {
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + h), bg);
    }
    prim::dashed_rect(dl, pos, ImVec2(pos.x + width, pos.y + h), border, scale::dp(1.5f), scale::dp(12.0f), scale::dp(8.0f));

    ImFont* icon_font = fonts::get(fonts::Face::IconLg);
    const float icon_sz = icon_font ? icon_font->LegacySize : scale::dp(20.0f);
    const float tx_pad = scale::dp(12.0f);
    float tx = pos.x + pad;
    if (icon_font && icon_utf8) {
        ImVec2 isz = icon_font->CalcTextSizeA(icon_sz, FLT_MAX, 0.0f, icon_utf8);
        dl->AddText(icon_font, icon_sz,
                    ImVec2(tx, pos.y + (h - isz.y) * 0.5f),
                    tokens::kToxic, icon_utf8);
        tx += icon_sz + tx_pad;
    }
    const float cv_sz = scale::dp(18.0f);
    const float avail_tw = (pos.x + width - pad - cv_sz) - tx - scale::dp(6.0f);
    draw_title_subtitle(dl, tx, pos.y, h, avail_tw, title, subtitle,
                        tokens::kOffWhite, tokens::kOffWhiteDim);

    const ImVec2 cv(pos.x + width - pad - cv_sz * 0.5f, pos.y + h * 0.5f);
    draw_chevron_right(dl, cv, cv_sz, tokens::kToxic, scale::dp(2.0f));

    if (out_clicked) *out_clicked = clicked;
    return h;
}

float card_button_action(const ImVec2& pos, float width,
                         const char* id,
                         const char* label,
                         ImU32 bg_color,
                         ImU32 fg_color,
                         bool* out_clicked)
{
    ImFont* f = fonts::get(fonts::Face::BrutalistMd);
    const float fs = f ? f->LegacySize : scale::dp(14.0f);
    const float pad_y = scale::dp(12.0f);
    const float h = pad_y * 2.0f + fs;

    if (card_culled(pos, h)) {
        if (out_clicked) *out_clicked = false;
        return h;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    char btn_id[64]; std::snprintf(btn_id, sizeof(btn_id), "##btn_%s", id ? id : label);
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(btn_id, ImVec2(width, h));
    const bool pressed = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked();

    const ImVec2 shadow_off = pressed ? ImVec2(1.0f, 1.0f) : ImVec2(scale::dp(tokens::kShadowOffsetThick), scale::dp(tokens::kShadowOffsetThick));
    dl->AddRectFilled(ImVec2(pos.x + shadow_off.x, pos.y + shadow_off.y),
                      ImVec2(pos.x + width + shadow_off.x, pos.y + h + shadow_off.y),
                      tokens::kBlack);
    ImVec2 fp = pos;
    if (pressed) { fp.x += 2; fp.y += 2; }
    dl->AddRectFilled(fp, ImVec2(fp.x + width, fp.y + h), bg_color);
    dl->AddRect(fp, ImVec2(fp.x + width, fp.y + h), tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThick));

    if (f) {
        ImVec2 lsz = f->CalcTextSizeA(fs, FLT_MAX, 0.0f, label);
        dl->AddText(f, fs,
                    ImVec2(fp.x + (width - lsz.x) * 0.5f, fp.y + pad_y),
                    fg_color, label);
    }

    if (clicked) {
        fx::glitch::notify(icons::k_bolt, ui::i18n::t(label ? label : (id ? id : "ACTION")));
    }

    if (out_clicked) *out_clicked = clicked;
    return h;
}

float card_text_input(const ImVec2& pos, float width,
                      const char* state_key,
                      const char* title,
                      const char* hint,
                      const char* footer,
                      int max_len)
{
    ImFont* title_f  = fonts::get(fonts::Face::BrutalistMd);
    ImFont* mono_f   = fonts::get(fonts::Face::MonoSm);
    const float title_fs = title_f ? title_f->LegacySize : scale::dp(14.0f);
    const float mono_fs  = mono_f  ? mono_f->LegacySize  : scale::dp(12.0f);

    const float pad_x = scale::dp(12.0f);
    const float pad_y = scale::dp(10.0f);
    const float input_h = scale::dp(34.0f);
    const float footer_h = footer && *footer ? mono_fs + scale::dp(4.0f) : 0.0f;
    const float h = pad_y * 2.0f + title_fs + scale::dp(6.0f) + input_h + (footer_h > 0.0f ? scale::dp(6.0f) + footer_h : 0.0f);

    if (card_culled(pos, h)) return h;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    draw_card_bg(dl, pos, ImVec2(width, h), false);

    if (title_f && title && *title) {
        dl->AddText(title_f, title_fs,
                    ImVec2(pos.x + pad_x, pos.y + pad_y),
                    tokens::kOffWhite, ui::i18n::t(title));
    }

    if (max_len < 4) max_len = 4;
    if (max_len > 64) max_len = 64;

    std::string current = state::get_string(state_key ? state_key : "", "");
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s", current.c_str());

    const ImVec2 input_pos(pos.x + pad_x, pos.y + pad_y + title_fs + scale::dp(6.0f));
    const float input_w = width - pad_x * 2.0f;

    ImGui::SetCursorScreenPos(input_pos);
    ImGui::PushItemWidth(input_w);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, tokens::kBlack);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, tokens::kBlack);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, tokens::kBlack);
    ImGui::PushStyleColor(ImGuiCol_Text, tokens::kToxic);
    const float frame_pad_y = (input_h - mono_fs) * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(scale::dp(8.0f), frame_pad_y > 0 ? frame_pad_y : scale::dp(4.0f)));

    ImFont* push_font = mono_f ? mono_f : ImGui::GetFont();
    ImGui::PushFont(push_font, push_font->LegacySize);

    char id_buf[96];
    std::snprintf(id_buf, sizeof(id_buf), "##tin_%s", state_key ? state_key : "x");

    const bool changed = ImGui::InputTextWithHint(
        id_buf,
        (hint && *hint) ? hint : "",
        buf, static_cast<size_t>(max_len) + 1,
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);

    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::PopItemWidth();

    if (changed) {
        state::set_string(state_key ? state_key : "", buf);
    }

    if (footer_h > 0.0f && mono_f) {
        dl->AddText(mono_f, mono_fs,
                    ImVec2(pos.x + pad_x, input_pos.y + input_h + scale::dp(6.0f)),
                    tokens::kOffWhiteDim, footer);
    }

    return h;
}

float card_fqa_size(const ImVec2& pos, float width) {
    const char* state_key = "fqa.size_mult";
    const float min_v = 0.4f;
    const float max_v = 1.5f;
    const float step  = 0.05f;
    const float def_v = 0.7f;

    if (!state::has(state_key)) state::set_float(state_key, def_v);
    float v = state::get_float(state_key, def_v);

    const float pad = scale::dp(tokens::kCardPadding);
    ImFont* t_font   = fonts::get(fonts::Face::BrutalistMd);
    ImFont* mono_sm  = fonts::get(fonts::Face::MonoSm);
    ImFont* mono_xs  = fonts::get(fonts::Face::MonoXs);
    ImFont* icon_font= fonts::get(fonts::Face::IconMd);
    ImFont* big_icon = fonts::get(fonts::Face::IconLg);

    const float ts       = t_font  ? t_font->LegacySize  : scale::dp(15.0f);
    const float ms       = mono_sm ? mono_sm->LegacySize : scale::dp(12.0f);
    const float lbl_s    = mono_xs ? mono_xs->LegacySize : scale::dp(11.0f);
    const float header_h = (ts > ms ? ts : ms) + scale::dp(4.0f);
    const float track_h  = scale::dp(10.0f);
    const float label_h  = lbl_s + scale::dp(2.0f);

    const float preview_btn_max = scale::dp(tokens::kFqaButtonSize) * max_v;
    const float preview_h = preview_btn_max + scale::dp(14.0f);

    const float h = pad * 2.0f + header_h + scale::dp(8.0f)
                  + preview_h + scale::dp(10.0f)
                  + track_h + scale::dp(6.0f) + label_h;

    if (card_culled(pos, h)) return h;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char btn_id[64]; std::snprintf(btn_id, sizeof(btn_id), "##slider_%s", state_key);

    const ImVec2 track_min(pos.x + pad,
                           pos.y + pad + header_h + scale::dp(8.0f) + preview_h + scale::dp(10.0f));
    const ImVec2 track_max(pos.x + width - pad, track_min.y + track_h);

    ImGui::SetCursorScreenPos(ImVec2(track_min.x, track_min.y - scale::dp(10.0f)));
    ImGui::InvisibleButton(btn_id, ImVec2(track_max.x - track_min.x, track_h + scale::dp(20.0f)));
    const bool active  = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    if (active) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        float p = (m.x - track_min.x) / (track_max.x - track_min.x);
        if (p < 0.0f) p = 0.0f; if (p > 1.0f) p = 1.0f;
        float new_v = min_v + p * (max_v - min_v);
        if (step > 0.0f) new_v = std::round(new_v / step) * step;
        if (new_v < min_v) new_v = min_v; if (new_v > max_v) new_v = max_v;
        state::set_float(state_key, new_v);
        v = new_v;
    }

    draw_card_bg(dl, pos, ImVec2(width, h), false);

    if (icon_font) {
        const float isz = icon_font->LegacySize;
        ImVec2 msz = icon_font->CalcTextSizeA(isz, FLT_MAX, 0.0f, icons::k_tune);
        dl->AddText(icon_font, isz,
                    ImVec2(pos.x + pad, pos.y + pad + (header_h - msz.y) * 0.5f),
                    tokens::kToxic, icons::k_tune);
    }
    if (t_font) {
        const float icon_w = icon_font ? icon_font->LegacySize + scale::dp(6.0f) : 0.0f;
        dl->AddText(t_font, ts,
                    ImVec2(pos.x + pad + icon_w, pos.y + pad),
                    tokens::kOffWhite, "FQA_SIZE");
    }

    {
        char val_buf[16];
        std::snprintf(val_buf, sizeof(val_buf), "%d%%",
                      static_cast<int>(std::round(v * 100.0f)));
        if (mono_sm) {
            ImVec2 vsz = mono_sm->CalcTextSizeA(ms, FLT_MAX, 0.0f, val_buf);
            const float vx = pos.x + width - pad - vsz.x - scale::dp(8.0f);
            const float vy = pos.y + pad + (header_h - vsz.y) * 0.5f;
            const ImVec2 bg_min(vx - scale::dp(8.0f), vy - scale::dp(3.0f));
            const ImVec2 bg_max(vx + vsz.x + scale::dp(8.0f), vy + vsz.y + scale::dp(3.0f));
            dl->AddRectFilled(bg_min, bg_max, tokens::kBlack);
            dl->AddRect(bg_min, bg_max, tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));
            dl->AddText(mono_sm, ms, ImVec2(vx, vy), tokens::kToxic, val_buf);
        }
    }

    {
        const float preview_top = pos.y + pad + header_h + scale::dp(8.0f);
        const ImVec2 pv_min(pos.x + pad, preview_top);
        const ImVec2 pv_max(pos.x + width - pad, preview_top + preview_h);

        dl->AddRectFilled(pv_min, pv_max, IM_COL32(0x00, 0x00, 0x00, 0x40));
        const int dash_segs = 24;
        for (int i = 0; i < dash_segs; ++i) {
            if ((i & 1) == 0) continue;
            const float x0 = pv_min.x + (pv_max.x - pv_min.x) * (static_cast<float>(i) / dash_segs);
            const float x1 = pv_min.x + (pv_max.x - pv_min.x) * (static_cast<float>(i + 1) / dash_segs);
            dl->AddLine(ImVec2(x0, pv_min.y + scale::dp(1.0f)),
                        ImVec2(x1, pv_min.y + scale::dp(1.0f)),
                        tokens::kToxicDim, scale::dp(1.0f));
            dl->AddLine(ImVec2(x0, pv_max.y - scale::dp(1.0f)),
                        ImVec2(x1, pv_max.y - scale::dp(1.0f)),
                        tokens::kToxicDim, scale::dp(1.0f));
        }

        const float cur_btn = scale::dp(tokens::kFqaButtonSize) * v;
        const float mult_preview = cur_btn / scale::dp(tokens::kFqaButtonSize);
        const float cx = (pv_min.x + pv_max.x) * 0.5f;
        const float cy = (pv_min.y + pv_max.y) * 0.5f;
        const float radius = cur_btn * 0.5f;

        const float shadow_off = scale::dp(3.0f) * mult_preview;
        dl->AddCircleFilled(ImVec2(cx + shadow_off, cy + shadow_off),
                            radius, tokens::kBlack, 48);
        dl->AddCircleFilled(ImVec2(cx, cy), radius, tokens::kToxic, 48);
        dl->AddCircle(ImVec2(cx, cy), radius, tokens::kBlack, 48,
                      scale::dp(tokens::kBorderThick) * mult_preview);

        if (big_icon) {
            const float isz = big_icon->LegacySize * 1.1f * mult_preview;
            ImVec2 esz = big_icon->CalcTextSizeA(isz, FLT_MAX, 0.0f, icons::k_analytics);
            dl->AddText(big_icon, isz,
                        ImVec2(cx - esz.x * 0.5f, cy - esz.y * 0.5f),
                        tokens::kBlack, icons::k_analytics);
        }

        if (mono_xs) {
            const float cfs = mono_xs->LegacySize;
            char buf[24];
            std::snprintf(buf, sizeof(buf), "%dpx",
                          static_cast<int>(std::round(cur_btn)));
            ImVec2 tsz = mono_xs->CalcTextSizeA(cfs, FLT_MAX, 0.0f, buf);
            const float tx = cx + radius + scale::dp(12.0f);
            const float ty = cy - tsz.y * 0.5f;
            if (tx + tsz.x + scale::dp(6.0f) < pv_max.x) {
                dl->AddRectFilled(ImVec2(tx - scale::dp(6.0f), ty - scale::dp(2.0f)),
                                  ImVec2(tx + tsz.x + scale::dp(6.0f), ty + tsz.y + scale::dp(2.0f)),
                                  tokens::kBlack);
                dl->AddText(mono_xs, cfs, ImVec2(tx, ty), tokens::kToxic, buf);
            }
        }
    }

    dl->AddRectFilled(track_min, track_max, tokens::kBlack);
    dl->AddRect(track_min, track_max, tokens::kBlack, 0.0f, 0, scale::dp(tokens::kBorderThin));

    const float denom = (max_v - min_v);
    const float pct = denom != 0.0f ? ((v - min_v) / denom) : 0.0f;
    const float fill_w = (track_max.x - track_min.x) * pct;
    dl->AddRectFilled(track_min, ImVec2(track_min.x + fill_w, track_max.y), tokens::kToxic);

    char knob_anim_id[96]; std::snprintf(knob_anim_id, sizeof(knob_anim_id), "anim_knob_%s", state_key);
    float& knob_phase = get_anim_state(knob_anim_id, 0.0f);
    const float knob_target = (active || hovered) ? 1.0f : 0.0f;
    knob_phase = fx::anim::spring_to(knob_phase, knob_target, ImGui::GetIO().DeltaTime, 16.0f);

    const float knob_base = scale::dp(18.0f);
    const float knob_sz = knob_base + scale::dp(4.0f) * knob_phase;
    const float kx = track_min.x + fill_w;
    const float ky = (track_min.y + track_max.y) * 0.5f;
    if (knob_phase > 0.01f) {
        const float halo = scale::dp(12.0f) * knob_phase;
        dl->AddRectFilled(ImVec2(kx - knob_sz * 0.5f - halo, ky - knob_sz * 0.5f - halo),
                          ImVec2(kx + knob_sz * 0.5f + halo, ky + knob_sz * 0.5f + halo),
                          IM_COL32(0xCC, 0xFF, 0x00, static_cast<int>(0x33 * knob_phase)));
    }
    dl->AddRectFilled(ImVec2(kx - knob_sz * 0.5f, ky - knob_sz * 0.5f),
                      ImVec2(kx + knob_sz * 0.5f, ky + knob_sz * 0.5f),
                      tokens::kBlack);
    dl->AddRect(ImVec2(kx - knob_sz * 0.5f, ky - knob_sz * 0.5f),
                ImVec2(kx + knob_sz * 0.5f, ky + knob_sz * 0.5f),
                tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));

    if (mono_xs) {
        char min_buf[16], max_buf[16];
        std::snprintf(min_buf, sizeof(min_buf), "%d%%", static_cast<int>(std::round(min_v * 100.0f)));
        std::snprintf(max_buf, sizeof(max_buf), "%d%%", static_cast<int>(std::round(max_v * 100.0f)));
        const float y = track_max.y + scale::dp(6.0f);
        dl->AddText(mono_xs, lbl_s, ImVec2(track_min.x, y), tokens::kOffWhiteDim, min_buf);
        ImVec2 msz = mono_xs->CalcTextSizeA(lbl_s, FLT_MAX, 0.0f, max_buf);
        dl->AddText(mono_xs, lbl_s, ImVec2(track_max.x - msz.x, y), tokens::kOffWhiteDim, max_buf);
    }

    return h;
}

}
