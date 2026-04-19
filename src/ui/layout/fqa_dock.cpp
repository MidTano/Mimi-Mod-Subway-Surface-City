#include "fqa_dock.hpp"
#include "../fx/glitch.hpp"
#include "../fx/animation.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../state/mod_state.hpp"
#include "../i18n/lang.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ui::layout::fqa_dock {

namespace {

std::atomic<float> g_hud_opacity{1.0f};

enum class FType { Toggle, Arc, PieSingle, PieMulti, Action };

struct PieOption {
    const char* icon_utf8;
    const char* label;
    const char* key;
    int value;
};

struct Feature {
    const char* id;
    const char* icon_utf8;
    const char* label;
    FType type;
    float arc_min;
    float arc_max;
    float arc_step;
    const char* arc_fmt;
    const PieOption* options;
    int option_count;
};

constexpr PieOption kPowerOptions[] = {
    { icons::k_near_me,          "MAG", "pw.magnet",    0 },
    { icons::k_close_fullscreen, "2X",  "pw.double",    0 },
    { icons::k_blur_on,          "GUM", "pw.bubble_gum",0 },
    { icons::k_all_inclusive,    "ALL", "pw.all",       0 },
};

const Feature kFeatures[] = {
    { "hud.visible",        icons::k_analytics,      "HUD",   FType::Toggle,    0.0f, 0.0f, 0.0f, "",  nullptr,         0 },
    { "time_scale.value",   icons::k_schedule,       "TIME",  FType::Arc,       0.05f, 3.0f, 0.01f, "x", nullptr,        0 },
    { "run.invincibility",  icons::k_shield,         "INV",   FType::Toggle,    0.0f, 0.0f, 0.0f, "",  nullptr,         0 },
    { "death.skip_screen",  icons::k_visibility_off, "SKIP",  FType::Toggle,    0.0f, 0.0f, 0.0f, "",  nullptr,         0 },
    { "run.auto_restart",   icons::k_autorenew,      "REST",  FType::Toggle,    0.0f, 0.0f, 0.0f, "",  nullptr,         0 },
    { "pw.all",             icons::k_rocket_launch,  "POWER", FType::PieMulti,  0.0f, 0.0f, 0.0f, "",  kPowerOptions,   4 },
    { "mode.mirror",        icons::k_flip,           "MIRR",  FType::Toggle,    0.0f, 0.0f, 0.0f, "",  nullptr,         0 },
};

int count_power_active() {
    int n = 0;
    for (int i = 0; i < 4; ++i) {
        if (state::get_bool(kPowerOptions[i].key, false)) ++n;
    }
    return n;
}

bool is_feature_active(const Feature& f) {
    (void)f;
    return true;
}

bool is_feature_on(const Feature& f) {
    switch (f.type) {
        case FType::Toggle:    return state::get_bool(f.id, false);
        case FType::PieSingle: return state::get_int(f.id, 0) > 0;
        case FType::PieMulti:  return count_power_active() > 0;
        default:               return false;
    }
}

float& get_anim_state(const char* key, float initial) {
    ImGuiID id = ImGui::GetID(key);
    return *ImGui::GetStateStorage()->GetFloatRef(id, initial);
}

ImU32 col_a(ImU32 c, float a) {
    if (a <= 0.0f) return 0;
    if (a >= 1.0f) a = 1.0f;
    const int al = static_cast<int>(((c >> IM_COL32_A_SHIFT) & 0xFF) * a);
    return (c & 0x00FFFFFF) | (static_cast<ImU32>(al) << IM_COL32_A_SHIFT);
}

ImU32 desaturate(ImU32 c, float amount) {
    const ImU8 r = (c >> IM_COL32_R_SHIFT) & 0xFF;
    const ImU8 g = (c >> IM_COL32_G_SHIFT) & 0xFF;
    const ImU8 b = (c >> IM_COL32_B_SHIFT) & 0xFF;
    const ImU8 a = (c >> IM_COL32_A_SHIFT) & 0xFF;
    const float gray = 0.299f * r + 0.587f * g + 0.114f * b;
    const float t = amount < 0.0f ? 0.0f : (amount > 1.0f ? 1.0f : amount);
    const ImU8 nr = static_cast<ImU8>(r + (gray - r) * t);
    const ImU8 ng = static_cast<ImU8>(g + (gray - g) * t);
    const ImU8 nb = static_cast<ImU8>(b + (gray - b) * t);
    return IM_COL32(nr, ng, nb, a);
}

void draw_button(ImDrawList* dl,
                 const ImVec2& pos, float base_size,
                 const char* icon_utf8,
                 bool on, bool hovered, bool pressed, bool engaged,
                 float pulse_phase,
                 float other_engaged,
                 int badge_count,
                 const char* value_tag_text)
{
    const float dim_mix = 0.72f * other_engaged;
    const float alpha_mult = (1.0f - 0.72f * other_engaged) * g_hud_opacity.load();

    const float size_mult = base_size / scale::dp(tokens::kFqaButtonSize);
    const float engage_scale = 1.0f + 0.08f * engaged;
    const float size = base_size * engage_scale;
    const float cx = pos.x + base_size * 0.5f;
    const float cy = pos.y + base_size * 0.5f;
    const float radius = size * 0.5f;

    const float shadow_dp = pressed ? (1.0f * size_mult) : (scale::dp(3.0f) * size_mult);
    const float border_thick = scale::dp(tokens::kBorderThick) * size_mult;

    constexpr int kCircleSegs = 48;

    if (on && !pressed) {
        const float halo = scale::dp(8.0f) * size_mult * pulse_phase;
        if (halo > 0.25f) {
            const ImU32 glow = col_a(tokens::kToxic, 0.35f * pulse_phase * alpha_mult);
            dl->AddCircleFilled(ImVec2(cx, cy), radius + halo, glow, kCircleSegs);
        }
    }

    dl->AddCircleFilled(ImVec2(cx + shadow_dp, cy + shadow_dp), radius,
                        col_a(tokens::kBlack, alpha_mult), kCircleSegs);

    const ImU32 raw_fill = (engaged || on)
                               ? tokens::kToxic
                               : (hovered ? IM_COL32(0x2A, 0x2A, 0x2A, 0xF0)
                                          : IM_COL32(0x12, 0x12, 0x12, 0xF0));
    const ImU32 raw_border = (engaged || on) ? tokens::kBlack : tokens::kToxic;
    const ImU32 fill = col_a(desaturate(raw_fill, dim_mix), alpha_mult);
    const ImU32 border = col_a(desaturate(raw_border, dim_mix), alpha_mult);

    dl->AddCircleFilled(ImVec2(cx, cy), radius, fill, kCircleSegs);
    dl->AddCircle(ImVec2(cx, cy), radius, border, kCircleSegs, border_thick);

    ImFont* icon_font = fonts::get(fonts::Face::IconLg);
    if (icon_font && icon_utf8 && *icon_utf8) {
        const float isz = icon_font->LegacySize * 1.1f * size_mult;
        ImVec2 esz = icon_font->CalcTextSizeA(isz, FLT_MAX, 0.0f, icon_utf8);
        const ImU32 raw_icon_col = (engaged || on) ? tokens::kBlack : tokens::kToxic;
        const ImU32 icon_col = col_a(desaturate(raw_icon_col, dim_mix), alpha_mult);
        dl->AddText(icon_font, isz,
                    ImVec2(cx - esz.x * 0.5f, cy - esz.y * 0.5f),
                    icon_col, icon_utf8);
    }

    if (badge_count > 0) {
        ImFont* bf = fonts::get(fonts::Face::BrutalistXs);
        const float bs = (bf ? bf->LegacySize : scale::dp(10.0f)) * size_mult;
        char buf[8]; std::snprintf(buf, sizeof(buf), "%d", badge_count);
        ImVec2 bsz = bf ? bf->CalcTextSizeA(bs, FLT_MAX, 0.0f, buf) : ImGui::CalcTextSize(buf);
        const float br = scale::dp(10.0f) * size_mult;
        const float bcx = cx + radius * 0.75f;
        const float bcy = cy - radius * 0.75f;
        dl->AddCircleFilled(ImVec2(bcx, bcy), br, col_a(tokens::kErrorRed, alpha_mult), 24);
        dl->AddCircle(ImVec2(bcx, bcy), br, col_a(tokens::kBlack, alpha_mult), 24, scale::dp(1.5f) * size_mult);
        if (bf) dl->AddText(bf, bs,
                            ImVec2(bcx - bsz.x * 0.5f, bcy - bsz.y * 0.5f),
                            col_a(tokens::kBlack, alpha_mult), buf);
    }

    if (value_tag_text && *value_tag_text) {
        ImFont* vf = fonts::get(fonts::Face::MonoXs);
        const float vs = vf ? vf->LegacySize : scale::dp(10.0f);
        ImVec2 ts = vf ? vf->CalcTextSizeA(vs, FLT_MAX, 0.0f, value_tag_text)
                       : ImGui::CalcTextSize(value_tag_text);
        const float vw = ts.x + scale::dp(10.0f);
        const float vh = ts.y + scale::dp(4.0f);
        const ImVec2 vp(cx - vw * 0.5f, cy + radius + scale::dp(4.0f));
        dl->AddRectFilled(vp, ImVec2(vp.x + vw, vp.y + vh), col_a(tokens::kBlack, alpha_mult));
        dl->AddRect(vp, ImVec2(vp.x + vw, vp.y + vh), col_a(tokens::kToxic, alpha_mult), 0.0f, 0, scale::dp(1.0f));
        if (vf) dl->AddText(vf, vs,
                            ImVec2(vp.x + scale::dp(5.0f), vp.y + scale::dp(2.0f)),
                            col_a(tokens::kToxic, alpha_mult), value_tag_text);
    }
}

struct ArcOri { float center; float start; float end; ImVec2 anchor; };

ArcOri compute_arc_ori(const ImVec2& btn_center, const ImVec2& vp_min, const ImVec2& vp_max) {
    const float vw = vp_max.x - vp_min.x;
    const float vh = vp_max.y - vp_min.y;
    const float fx = vw > 0.0f ? (btn_center.x - vp_min.x) / vw : 0.5f;
    const float fy = vh > 0.0f ? (btn_center.y - vp_min.y) / vh : 0.5f;
    const float dx = fx - 0.5f;
    const float dy = fy - 0.5f;
    float c;
    ImVec2 anchor = btn_center;
    const float edge_margin = scale::dp(6.0f);
    if (std::fabs(dx) >= std::fabs(dy)) {
        if (dx > 0.0f) { c = 3.14159265f; anchor.x = vp_max.x - edge_margin; }
        else           { c = 0.0f;        anchor.x = vp_min.x + edge_margin; }
        anchor.y = btn_center.y;
    } else {
        if (dy > 0.0f) { c = -1.5707963f; anchor.y = vp_max.y - edge_margin; }
        else           { c =  1.5707963f; anchor.y = vp_min.y + edge_margin; }
        anchor.x = btn_center.x;
    }
    const float half = 1.5707963f;
    ArcOri o;
    o.center = c;
    o.start = c - half;
    o.end   = c + half;
    o.anchor = anchor;
    return o;
}

void render_arc_engage(ImDrawList* dl, const Feature& f, const ImVec2& btn_center,
                       const ImVec2& vp_min, const ImVec2& vp_max,
                       float current_value, float anim_t)
{
    const ArcOri o = compute_arc_ori(btn_center, vp_min, vp_max);
    const ImVec2 cen = o.anchor;
    const float r_base = scale::dp(96.0f);
    const float stroke = scale::dp(12.0f);

    const float t_ring = fx::anim::ease_out_cubic(anim_t);
    if (t_ring <= 0.001f) return;
    const float r = r_base * t_ring;

    const float span_half = 1.5707963f * t_ring;
    const float arc_start = o.center - span_half;
    const float arc_end   = o.center + span_half;

    const ImU32 black_a = col_a(tokens::kBlack, t_ring);
    const ImU32 toxic_a = col_a(tokens::kToxic, t_ring);

    dl->PathClear();
    dl->PathArcTo(cen, r, arc_start, arc_end, 48);
    dl->PathStroke(black_a, ImDrawFlags_None, stroke + scale::dp(4.0f));

    const int dash_segs = 18;
    for (int i = 0; i < dash_segs; ++i) {
        if ((i & 1) == 0) continue;
        const float t0 = static_cast<float>(i) / dash_segs;
        const float t1 = static_cast<float>(i + 1) / dash_segs;
        const float a0 = arc_start + t0 * (arc_end - arc_start);
        const float a1 = arc_start + t1 * (arc_end - arc_start);
        dl->PathClear();
        dl->PathArcTo(cen, r, a0, a1, 8);
        dl->PathStroke(toxic_a, ImDrawFlags_None, stroke);
    }

    const float pct = (current_value - f.arc_min) / (f.arc_max - f.arc_min);
    const float pct_c = pct < 0.0f ? 0.0f : (pct > 1.0f ? 1.0f : pct);
    const float a_knob = arc_start + pct_c * (arc_end - arc_start);
    const ImVec2 knob(cen.x + std::cos(a_knob) * r,
                      cen.y + std::sin(a_knob) * r);

    dl->PathClear();
    dl->PathArcTo(cen, r, arc_start, a_knob, 32);
    dl->PathStroke(toxic_a, ImDrawFlags_None, stroke + scale::dp(2.0f));

    dl->AddCircleFilled(knob, scale::dp(13.0f) * t_ring, black_a, 32);
    dl->AddCircle(knob, scale::dp(13.0f) * t_ring, toxic_a, 32, scale::dp(3.0f));

    const float tag_alpha = fx::anim::clamp01((anim_t - 0.45f) / 0.55f);
    if (tag_alpha > 0.01f) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.2f%s", current_value, f.arc_fmt ? f.arc_fmt : "");
        ImFont* vf = fonts::get(fonts::Face::BrutalistMd);
        const float vs = vf ? vf->LegacySize : scale::dp(16.0f);
        ImVec2 ts = vf ? vf->CalcTextSizeA(vs, FLT_MAX, 0.0f, buf) : ImGui::CalcTextSize(buf);
        const float tag_r = r_base + scale::dp(34.0f);
        const float tag_a = o.center;
        float tx = cen.x + std::cos(tag_a) * tag_r;
        float ty = cen.y + std::sin(tag_a) * tag_r;
        const float tag_pad = scale::dp(8.0f);
        ImVec2 tmn(tx - ts.x * 0.5f - tag_pad, ty - ts.y * 0.5f - scale::dp(4.0f));
        ImVec2 tmx(tx + ts.x * 0.5f + tag_pad, ty + ts.y * 0.5f + scale::dp(4.0f));
        if (tmn.x < vp_min.x + scale::dp(8.0f)) { const float d = vp_min.x + scale::dp(8.0f) - tmn.x; tmn.x += d; tmx.x += d; }
        if (tmx.x > vp_max.x - scale::dp(8.0f)) { const float d = tmx.x - (vp_max.x - scale::dp(8.0f)); tmn.x -= d; tmx.x -= d; }
        if (tmn.y < vp_min.y + scale::dp(8.0f)) { const float d = vp_min.y + scale::dp(8.0f) - tmn.y; tmn.y += d; tmx.y += d; }
        if (tmx.y > vp_max.y - scale::dp(8.0f)) { const float d = tmx.y - (vp_max.y - scale::dp(8.0f)); tmn.y -= d; tmx.y -= d; }
        dl->AddRectFilled(ImVec2(tmn.x + scale::dp(2.0f), tmn.y + scale::dp(2.0f)),
                          ImVec2(tmx.x + scale::dp(2.0f), tmx.y + scale::dp(2.0f)),
                          col_a(tokens::kBlack, tag_alpha));
        dl->AddRectFilled(tmn, tmx, col_a(tokens::kToxic, tag_alpha));
        dl->AddRect(tmn, tmx, col_a(tokens::kBlack, tag_alpha), 0.0f, 0, scale::dp(tokens::kBorderThin));
        if (vf) dl->AddText(vf, vs, ImVec2(tmn.x + tag_pad, tmn.y + scale::dp(4.0f)),
                            col_a(tokens::kBlack, tag_alpha), buf);
    }
}

int pick_arc_value(const Feature& f, const ImVec2& btn_center, const ImVec2& mouse,
                   const ImVec2& vp_min, const ImVec2& vp_max, float& out_value)
{
    const ArcOri o = compute_arc_ori(btn_center, vp_min, vp_max);
    const float two_pi = 6.2831853f;
    float a = std::atan2(mouse.y - o.anchor.y, mouse.x - o.anchor.x);
    float delta = a - o.center;
    while (delta >  3.14159265f) delta -= two_pi;
    while (delta < -3.14159265f) delta += two_pi;
    const float half = 1.5707963f;
    if (delta >  half) delta =  half;
    if (delta < -half) delta = -half;
    const float pct = (delta + half) / (2.0f * half);
    float v = f.arc_min + pct * (f.arc_max - f.arc_min);
    if (f.arc_step > 0.0f) {
        v = std::round(v / f.arc_step) * f.arc_step;
    }
    if (v < f.arc_min) v = f.arc_min;
    if (v > f.arc_max) v = f.arc_max;
    out_value = v;
    return 1;
}

ImVec2 compute_pie_center(const ImVec2& btn_center, float r_outer,
                          const ImVec2& vp_min, const ImVec2& vp_max)
{
    const float label_ext = scale::dp(22.0f);
    const float eff = r_outer + label_ext;
    float cx = btn_center.x;
    float cy = btn_center.y;
    if (cx < vp_min.x + eff) cx = vp_min.x + eff;
    if (cx > vp_max.x - eff) cx = vp_max.x - eff;
    if (cy < vp_min.y + eff) cy = vp_min.y + eff;
    if (cy > vp_max.y - eff) cy = vp_max.y - eff;
    return ImVec2(cx, cy);
}

int pick_pie_segment(const Feature& f, const ImVec2& pie_center, const ImVec2& mouse,
                     float r_inner, float r_outer)
{
    const float dx = mouse.x - pie_center.x;
    const float dy = mouse.y - pie_center.y;
    const float d  = std::sqrt(dx * dx + dy * dy);
    if (d < r_inner * 0.55f) return -1;
    (void)r_outer;
    const int n = f.option_count;
    const float step = 6.2831853f / n;
    const float offset = -1.5707963f - step * 0.5f;
    float a = std::atan2(dy, dx) - offset;
    while (a < 0.0f) a += 6.2831853f;
    int idx = static_cast<int>(std::fmod(a, 6.2831853f) / step);
    if (idx < 0) idx = 0;
    if (idx >= n) idx = n - 1;
    return idx;
}

void render_pie_engage(ImDrawList* dl, const Feature& f, const ImVec2& pie_center,
                       int hover_idx, bool is_multi, float anim_t)
{
    const float r_inner = scale::dp(44.0f);
    const float r_outer = scale::dp(104.0f);
    const int n = f.option_count;
    const float step = 6.2831853f / n;
    const float offset = -1.5707963f - step * 0.5f;

    const int arc_subdiv = 48;
    const int circle_segs = 96;
    const float sep_th    = scale::dp(2.0f);
    const float border_th = scale::dp(3.0f);

    const ImDrawListFlags prev_flags = dl->Flags;
    dl->Flags = prev_flags & ~ImDrawListFlags_AntiAliasedFill;

    const float stagger = 0.1f;
    const float per_seg_span = 1.0f - stagger * static_cast<float>(n - 1);

    const float bg_alpha = fx::anim::clamp01(anim_t / 0.25f);
    if (bg_alpha > 0.01f) {
        dl->PathClear();
        dl->PathArcTo(pie_center, r_outer, 0.0f, 6.2831853f, circle_segs);
        dl->PathFillConvex(col_a(tokens::kCharcoal, bg_alpha));
    }

    for (int i = 0; i < n; ++i) {
        const float a1 = offset + step * i;
        const float a2 = a1 + step;
        const bool on = is_multi ? state::get_bool(f.options[i].key, false)
                                 : (state::get_int(f.id, 0) == f.options[i].value);
        const bool hov = (i == hover_idx);

        const float seg_t_raw = (anim_t - stagger * static_cast<float>(i)) / per_seg_span;
        float seg_t = seg_t_raw < 0.0f ? 0.0f : (seg_t_raw > 1.0f ? 1.0f : seg_t_raw);
        seg_t = fx::anim::ease_out_cubic(seg_t);
        if (seg_t <= 0.001f) continue;

        ImU32 fill;
        if (on && hov)        fill = tokens::kHazardOrange;
        else if (on)          fill = tokens::kToxic;
        else if (hov)         fill = tokens::kToxic;
        else                  fill = tokens::kCharcoal;

        const float a2_cur = a1 + seg_t * (a2 - a1);
        const int subdiv = std::max(2, static_cast<int>(arc_subdiv * seg_t) + 1);
        for (int k = 0; k < subdiv; ++k) {
            const float t0 = static_cast<float>(k)     / subdiv;
            const float t1 = static_cast<float>(k + 1) / subdiv;
            const float ak0 = a1 + t0 * (a2_cur - a1);
            const float ak1 = a1 + t1 * (a2_cur - a1);
            const float c0 = std::cos(ak0), s0 = std::sin(ak0);
            const float c1 = std::cos(ak1), s1 = std::sin(ak1);
            ImVec2 quad[4] = {
                ImVec2(pie_center.x + c0 * r_outer, pie_center.y + s0 * r_outer),
                ImVec2(pie_center.x + c1 * r_outer, pie_center.y + s1 * r_outer),
                ImVec2(pie_center.x + c1 * r_inner, pie_center.y + s1 * r_inner),
                ImVec2(pie_center.x + c0 * r_inner, pie_center.y + s0 * r_inner),
            };
            dl->AddConvexPolyFilled(quad, 4, fill);
        }

        const float mid_a = (a1 + a2) * 0.5f;
        const float mid_r = (r_inner + r_outer) * 0.5f;
        const ImVec2 lp(pie_center.x + std::cos(mid_a) * mid_r,
                        pie_center.y + std::sin(mid_a) * mid_r);

        const float text_alpha = fx::anim::clamp01((seg_t - 0.6f) / 0.4f);

        ImFont* icf = fonts::get(fonts::Face::IconLg);
        if (icf && f.options[i].icon_utf8 && text_alpha > 0.01f) {
            const float isz = icf->LegacySize * 0.95f;
            ImVec2 esz = icf->CalcTextSizeA(isz, FLT_MAX, 0.0f, f.options[i].icon_utf8);
            const ImU32 icol_base = (on || hov) ? tokens::kBlack : tokens::kToxic;
            const ImU32 icol = col_a(icol_base, text_alpha);
            dl->AddText(icf, isz,
                        ImVec2(lp.x - esz.x * 0.5f, lp.y - esz.y * 0.5f - scale::dp(6.0f)),
                        icol, f.options[i].icon_utf8);
        }
        ImFont* lf = fonts::get(fonts::Face::BrutalistXs);
        if (lf && f.options[i].label && text_alpha > 0.01f) {
            const char* label_tr = ui::i18n::t(f.options[i].label);
            const float ls = lf->LegacySize;
            ImVec2 lsz = lf->CalcTextSizeA(ls, FLT_MAX, 0.0f, label_tr);
            const ImU32 lcol_base = (on || hov) ? tokens::kBlack : tokens::kToxic;
            const ImU32 lcol = col_a(lcol_base, text_alpha);
            dl->AddText(lf, ls,
                        ImVec2(lp.x - lsz.x * 0.5f, lp.y + scale::dp(6.0f)),
                        lcol, label_tr);
        }
    }

    dl->Flags = prev_flags;

    if (bg_alpha > 0.01f) {
        const ImU32 border_col = col_a(tokens::kBlack, bg_alpha);
        dl->AddCircle(pie_center, r_outer, border_col, circle_segs, border_th);
        dl->AddCircle(pie_center, r_inner, border_col, circle_segs, border_th);
    }

    for (int i = 0; i < n; ++i) {
        const float a = offset + step * i;
        const float sep_t_raw = (anim_t - stagger * static_cast<float>(i)) / per_seg_span;
        float sep_t = sep_t_raw < 0.0f ? 0.0f : (sep_t_raw > 1.0f ? 1.0f : sep_t_raw);
        if (sep_t <= 0.01f) continue;
        const ImVec2 p_in(pie_center.x + std::cos(a) * r_inner,
                          pie_center.y + std::sin(a) * r_inner);
        const ImVec2 p_out(pie_center.x + std::cos(a) * r_outer,
                           pie_center.y + std::sin(a) * r_outer);
        dl->AddLine(p_in, p_out, col_a(tokens::kBlack, sep_t), sep_th);
    }
}

float compute_pulse() {
    const double now = ImGui::GetTime();
    const float t = static_cast<float>(std::fmod(now, 1.6) * (1.0 / 1.6));
    const float s = std::sin(t * 6.2831853f);
    return 0.5f + 0.5f * s;
}

}

void render(State& s,
            fx::glitch::State* glitch,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt)
{
    if (!state::get_bool("fqa.enabled", true)) {
        s.engaged_idx = -1;
        s.engaged_segment = -1;
        s.engaged_anim = 0.0f;
        s.hub_expanded = false;
        s.hub_timer = 0.0f;
        return;
    }

    {
        float op = state::get_float("hud.opacity", 1.0f);
        if (!(op > 0.0f)) op = 0.0f;
        if (op > 1.0f) op = 1.0f;
        g_hud_opacity.store(op);
    }

    const bool horiz = state::get_int("fqa.orientation", 0) == 1;
    const bool collapse_wanted = state::get_bool("fqa.collapse", false);

    const int total = static_cast<int>(sizeof(kFeatures) / sizeof(kFeatures[0]));
    int visible_ids[16];
    int visible_n = 0;
    for (int i = 0; i < total; ++i) {
        if (is_feature_active(kFeatures[i])) visible_ids[visible_n++] = i;
    }
    if (visible_n == 0) return;

    const bool hub_mode = collapse_wanted && visible_n >= 4 && !s.hub_expanded;
    if (s.hub_expanded) {
        s.hub_timer -= dt;
        if (s.hub_timer <= 0.0f) { s.hub_expanded = false; s.hub_timer = 0.0f; }
    }

    float size_mult = state::get_float("fqa.size_mult", 0.7f);
    if (size_mult < 0.4f) size_mult = 0.4f;
    if (size_mult > 1.6f) size_mult = 1.6f;
    const float btn_size = scale::dp(tokens::kFqaButtonSize) * size_mult;
    const float gap = scale::dp(10.0f) * size_mult;
    const int render_count = hub_mode ? 1 : visible_n;

    const float dock_w = horiz ? (render_count * btn_size + (render_count - 1) * gap) : btn_size;
    const float dock_h = horiz ? btn_size : (render_count * btn_size + (render_count - 1) * gap);

    const float vw = viewport_max.x - viewport_min.x;
    const float vh = viewport_max.y - viewport_min.y;

    const float anchor_x_norm = state::get_float("fqa.anchor_x", 1.0f);
    const float user_y_norm = state::get_float("fqa.dock_y_norm", 0.5f);

    const float margin = scale::dp(12.0f);
    const float dock_x = anchor_x_norm >= 0.5f
                             ? (viewport_max.x - dock_w - margin)
                             : (viewport_min.x + margin);

    float dock_y_center = viewport_min.y + vh * user_y_norm;
    float dock_y = dock_y_center - dock_h * 0.5f;
    const float min_y = viewport_min.y + scale::dp(72.0f);
    const float max_y = viewport_max.y - dock_h - scale::dp(32.0f);
    if (dock_y < min_y) dock_y = min_y;
    if (dock_y > max_y) dock_y = max_y;

    const float opacity = state::get_float("fqa.opacity", 1.0f);
    const float alpha_master = opacity < 0.0f ? 0.0f : (opacity > 1.0f ? 1.0f : opacity);
    (void)alpha_master;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const float pulse = compute_pulse();

    if (hub_mode) {
        const float x = dock_x;
        const float y = dock_y;
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::InvisibleButton("##fqa_hub", ImVec2(btn_size, btn_size));
        const bool hov = ImGui::IsItemHovered();
        const bool act = ImGui::IsItemActive();
        if (ImGui::IsItemClicked()) {
            s.hub_expanded = true;
            s.hub_timer = 4.5f;
        }
        draw_button(dl, ImVec2(x, y), btn_size, icons::k_apps,
                    true, hov, act, false, pulse, 0.0f, visible_n, nullptr);
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse = io.MousePos;

    if (s.engaged_idx >= 0 && s.engaged_idx < total) {
        const Feature& ef = kFeatures[s.engaged_idx];
        if (ef.type == FType::Arc) {
            float v = 0.0f;
            pick_arc_value(ef, s.engaged_center, mouse, viewport_min, viewport_max, v);
            s.engaged_value = v;
            s.engaged_had_move = true;
        } else if (ef.type == FType::PieSingle || ef.type == FType::PieMulti) {
            const ImVec2 pie_c = compute_pie_center(s.engaged_center, scale::dp(104.0f),
                                                    viewport_min, viewport_max);
            const int seg = pick_pie_segment(ef, pie_c, mouse,
                                             scale::dp(44.0f), scale::dp(104.0f));
            s.engaged_segment = seg;
            if (seg >= 0) s.engaged_had_move = true;
        }

        if (!ImGui::IsMouseDown(0)) {
            if (ef.type == FType::Arc) {
                state::set_float(ef.id, s.engaged_value);
                if (glitch) {
                    char msg[96]; std::snprintf(msg, sizeof(msg), "%s %.2f%s",
                                                ui::i18n::t(ef.label),
                                                s.engaged_value,
                                                ef.arc_fmt ? ef.arc_fmt : "");
                    fx::glitch::trigger(*glitch, ef.icon_utf8, msg);
                }
            } else if (ef.type == FType::PieSingle && s.engaged_segment >= 0) {
                const int v = ef.options[s.engaged_segment].value;
                state::set_int(ef.id, v);
                if (glitch) {
                    char msg[96]; std::snprintf(msg, sizeof(msg), "%s %s",
                                                ui::i18n::t(ef.label),
                                                ui::i18n::t(ef.options[s.engaged_segment].label));
                    fx::glitch::trigger(*glitch, ef.options[s.engaged_segment].icon_utf8, msg);
                }
            } else if (ef.type == FType::PieMulti && s.engaged_segment >= 0) {
                const char* k = ef.options[s.engaged_segment].key;
                const bool nv = !state::get_bool(k, false);
                state::set_bool(k, nv);
                if (glitch) {
                    char msg[96]; std::snprintf(msg, sizeof(msg), "%s %s",
                                                ui::i18n::t(ef.options[s.engaged_segment].label),
                                                ui::i18n::t(nv ? "ON" : "OFF"));
                    fx::glitch::trigger(*glitch, ef.options[s.engaged_segment].icon_utf8, msg);
                }
            }
            s.engaged_idx = -1;
            s.engaged_segment = -1;
            s.engaged_had_move = false;
        }
    }

    const float target_eng = s.engaged_idx >= 0 ? 1.0f : 0.0f;
    s.engaged_anim = fx::anim::spring_to(s.engaged_anim, target_eng, dt, 5.0f);

    bool     defer_btn_valid = false;
    ImVec2   defer_btn_pos(0, 0);
    const char* defer_btn_icon = nullptr;
    bool     defer_btn_on = false;
    bool     defer_btn_hov = false;
    bool     defer_btn_act = false;
    int      defer_btn_badge = 0;

    for (int i = 0; i < visible_n; ++i) {
        const Feature& f = kFeatures[visible_ids[i]];
        const float x = horiz ? (dock_x + i * (btn_size + gap)) : dock_x;
        const float y = horiz ? dock_y : (dock_y + i * (btn_size + gap));

        char id[64]; std::snprintf(id, sizeof(id), "##fqa_%s", f.id);
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::InvisibleButton(id, ImVec2(btn_size, btn_size));
        const bool hov = ImGui::IsItemHovered();
        const bool act = ImGui::IsItemActive();
        const bool clk = ImGui::IsItemClicked();
        const bool activated = ImGui::IsItemActivated();

        if (activated && s.engaged_idx < 0) {
            if (f.type == FType::Arc) {
                s.engaged_idx = visible_ids[i];
                s.engaged_center = ImVec2(x + btn_size * 0.5f, y + btn_size * 0.5f);
                s.engaged_value = state::get_float(f.id, 1.0f);
                s.engaged_had_move = false;
            } else if (f.type == FType::PieSingle || f.type == FType::PieMulti) {
                s.engaged_idx = visible_ids[i];
                s.engaged_center = ImVec2(x + btn_size * 0.5f, y + btn_size * 0.5f);
                s.engaged_segment = -1;
                s.engaged_had_move = false;
            }
        }

        const bool this_engaged = (s.engaged_idx == visible_ids[i]);
        const float other_eng = (s.engaged_idx >= 0 && !this_engaged) ? s.engaged_anim : 0.0f;

        const bool on = is_feature_on(f);
        const int badge = (f.type == FType::PieMulti) ? count_power_active() : 0;

        ImVec2 draw_pos(x, y);
        if (this_engaged && (f.type == FType::PieSingle || f.type == FType::PieMulti)) {
            const float eased = fx::anim::ease_out_cubic(s.engaged_anim);
            const ImVec2 target_c = compute_pie_center(s.engaged_center, scale::dp(104.0f),
                                                       viewport_min, viewport_max);
            const ImVec2 btn_c(x + btn_size * 0.5f, y + btn_size * 0.5f);
            const float cx = btn_c.x + (target_c.x - btn_c.x) * eased;
            const float cy = btn_c.y + (target_c.y - btn_c.y) * eased;
            draw_pos = ImVec2(cx - btn_size * 0.5f, cy - btn_size * 0.5f);

            defer_btn_valid = true;
            defer_btn_pos   = draw_pos;
            defer_btn_icon  = f.icon_utf8;
            defer_btn_on    = on;
            defer_btn_hov   = hov;
            defer_btn_act   = act;
            defer_btn_badge = badge;
        } else {
            draw_button(dl, draw_pos, btn_size,
                        f.icon_utf8, on, hov, act,
                        this_engaged,
                        pulse, other_eng, badge, nullptr);
        }

        if (clk && glitch && s.engaged_idx < 0) {
            if (f.type == FType::Toggle) {
                const bool new_v = !on;
                state::set_bool(f.id, new_v);
                char msg[128];
                std::snprintf(msg, sizeof(msg), "%s %s",
                              ui::i18n::t(f.label), ui::i18n::t(new_v ? "ON" : "OFF"));
                fx::glitch::trigger(*glitch, f.icon_utf8, msg);
            } else if (f.type == FType::Action) {
                fx::glitch::trigger(*glitch, f.icon_utf8, ui::i18n::t(f.label));
            }
        }
    }

    if (s.engaged_idx >= 0 && s.engaged_idx < total) {
        const Feature& ef = kFeatures[s.engaged_idx];
        if (ef.type == FType::Arc) {
            render_arc_engage(dl, ef, s.engaged_center, viewport_min, viewport_max,
                              s.engaged_value, s.engaged_anim);
        } else if (ef.type == FType::PieSingle || ef.type == FType::PieMulti) {
            const ImVec2 pie_c = compute_pie_center(s.engaged_center, scale::dp(104.0f),
                                                    viewport_min, viewport_max);
            render_pie_engage(dl, ef, pie_c, s.engaged_segment,
                              ef.type == FType::PieMulti, s.engaged_anim);
        }
    }

    if (defer_btn_valid) {
        draw_button(dl, defer_btn_pos, btn_size,
                    defer_btn_icon, defer_btn_on, defer_btn_hov, defer_btn_act,
                    true, pulse, 0.0f, defer_btn_badge, nullptr);
    }
}

}
