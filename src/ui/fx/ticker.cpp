#include "ticker.hpp"

#include <cmath>

namespace ui::fx::ticker {

void render_marquee(ImDrawList* dl,
                    const ImVec2& bmin,
                    const ImVec2& bmax,
                    const std::vector<TickerSegment>& segments,
                    float speed,
                    float dt,
                    float& state,
                    ImU32 text_color,
                    ImU32 dot_color,
                    ImFont* font,
                    float gap)
{
    if (segments.empty()) return;
    const float h = bmax.y - bmin.y;
    const float w = bmax.x - bmin.x;
    if (h <= 0.0f || w <= 0.0f) return;

    const float font_size = font ? font->LegacySize : 11.0f;

    const int n = static_cast<int>(segments.size());
    float widths_stack[64];
    float text_h_stack[64];
    float* widths = widths_stack;
    float* text_h = text_h_stack;
    std::vector<float> widths_heap, text_h_heap;
    if (n > 64) {
        widths_heap.resize(n);
        text_h_heap.resize(n);
        widths = widths_heap.data();
        text_h = text_h_heap.data();
    }

    float total_w = 0.0f;
    for (int i = 0; i < n; ++i) {
        const auto& s = segments[i];
        if (s.dot) {
            widths[i] = 5.0f + gap;
            text_h[i] = 0.0f;
        } else {
            ImVec2 sz = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, s.text.c_str())
                             : ImGui::CalcTextSize(s.text.c_str());
            widths[i] = sz.x + gap;
            text_h[i] = sz.y;
        }
        total_w += widths[i];
    }
    if (total_w <= 0.0f) return;

    state += speed * dt;
    if (state >= total_w) state = std::fmod(state, total_w);

    dl->PushClipRect(bmin, bmax, true);

    float x = bmin.x - state;
    const float cy_dot = bmin.y + h * 0.5f;
    while (x < bmax.x) {
        for (int i = 0; i < n; ++i) {
            const auto& s = segments[i];
            if (s.dot) {
                dl->AddCircleFilled(ImVec2(x + 2.5f, cy_dot), 2.5f, dot_color, 6);
            } else {
                const float ty = bmin.y + (h - text_h[i]) * 0.5f;
                if (font) dl->AddText(font, font_size, ImVec2(x, ty), text_color, s.text.c_str());
                else      dl->AddText(ImVec2(x, ty), text_color, s.text.c_str());
            }
            x += widths[i];
            if (x >= bmax.x) break;
        }
    }

    dl->PopClipRect();
}

void render_hazard(ImDrawList* dl,
                   const ImVec2& bmin,
                   const ImVec2& bmax,
                   bool reverse,
                   float speed,
                   float dt,
                   float& offset,
                   ImU32 hazard,
                   ImU32 dark,
                   float stripe_w)
{
    const float h = bmax.y - bmin.y;
    const float w = bmax.x - bmin.x;
    if (h <= 0.0f || w <= 0.0f) return;

    offset += (reverse ? -speed : speed) * dt;
    const float period = stripe_w * 2.0f;
    if (offset > period) offset = std::fmod(offset, period);
    if (offset < 0.0f)   offset = period - std::fmod(-offset, period);

    dl->PushClipRect(bmin, bmax, true);
    dl->AddRectFilled(bmin, bmax, dark);

    const float skew = h;
    const float start_x = bmin.x - skew - period;
    const float end_x   = bmax.x + period;

    int idx = 0;
    for (float x = start_x - std::fmod(offset, period); x < end_x; x += stripe_w, ++idx) {
        if ((idx & 1) == 0) {
            const ImVec2 p1(x, bmin.y);
            const ImVec2 p2(x + stripe_w, bmin.y);
            const ImVec2 p3(x + stripe_w + skew, bmax.y);
            const ImVec2 p4(x + skew, bmax.y);
            dl->AddQuadFilled(p1, p2, p3, p4, hazard);
        }
    }

    dl->PopClipRect();
}

}
