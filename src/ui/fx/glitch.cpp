#include "glitch.hpp"
#include "animation.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"

#include <cmath>
#include <cstring>
#include <cstdio>

namespace ui::fx::glitch {

namespace {

State* g_active = nullptr;

float interp_keys(float t, const float* ks, const float* vs, int n) {
    if (t <= ks[0]) return vs[0];
    if (t >= ks[n - 1]) return vs[n - 1];
    for (int i = 1; i < n; ++i) {
        if (t <= ks[i]) {
            const float a = (t - ks[i - 1]) / (ks[i] - ks[i - 1]);
            return vs[i - 1] + (vs[i] - vs[i - 1]) * a;
        }
    }
    return vs[n - 1];
}

ImU32 col_a(ImU32 c, float a) {
    const int al = static_cast<int>(((c >> IM_COL32_A_SHIFT) & 0xFF) * fx::anim::clamp01(a));
    return (c & 0x00FFFFFF) | (static_cast<ImU32>(al) << IM_COL32_A_SHIFT);
}

void draw_icon_with_glow(ImDrawList* dl, ImFont* font, float size,
                         const ImVec2& center, const char* glyph,
                         float opacity, float rot_deg)
{
    if (!font || !glyph || !*glyph) return;
    ImVec2 sz = font->CalcTextSizeA(size, FLT_MAX, 0.0f, glyph);
    const ImVec2 base(center.x - sz.x * 0.5f, center.y - sz.y * 0.5f);

    const float spread = scale::dp(5.0f);
    const float glow_a = opacity * 0.22f;
    if (glow_a > 0.01f) {
        const ImU32 glow_col = col_a(tokens::kToxic, glow_a);
        dl->AddText(font, size, ImVec2(base.x,          base.y - spread), glow_col, glyph);
        dl->AddText(font, size, ImVec2(base.x,          base.y + spread), glow_col, glyph);
        dl->AddText(font, size, ImVec2(base.x - spread, base.y),          glow_col, glyph);
        dl->AddText(font, size, ImVec2(base.x + spread, base.y),          glow_col, glyph);
    }

    const float split = scale::dp(3.0f);
    dl->AddText(font, size, ImVec2(base.x - split, base.y),
                col_a(tokens::kErrorRed, opacity * 0.85f), glyph);
    dl->AddText(font, size, ImVec2(base.x + split, base.y),
                col_a(tokens::kCyan, opacity * 0.85f), glyph);

    dl->AddText(font, size, base, col_a(tokens::kToxic, opacity), glyph);
    (void)rot_deg;
}

}

void trigger(State& s, const char* icon_utf8, const char* label) {
    s.phase = 1.0f;
    if (icon_utf8) {
        std::snprintf(s.icon_utf8, sizeof(s.icon_utf8), "%s", icon_utf8);
    } else {
        s.icon_utf8[0] = '\0';
    }
    if (label) {
        std::snprintf(s.label, sizeof(s.label), "%s", label);
    } else {
        s.label[0] = '\0';
    }
}

void set_active(State* s) { g_active = s; }

void notify(const char* icon_utf8, const char* label) {
    if (g_active) trigger(*g_active, icon_utf8, label);
}

void render(State& s, const ImVec2& viewport_min, const ImVec2& viewport_max, float dt) {
    if (s.phase <= 0.0f) return;
    s.phase -= dt / s.duration;
    if (s.phase < 0.0f) s.phase = 0.0f;
    if (s.phase == 0.0f) return;

    const float p = 1.0f - s.phase;

    static const float dim_k[] = { 0.0f, 0.22f, 0.65f, 1.0f };
    static const float dim_v[] = { 0.0f, 1.0f,  0.6f,  0.0f };
    const float dim = interp_keys(p, dim_k, dim_v, 4);

    static const float icon_op_k[] = { 0.0f, 0.35f, 0.65f, 1.0f };
    static const float icon_op_v[] = { 0.0f, 1.0f,  1.0f,  0.0f };
    const float icon_op = interp_keys(p, icon_op_k, icon_op_v, 4);

    static const float icon_s_k[] = { 0.0f, 0.35f, 0.65f, 1.0f };
    static const float icon_s_v[] = { 0.5f, 1.2f,  1.0f,  1.05f };
    const float icon_scale = interp_keys(p, icon_s_k, icon_s_v, 4);

    static const float lbl_op_k[] = { 0.0f, 0.45f, 0.8f, 1.0f };
    static const float lbl_op_v[] = { 0.0f, 1.0f,  1.0f, 0.0f };
    const float lbl_op = interp_keys(p, lbl_op_k, lbl_op_v, 4);

    static const float lbl_ty_k[] = { 0.0f, 0.45f, 1.0f };
    static const float lbl_ty_v[] = { 12.0f,0.0f,  0.0f };
    const float lbl_ty = interp_keys(p, lbl_ty_k, lbl_ty_v, 3);

    const float shake_phase = p * 5.0f;
    float sx = 0.0f, sy = 0.0f;
    const int stage = static_cast<int>(shake_phase) % 5;
    switch (stage) {
        case 0: sx = -3.0f; sy = 1.0f;  break;
        case 1: sx = 2.0f;  sy = -2.0f; break;
        case 2: sx = -1.0f; sy = 1.0f;  break;
        case 3: sx = 1.0f;  sy = 0.0f;  break;
        default: sx = 0.0f; sy = 0.0f;  break;
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const ImU32 dim_col = IM_COL32(0, 0, 0, static_cast<int>(0x73 * dim));
    dl->AddRectFilled(viewport_min, viewport_max, dim_col);

    const ImVec2 center((viewport_min.x + viewport_max.x) * 0.5f + sx,
                        (viewport_min.y + viewport_max.y) * 0.5f + sy);

    ImFont* icon_font = fonts::get(fonts::Face::IconXl);
    if (!icon_font) icon_font = fonts::get(fonts::Face::IconLg);
    if (icon_font && s.icon_utf8[0]) {
        const float base_size = scale::dp(112.0f);
        const float isz = base_size * icon_scale;
        draw_icon_with_glow(dl, icon_font, isz, center, s.icon_utf8, icon_op, 0.0f);
    }

    if (s.label[0]) {
        ImFont* lf = fonts::get(fonts::Face::BrutalistSm);
        const float lfs = lf ? lf->LegacySize : scale::dp(13.0f);
        ImVec2 lsz = lf ? lf->CalcTextSizeA(lfs, FLT_MAX, 0.0f, s.label) : ImGui::CalcTextSize(s.label);
        const float pad_x = scale::dp(12.0f);
        const float pad_y = scale::dp(5.0f);
        const float bw = lsz.x + pad_x * 2.0f;
        const float bh = lsz.y + pad_y * 2.0f;
        const float bx = center.x - bw * 0.5f;
        const float by = center.y + scale::dp(56.0f) + scale::dp(lbl_ty);

        const float shadow_off = scale::dp(3.0f);
        dl->AddRectFilled(ImVec2(bx + shadow_off, by + shadow_off),
                          ImVec2(bx + bw + shadow_off, by + bh + shadow_off),
                          col_a(tokens::kBlack, lbl_op));
        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + bh),
                          col_a(tokens::kToxic, lbl_op));
        dl->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh),
                    col_a(tokens::kBlack, lbl_op), 0.0f, 0, scale::dp(tokens::kBorderThin));
        if (lf) {
            dl->AddText(lf, lfs,
                        ImVec2(bx + pad_x, by + pad_y),
                        col_a(tokens::kBlack, lbl_op), s.label);
        }
    }
}

}
