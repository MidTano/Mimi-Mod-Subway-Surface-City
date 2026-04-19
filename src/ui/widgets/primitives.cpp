#include "primitives.hpp"
#include "../tokens.hpp"

#include <cmath>

namespace ui::widgets::prim {

void brutalist_border(ImDrawList* dl,
                      const ImVec2& min,
                      const ImVec2& max,
                      ImU32 fill,
                      ImU32 border,
                      ImU32 shadow,
                      float thick,
                      float shadow_off)
{
    if (shadow_off > 0.0f && (shadow & IM_COL32_A_MASK)) {
        dl->AddRectFilled(ImVec2(min.x + shadow_off, min.y + shadow_off),
                          ImVec2(max.x + shadow_off, max.y + shadow_off),
                          shadow);
    }
    if (fill & IM_COL32_A_MASK) {
        dl->AddRectFilled(min, max, fill);
    }
    if (border & IM_COL32_A_MASK) {
        dl->AddRect(min, max, border, 0.0f, 0, thick);
    }
}

void brutalist_rect(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 fill, float thick, float shadow_off) {
    brutalist_border(dl, min, max, fill, tokens::kBlack, tokens::kBlack, thick, shadow_off);
}

void brutalist_rect_thin(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 fill, float thick, float shadow_off) {
    brutalist_border(dl, min, max, fill, tokens::kBlack, tokens::kBlack, thick, shadow_off);
}

void asymmetric_clip(ImDrawList* dl,
                     const ImVec2& min,
                     const ImVec2& max,
                     ImU32 fill,
                     ImU32 border,
                     ImU32 shadow,
                     float thick,
                     float shadow_off,
                     float clip_w_pct,
                     float clip_h_pct)
{
    const float w = max.x - min.x;
    const float h = max.y - min.y;
    const float cw = w * clip_w_pct;
    const float ch = h * clip_h_pct;

    ImVec2 pts[5] = {
        ImVec2(min.x,            min.y),
        ImVec2(max.x,            min.y),
        ImVec2(max.x,            max.y - ch),
        ImVec2(max.x - cw,       max.y),
        ImVec2(min.x,            max.y),
    };

    if (shadow_off > 0.0f && (shadow & IM_COL32_A_MASK)) {
        ImVec2 sh[5];
        for (int i = 0; i < 5; ++i) {
            sh[i] = ImVec2(pts[i].x + shadow_off, pts[i].y + shadow_off);
        }
        dl->AddConvexPolyFilled(sh, 5, shadow);
    }
    if (fill & IM_COL32_A_MASK) {
        dl->AddConvexPolyFilled(pts, 5, fill);
    }
    if (border & IM_COL32_A_MASK) {
        dl->AddPolyline(pts, 5, border, ImDrawFlags_Closed, thick);
    }
}

void dashed_rect(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 color, float thick, float dash, float gap) {
    if (!(color & IM_COL32_A_MASK)) return;
    const float step = dash + gap;

    auto draw_h = [&](float y) {
        for (float x = min.x; x < max.x; x += step) {
            const float x2 = x + dash > max.x ? max.x : x + dash;
            dl->AddLine(ImVec2(x, y), ImVec2(x2, y), color, thick);
        }
    };
    auto draw_v = [&](float x) {
        for (float y = min.y; y < max.y; y += step) {
            const float y2 = y + dash > max.y ? max.y : y + dash;
            dl->AddLine(ImVec2(x, y), ImVec2(x, y2), color, thick);
        }
    };

    draw_h(min.y);
    draw_h(max.y);
    draw_v(min.x);
    draw_v(max.x);
}

ImVec2 measure_skewed_label(ImFont* font, float font_size, const char* text, float pad_x, float pad_y, float skew_x) {
    if (!text || !*text) return ImVec2(0, 0);
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text)
                            : ImGui::CalcTextSize(text);
    const float w = text_size.x + pad_x * 2.0f;
    const float h = text_size.y + pad_y * 2.0f;
    const float skew_off = std::fabs(skew_x) * h;
    return ImVec2(w + skew_off, h);
}

void skewed_label(ImDrawList* dl,
                  ImFont* font,
                  float font_size,
                  const ImVec2& origin,
                  const char* text,
                  ImU32 bg,
                  ImU32 fg,
                  float pad_x,
                  float pad_y,
                  float skew_x)
{
    if (!text || !*text) return;
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text)
                            : ImGui::CalcTextSize(text);
    const float w = text_size.x + pad_x * 2.0f;
    const float h = text_size.y + pad_y * 2.0f;
    const float dx = skew_x * h;
    const float top_off    = dx > 0.0f ? dx : 0.0f;
    const float bottom_off = dx < 0.0f ? -dx : 0.0f;

    const ImVec2 q_tl(origin.x + top_off,         origin.y);
    const ImVec2 q_tr(origin.x + top_off + w,     origin.y);
    const ImVec2 q_br(origin.x + bottom_off + w,  origin.y + h);
    const ImVec2 q_bl(origin.x + bottom_off,      origin.y + h);

    if (bg & IM_COL32_A_MASK) {
        dl->AddQuadFilled(q_tl, q_tr, q_br, q_bl, bg);
    }

    const float text_x = origin.x + (top_off + bottom_off) * 0.5f + pad_x;
    const float text_y = origin.y + pad_y;
    if (font) dl->AddText(font, font_size, ImVec2(text_x, text_y), fg, text);
    else      dl->AddText(ImVec2(text_x, text_y), fg, text);
}

bool press_button(const char* id, const ImVec2& pos, const ImVec2& size, ImVec2& out_offset) {
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(id, size);
    const bool clicked = ImGui::IsItemClicked();
    const bool active = ImGui::IsItemActive();
    out_offset = active ? ImVec2(2.0f, 2.0f) : ImVec2(0, 0);
    return clicked;
}

}
