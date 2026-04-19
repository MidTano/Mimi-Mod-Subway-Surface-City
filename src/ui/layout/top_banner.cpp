#include "top_banner.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../fx/ticker.hpp"
#include "../i18n/lang.hpp"
#include "../widgets/primitives.hpp"

#include <cmath>

namespace ui::layout::top_banner {

namespace {

std::vector<fx::ticker::TickerSegment> ticker_segments() {
    std::vector<fx::ticker::TickerSegment> segs;
    const char* const kKeys[] = {
        "TICKER_3",
        "TICKER_4",
        "TICKER_5",
    };
    for (const char* k : kKeys) {
        const char* tr = ui::i18n::t(k);
        if (tr && *tr) {
            segs.push_back({std::string(tr), false});
            segs.push_back({std::string(), true});
        }
    }
    return segs;
}

}

bool render(State& s, const ImVec2& bmin, const ImVec2& bmax, float dt) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float hazard_top_h = scale::dp(tokens::kHazardHeight);
    const float ticker_h     = scale::dp(tokens::kBannerHeight);
    const float hazard_bot_h = scale::dp(tokens::kHazardThin);
    const float stripe_w     = scale::dp(18.0f);

    const ImVec2 hazard_top_min = bmin;
    const ImVec2 hazard_top_max(bmax.x, bmin.y + hazard_top_h);
    fx::ticker::render_hazard(dl, hazard_top_min, hazard_top_max,
                              false, scale::dp(40.0f), dt, s.hazard_top_offset,
                              tokens::kHazardOrange, tokens::kBlack, stripe_w);
    dl->AddLine(ImVec2(hazard_top_min.x, hazard_top_max.y - 1),
                ImVec2(hazard_top_max.x, hazard_top_max.y - 1), tokens::kBlack, scale::dp(2.0f));

    const ImVec2 ticker_min(bmin.x, hazard_top_max.y);
    const ImVec2 ticker_max(bmax.x, ticker_min.y + ticker_h);
    dl->AddRectFilled(ticker_min, ticker_max, tokens::kBlack);
    dl->AddLine(ImVec2(ticker_min.x, ticker_min.y),
                ImVec2(ticker_max.x, ticker_min.y), tokens::kToxic, scale::dp(2.0f));
    dl->AddLine(ImVec2(ticker_min.x, ticker_max.y - 1),
                ImVec2(ticker_max.x, ticker_max.y - 1), tokens::kToxic, scale::dp(2.0f));

    ImFont* brut = fonts::get(fonts::Face::BrutalistSm);
    ImFont* mono = fonts::get(fonts::Face::MonoSm);

    const char* badge_text = "MIMI_MOD";
    const float badge_pad_x = scale::dp(10.0f);
    const float badge_clip  = scale::dp(10.0f);
    ImFont* icon_md = fonts::get(fonts::Face::IconMd);
    const float icon_sz = icon_md ? icon_md->LegacySize : scale::dp(18.0f);
    const float icon_gap = scale::dp(4.0f);
    ImVec2 badge_text_sz = brut ? brut->CalcTextSizeA(brut->LegacySize, FLT_MAX, 0.0f, badge_text)
                                : ImGui::CalcTextSize(badge_text);
    const float badge_w = icon_sz + icon_gap + badge_text_sz.x + badge_pad_x * 2.0f + badge_clip;
    const ImVec2 badge_min = ticker_min;
    const ImVec2 badge_max(badge_min.x + badge_w, ticker_max.y);

    {
        const ImVec2 p1(badge_min.x,                badge_min.y);
        const ImVec2 p2(badge_max.x,                badge_min.y);
        const ImVec2 p3(badge_max.x - badge_clip,   badge_max.y);
        const ImVec2 p4(badge_min.x,                badge_max.y);
        ImVec2 sh[4] = {
            ImVec2(p1.x + 3, p1.y), ImVec2(p2.x + 3, p2.y),
            ImVec2(p3.x + 3, p3.y), ImVec2(p4.x + 3, p4.y),
        };
        dl->AddConvexPolyFilled(sh, 4, tokens::kBlack);
        ImVec2 face[4] = {p1, p2, p3, p4};
        dl->AddConvexPolyFilled(face, 4, tokens::kToxic);
    }
    float bx = badge_min.x + badge_pad_x;
    if (icon_md) {
        const ImVec2 ip(bx, badge_min.y + (ticker_h - icon_sz) * 0.5f);
        dl->AddText(icon_md, icon_sz, ip, tokens::kBlack, icons::k_bolt);
        bx += icon_sz + icon_gap;
    }
    if (brut) {
        const ImVec2 tp(bx, badge_min.y + (ticker_h - badge_text_sz.y) * 0.5f);
        dl->AddText(brut, brut->LegacySize, tp, tokens::kBlack, badge_text);
    }

    const float close_w = scale::dp(38.0f);
    const ImVec2 close_min(ticker_max.x - close_w, ticker_min.y);
    const ImVec2 close_max(ticker_max.x, ticker_max.y);

    ImGui::SetCursorScreenPos(close_min);
    ImGui::InvisibleButton("##banner_close", ImVec2(close_w, ticker_h));
    const bool close_active = ImGui::IsItemActive();
    const bool close_clicked = ImGui::IsItemClicked();

    dl->AddRectFilled(close_min, close_max, tokens::kToxic);
    dl->AddLine(close_min, ImVec2(close_min.x, close_max.y), tokens::kBlack, scale::dp(2.0f));
    if (close_active) {
        dl->AddRectFilled(close_min, close_max, IM_COL32(0xB8, 0xE5, 0x00, 0xFF));
    }

    {
        const ImU32 col = tokens::kBlack;
        if (icon_md) {
            ImVec2 isz = icon_md->CalcTextSizeA(icon_sz, FLT_MAX, 0.0f, icons::k_close);
            const ImVec2 ip(close_min.x + (close_w - isz.x) * 0.5f,
                            close_min.y + (ticker_h - isz.y) * 0.5f);
            dl->AddText(icon_md, icon_sz, ip, col, icons::k_close);
        }
    }

    const ImVec2 ttick_min(badge_max.x + scale::dp(6.0f), ticker_min.y);
    const ImVec2 ttick_max(close_min.x - scale::dp(4.0f), ticker_max.y);
    if (ttick_max.x > ttick_min.x) {
        fx::ticker::render_marquee(dl, ttick_min, ttick_max, ticker_segments(),
                                   scale::dp(46.0f), dt, s.ticker_state,
                                   tokens::kToxic, tokens::kToxicDim, mono, scale::dp(18.0f));
    }

    const ImVec2 hazard_bot_min(bmin.x, ticker_max.y);
    const ImVec2 hazard_bot_max(bmax.x, ticker_max.y + hazard_bot_h);
    dl->AddLine(ImVec2(hazard_bot_min.x, hazard_bot_min.y),
                ImVec2(hazard_bot_max.x, hazard_bot_min.y), tokens::kBlack, scale::dp(2.0f));
    fx::ticker::render_hazard(dl, hazard_bot_min, hazard_bot_max,
                              true, scale::dp(28.0f), dt, s.hazard_bot_offset,
                              tokens::kHazardOrange, tokens::kBlack, stripe_w);

    return close_clicked;
}

}
