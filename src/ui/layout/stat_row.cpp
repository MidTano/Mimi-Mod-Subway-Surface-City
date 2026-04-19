#include "stat_row.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../state/mod_state.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../i18n/lang.hpp"
#include "core/il2cpp/il2cpp_bridge.hpp"

#include <cstdio>
#include <cstdint>
#include <cstring>

namespace ui::layout::stat_row {

namespace {

constexpr ImU32 kPanelBg = IM_COL32(0x14, 0x14, 0x14, 0xFF);
constexpr ImU32 kDividerCol = IM_COL32(0xCC, 0xFF, 0x00, 0x33);

void format_distance(char* out, std::size_t cap, int meters) {
    if (meters < 1000) {
        std::snprintf(out, cap, "%d", meters);
    } else if (meters < 10000) {
        std::snprintf(out, cap, "%.2fK", meters / 1000.0f);
    } else if (meters < 100000) {
        std::snprintf(out, cap, "%.1fK", meters / 1000.0f);
    } else if (meters < 1000000) {
        std::snprintf(out, cap, "%dK", meters / 1000);
    } else {
        std::snprintf(out, cap, "%.2fM", meters / 1000000.0f);
    }
}

void draw_stat(ImDrawList* dl, float cx_min, float cx_max, float y_top, float y_bot,
               const char* label, const char* value, const char* unit, float alpha) {
    ImFont* mono = fonts::get(fonts::Face::MonoSm);
    if (!mono) return;
    const float fs = mono->LegacySize;
    const float h = y_bot - y_top;

    const char* label_tr = ui::i18n::t(label);
    const char* unit_tr  = (unit && *unit) ? ui::i18n::t(unit) : unit;
    const ImVec2 label_sz = mono->CalcTextSizeA(fs, FLT_MAX, 0.0f, label_tr);
    const ImVec2 value_sz = mono->CalcTextSizeA(fs, FLT_MAX, 0.0f, value);
    const ImVec2 unit_sz  = unit_tr && *unit_tr ? mono->CalcTextSizeA(fs, FLT_MAX, 0.0f, unit_tr) : ImVec2(0,0);

    const float gap = scale::dp(4.0f);
    const float total = label_sz.x + gap + value_sz.x + (unit_sz.x > 0 ? gap + unit_sz.x : 0);
    float x = cx_min + ((cx_max - cx_min) - total) * 0.5f;
    if (x < cx_min + scale::dp(3.0f)) x = cx_min + scale::dp(3.0f);
    const float y = y_top + (h - label_sz.y) * 0.5f;

    const ImU32 tox = tokens::with_alpha_mul(tokens::kToxic, alpha);
    const ImU32 ofw = tokens::with_alpha_mul(tokens::kOffWhite, alpha);

    dl->AddText(mono, fs, ImVec2(x, y), tox, label_tr);
    x += label_sz.x + gap;
    dl->AddText(mono, fs, ImVec2(x, y), ofw, value);
    if (unit_sz.x > 0) {
        x += value_sz.x + gap;
        dl->AddText(mono, fs, ImVec2(x, y), tox, unit_tr);
    }
}

}

void render(const ImVec2& min, const ImVec2& max) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(min, max, kPanelBg);
    dl->AddRect(min, max, tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));

    const float w = max.x - min.x;
    const float pad = scale::dp(5.0f);
    const float gap = scale::dp(5.0f);

    constexpr int kChipCount = 3;
    const float chip_w = (w - pad * 2.0f - gap * static_cast<float>(kChipCount - 1))
                         / static_cast<float>(kChipCount);
    const float y0 = min.y;
    const float y1 = max.y;

    const float spd = il2cpp::get_current_player_speed();
    const int   dst = static_cast<int>(il2cpp::get_current_run_distance());
    const float ts  = il2cpp::get_run_time_seconds();

    char spd_buf[16]; std::snprintf(spd_buf, sizeof(spd_buf), "%.1f", spd);
    char dst_buf[16]; format_distance(dst_buf, sizeof(dst_buf), dst);
    char tim_buf[16]; {
        int mins = static_cast<int>(ts) / 60;
        float rem = ts - mins * 60.0f;
        std::snprintf(tim_buf, sizeof(tim_buf), "%02d:%05.2f", mins, rem);
    }

    float x = min.x + pad;
    draw_stat(dl, x, x + chip_w, y0, y1, "SPD", spd_buf, "m/s", 1.0f);
    x += chip_w;
    dl->AddLine(ImVec2(x + gap * 0.5f, y0 + scale::dp(4.0f)),
                ImVec2(x + gap * 0.5f, y1 - scale::dp(4.0f)),
                kDividerCol, 1.0f);
    x += gap;
    draw_stat(dl, x, x + chip_w, y0, y1, "DIST", dst_buf, "m", 1.0f);
    x += chip_w;
    dl->AddLine(ImVec2(x + gap * 0.5f, y0 + scale::dp(4.0f)),
                ImVec2(x + gap * 0.5f, y1 - scale::dp(4.0f)),
                kDividerCol, 1.0f);
    x += gap;
    draw_stat(dl, x, x + chip_w, y0, y1, "TIME", tim_buf, "", 1.0f);
}

void render_unified(const ImVec2& min, const ImVec2& max, const UnifiedStyle& style) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const float btn_w = style.button_width;
    const float h = max.y - min.y;
    const float notch = scale::dp(10.0f);
    const float border = scale::dp(tokens::kBorderThin);
    const float a = (style.alpha <= 0.0f) ? 1.0f : style.alpha;
    const ImU32 c_toxic   = tokens::with_alpha_mul(tokens::kToxic, a);
    const ImU32 c_panel   = tokens::with_alpha_mul(kPanelBg, a);
    const ImU32 c_shadow  = tokens::with_alpha_mul(IM_COL32(0x00, 0x00, 0x00, 0xAA), a);
    const ImU32 c_divider = tokens::with_alpha_mul(kDividerCol, a);
    const ImU32 c_black   = tokens::with_alpha_mul(tokens::kBlack, a);

    ImFont* mono_seed = fonts::get(fonts::Face::MonoXs);
    const float sfs = mono_seed ? mono_seed->LegacySize : scale::dp(10.0f);
    const std::uint32_t seed = il2cpp::get_effective_seed();
    char seed_buf[24];
    if (seed == 0) {
        std::snprintf(seed_buf, sizeof(seed_buf), "----");
    } else {
        std::uint32_t s = (seed > 9999999u) ? (seed % 10000000u) : seed;
        std::snprintf(seed_buf, sizeof(seed_buf), "%u", s);
    }
    const char* seed_label_tr = ui::i18n::t("SEED");
    char seed_full[48];
    std::snprintf(seed_full, sizeof(seed_full), "%s %s", seed_label_tr, seed_buf);
    const ImVec2 seed_tsz = mono_seed ? mono_seed->CalcTextSizeA(sfs, FLT_MAX, 0.0f, seed_full)
                                      : ImVec2(scale::dp(80.0f), scale::dp(10.0f));

    const float seed_pad_x = scale::dp(8.0f);
    const float seed_pad_y = scale::dp(3.0f);
    const float seed_w = seed_tsz.x + seed_pad_x * 2.0f;
    const float seed_h = seed_tsz.y + seed_pad_y * 2.0f;

    const float offx = style.btn_active ? scale::dp(1.0f) : 0.0f;
    const float offy = style.btn_active ? scale::dp(1.0f) : 0.0f;

    const float L  = min.x + offx;
    const float R  = max.x + offx;
    const float T  = min.y + offy;
    const float B  = max.y + offy;
    const float SX1 = R;
    const float SX0 = R - seed_w;
    const float SY  = B + seed_h;

    const float shadow_off = scale::dp(3.0f);
    dl->AddRectFilled(ImVec2(L + shadow_off, T + shadow_off),
                      ImVec2(R + shadow_off, B + shadow_off),
                      c_shadow);

    const ImU32 btn_bg = style.btn_hovered ? c_toxic : c_panel;
    dl->AddRectFilled(ImVec2(L, T), ImVec2(L + btn_w, B), btn_bg);
    dl->AddRectFilled(ImVec2(L + btn_w, T), ImVec2(R, B), c_panel);

    ImVec2 seed_poly[5] = {
        ImVec2(SX0,         B),
        ImVec2(SX1,         B),
        ImVec2(SX1,         SY),
        ImVec2(SX0 + notch, SY),
        ImVec2(SX0,         SY - notch),
    };
    dl->AddConvexPolyFilled(seed_poly, 5, c_panel);

    dl->PathLineTo(ImVec2(L, T));
    dl->PathLineTo(ImVec2(R, T));
    dl->PathLineTo(ImVec2(R, B));
    dl->PathLineTo(ImVec2(R, SY));
    dl->PathLineTo(ImVec2(SX0 + notch, SY));
    dl->PathLineTo(ImVec2(SX0, SY - notch));
    dl->PathLineTo(ImVec2(SX0, B));
    dl->PathLineTo(ImVec2(L, B));
    dl->PathStroke(c_toxic, ImDrawFlags_Closed, border);

    dl->AddLine(ImVec2(L + btn_w, T), ImVec2(L + btn_w, B),
                c_toxic, border);

    ImFont* ico = fonts::get(fonts::Face::IconLg);
    if (ico) {
        const float isz = ico->LegacySize;
        const ImVec2 esz = ico->CalcTextSizeA(isz, FLT_MAX, 0.0f, icons::k_bolt);
        const float ix = L + (btn_w - esz.x) * 0.5f;
        const float iy = T + (h - esz.y) * 0.5f;
        dl->AddText(ico, isz, ImVec2(ix, iy),
                    style.btn_hovered ? c_black : c_toxic, icons::k_bolt);
    }

    const float spd = il2cpp::get_current_player_speed();
    const int   dst = static_cast<int>(il2cpp::get_current_run_distance());
    const float ts  = il2cpp::get_run_time_seconds();
    char spd_buf[16]; std::snprintf(spd_buf, sizeof(spd_buf), "%.1f", spd);
    char dst_buf[16]; format_distance(dst_buf, sizeof(dst_buf), dst);
    char tim_buf[16]; {
        int mins = static_cast<int>(ts) / 60;
        float rem = ts - mins * 60.0f;
        std::snprintf(tim_buf, sizeof(tim_buf), "%02d:%05.2f", mins, rem);
    }

    const float inner_pad = scale::dp(4.0f);
    const float col_gap = scale::dp(4.0f);
    const float panel_w = R - (L + btn_w) - inner_pad * 2.0f;
    const float chip_w = (panel_w - col_gap * 2.0f) / 3.0f;

    float cx = L + btn_w + inner_pad;
    draw_stat(dl, cx, cx + chip_w, T, B, "SPD", spd_buf, "m/s", a);
    cx += chip_w;
    dl->AddLine(ImVec2(cx + col_gap * 0.5f, T + scale::dp(5.0f)),
                ImVec2(cx + col_gap * 0.5f, B - scale::dp(5.0f)),
                c_divider, 1.0f);
    cx += col_gap;
    draw_stat(dl, cx, cx + chip_w, T, B, "DIST", dst_buf, "m", a);
    cx += chip_w;
    dl->AddLine(ImVec2(cx + col_gap * 0.5f, T + scale::dp(5.0f)),
                ImVec2(cx + col_gap * 0.5f, B - scale::dp(5.0f)),
                c_divider, 1.0f);
    cx += col_gap;
    draw_stat(dl, cx, cx + chip_w, T, B, "TIME", tim_buf, "", a);

    if (mono_seed) {
        const float tx = SX0 + seed_pad_x;
        const float ty = B + seed_pad_y;
        char seed_prefix[32];
        std::snprintf(seed_prefix, sizeof(seed_prefix), "%s ", seed_label_tr);
        dl->AddText(mono_seed, sfs, ImVec2(tx, ty), c_toxic, seed_prefix);
        const ImVec2 lsz = mono_seed->CalcTextSizeA(sfs, FLT_MAX, 0.0f, seed_prefix);
        const ImU32 c_off = tokens::with_alpha_mul(tokens::kOffWhite, a);
        dl->AddText(mono_seed, sfs, ImVec2(tx + lsz.x, ty), c_off, seed_buf);
    }
}

}
