#include "hero_card.hpp"
#include "primitives.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../i18n/lang.hpp"

#include <cmath>

namespace ui::widgets::hero_card {

namespace {

void draw_col(ImDrawList* dl, float x_left, float y_top, float col_w, float body_h,
              const char* label, const char* value, bool big)
{
    ImFont* lf = fonts::get(fonts::Face::MonoXs);
    ImFont* vf = big ? fonts::get(fonts::Face::HeroLg) : fonts::get(fonts::Face::HeroMd);
    const float ls = lf ? lf->LegacySize : scale::dp(11.0f);
    float vs = vf ? vf->LegacySize : scale::dp(28.0f);

    const float sep_w = scale::dp(4.0f);
    const float pad_l = scale::dp(8.0f);

    dl->AddRectFilled(ImVec2(x_left, y_top),
                      ImVec2(x_left + sep_w, y_top + body_h),
                      tokens::kBlack);

    if (lf) dl->AddText(lf, ls, ImVec2(x_left + sep_w + pad_l, y_top), tokens::kBlack, ui::i18n::t(label));

    if (vf) {
        const float avail_w = col_w - sep_w - pad_l * 2.0f;
        ImVec2 sz = vf->CalcTextSizeA(vs, FLT_MAX, 0.0f, value);
        if (sz.x > avail_w && sz.x > 0.0f) {
            const float min_vs = scale::dp(14.0f);
            vs = vs * (avail_w / sz.x);
            if (vs < min_vs) vs = min_vs;
        }
        const float vy = y_top + ls + scale::dp(4.0f);
        dl->AddText(vf, vs, ImVec2(x_left + sep_w + pad_l, vy), tokens::kBlack, value);
    }
}

}

float render(const ImVec2& pos, float width,
             const char* title, const char* status,
             const char* a_label, const char* a_value,
             const char* b_label, const char* b_value,
             const char* c_label, const char* c_value)
{
    const float pad = scale::dp(14.0f);
    ImFont* header_f = fonts::get(fonts::Face::BrutalistSm);
    ImFont* status_f = fonts::get(fonts::Face::MonoXs);
    ImFont* hero_f   = fonts::get(fonts::Face::HeroLg);
    const float hs = header_f ? header_f->LegacySize : scale::dp(12.0f);
    const float ss = status_f ? status_f->LegacySize : scale::dp(9.0f);
    const float vs = hero_f ? hero_f->LegacySize : scale::dp(36.0f);
    const float header_h = (hs > ss ? hs : ss);
    const float body_h   = scale::dp(9.0f) + scale::dp(4.0f) + vs;
    const float h = pad * 2.0f + header_h + scale::dp(10.0f) + body_h;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    {
        const ImVec2 cmin = dl->GetClipRectMin();
        const ImVec2 cmax = dl->GetClipRectMax();
        if (pos.y + h < cmin.y - 32.0f || pos.y > cmax.y + 32.0f) return h;
    }

    const ImVec2 p0(pos.x, pos.y);
    const ImVec2 p1(pos.x + width, pos.y + h);
    prim::asymmetric_clip(dl, p0, p1,
                          tokens::kToxic, tokens::kBlack, tokens::kBlack,
                          scale::dp(tokens::kBorderThick),
                          scale::dp(tokens::kShadowOffsetThick),
                          0.05f, 0.15f);

    if (header_f) dl->AddText(header_f, hs,
                              ImVec2(pos.x + pad, pos.y + pad),
                              tokens::kBlack, ui::i18n::t(title));
    if (status_f) {
        ImVec2 stsz = status_f->CalcTextSizeA(ss, FLT_MAX, 0.0f, status);
        const float pulse_t = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 3.2f);
        const int alpha = 140 + static_cast<int>(115 * pulse_t);
        const ImU32 pcol = IM_COL32(0x00, 0x00, 0x00, alpha);
        const float dot_r = scale::dp(3.5f);
        const float sx = pos.x + width - pad - stsz.x;
        const float sy = pos.y + pad + (hs - ss) * 0.5f;
        const float dot_gap = scale::dp(6.0f);
        dl->AddCircleFilled(ImVec2(sx - dot_gap - dot_r, sy + stsz.y * 0.5f), dot_r, pcol);
        dl->AddText(status_f, ss, ImVec2(sx, sy), pcol, status);
    }

    const float col_gap = scale::dp(6.0f);
    const float cols_x0 = pos.x + pad;
    const float cols_w  = width - pad * 2.0f;
    const float col_w   = (cols_w - col_gap * 2.0f) / 3.0f;
    const float cols_y  = pos.y + pad + header_h + scale::dp(10.0f);

    draw_col(dl, cols_x0,                               cols_y, col_w, body_h, a_label, a_value, true);
    draw_col(dl, cols_x0 + (col_w + col_gap) * 1.0f,    cols_y, col_w, body_h, b_label, b_value, true);
    draw_col(dl, cols_x0 + (col_w + col_gap) * 2.0f,    cols_y, col_w, body_h, c_label, c_value, false);

    return h;
}

}
