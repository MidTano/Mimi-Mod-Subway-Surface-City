#include "tab_bar.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../fx/animation.hpp"
#include "../i18n/lang.hpp"

#include <cstdio>

namespace ui::layout::tab_bar {

const char* tab_id(Tab t) {
    switch (t) {
        case Tab::Run: return "run";
        case Tab::Visuals: return "visuals";
        case Tab::Gameplay: return "gameplay";
        case Tab::PowerUps: return "powerups";
        case Tab::Cosmetics: return "cosmetics";
        default: return "";
    }
}
const char* tab_label(Tab t) {
    switch (t) {
        case Tab::Run: return "MAIN";
        case Tab::Visuals: return "VISUALS";
        case Tab::Gameplay: return "GAMEPLAY";
        case Tab::PowerUps: return "POWER";
        case Tab::Cosmetics: return "SKINS";
        default: return "";
    }
}

namespace {

const char* tab_glyph(Tab t) {
    switch (t) {
        case Tab::Run:       return icons::k_directions_run;
        case Tab::Visuals:   return icons::k_visibility;
        case Tab::Gameplay:  return icons::k_speed;
        case Tab::PowerUps:  return icons::k_bolt;
        case Tab::Cosmetics: return icons::k_category;
        default:             return icons::k_bolt;
    }
}

void draw_tab(ImDrawList* dl,
              const ImVec2& min, const ImVec2& max,
              Tab t, bool active, float anim_phase, bool hovered, bool pressed)
{
    const float h = max.y - min.y;
    const float w = max.x - min.x;

    if (active) {
        dl->AddRectFilled(min, max, tokens::kToxic);
        const float flag_h = scale::dp(6.0f);
        const float flag_phase = fx::anim::clamp01(anim_phase * 1.4f);
        dl->AddRectFilled(ImVec2(min.x - 1, min.y - flag_h * flag_phase),
                          ImVec2(max.x + 1, min.y),
                          tokens::kBlack);
        const float chev_phase = fx::anim::ease_out_back(anim_phase);
        const float ch_w = scale::dp(8.0f);
        const float ch_h = scale::dp(8.0f);
        const ImVec2 cb_center((min.x + max.x) * 0.5f, max.y - 1.0f + ch_h * (1.0f - chev_phase));
        const ImVec2 p1(cb_center.x - ch_w, max.y - 1.0f);
        const ImVec2 p2(cb_center.x + ch_w, max.y - 1.0f);
        const ImVec2 p3(cb_center.x,        max.y - 1.0f + ch_h);
        ImVec2 chev[3] = {p1, p2, p3};
        dl->AddConvexPolyFilled(chev, 3, tokens::kToxic);
        dl->AddPolyline(chev, 3, tokens::kBlack, ImDrawFlags_Closed, scale::dp(1.5f));
    } else {
        if (hovered || pressed) {
            dl->AddRectFilled(min, max, IM_COL32(0xCC, 0xFF, 0x00, 0x14));
        }
    }

    if (t != Tab::Cosmetics) {
        dl->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, max.y),
                    IM_COL32(0xCC, 0xFF, 0x00, 0x33), scale::dp(2.0f));
    }

    const ImU32 fg = active ? tokens::kBlack : tokens::kOffWhiteDim;

    const float translate_y = active ? -scale::dp(3.0f) * fx::anim::ease_out_back(anim_phase) : 0.0f;

    ImFont* icon = fonts::get(fonts::Face::IconLg);
    ImFont* lbl  = fonts::get(fonts::Face::BrutalistXs);
    const char* glyph = tab_glyph(t);
    if (icon) {
        float ifs = icon->LegacySize;
        if (active) {
            ifs *= 1.0f + fx::anim::ease_out_back(anim_phase) * 0.12f;
        }
        ImVec2 isz = icon->CalcTextSizeA(ifs, FLT_MAX, 0.0f, glyph);
        const ImVec2 ip(min.x + (w - isz.x) * 0.5f,
                        min.y + h * 0.18f + translate_y);
        dl->AddText(icon, ifs, ip, fg, glyph);
    }
    if (lbl) {
        const char* lbl_tr = ui::i18n::t(tab_label(t));
        const float lfs = lbl->LegacySize;
        ImVec2 lsz = lbl->CalcTextSizeA(lfs, FLT_MAX, 0.0f, lbl_tr);
        const ImVec2 lp(min.x + (w - lsz.x) * 0.5f,
                        max.y - lsz.y - h * 0.12f + translate_y);
        dl->AddText(lbl, lfs, lp, fg, lbl_tr);
    }
}

}

void render(State& s, const ImVec2& min, const ImVec2& max, float dt) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(min, max, tokens::kBlack);
    dl->AddLine(ImVec2(min.x, min.y), ImVec2(max.x, min.y),
                tokens::kToxic, scale::dp(4.0f));

    const int n = static_cast<int>(Tab::Count);
    const float w = (max.x - min.x) / n;

    s.anim_phase = fx::anim::advance_phase(s.anim_phase, dt, tokens::kAnimSlow);

    for (int i = 0; i < n; ++i) {
        const ImVec2 tmin(min.x + w * i, min.y);
        const ImVec2 tmax(min.x + w * (i + 1), max.y);

        char id[16]; std::snprintf(id, sizeof(id), "##tab_%d", i);
        ImGui::SetCursorScreenPos(tmin);
        ImGui::InvisibleButton(id, ImVec2(w, max.y - min.y));
        const bool hov = ImGui::IsItemHovered();
        const bool act = ImGui::IsItemActive();
        const bool clk = ImGui::IsItemClicked();

        const Tab t = static_cast<Tab>(i);
        const bool active = (t == s.active);

        draw_tab(dl, tmin, tmax, t, active, s.anim_phase, hov, act);

        if (clk && t != s.active) {
            s.last_active = s.active;
            s.active = t;
            s.anim_phase = 0.0f;
        }
    }
}

}
