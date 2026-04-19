#include "heading.hpp"
#include "primitives.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"

namespace ui::widgets::heading {

float height() {
    ImFont* f = fonts::get(fonts::Face::BrutalistMd);
    const float fs = f ? f->LegacySize : 14.0f;
    return fs + scale::dp(10.0f);
}

void render(const ImVec2& cursor, const char* text, float available_width) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* f = fonts::get(fonts::Face::BrutalistMd);
    const float fs = f ? f->LegacySize : 14.0f;
    const float pad_x = scale::dp(10.0f);
    const float pad_y = scale::dp(4.0f);

    ImVec2 sz = prim::measure_skewed_label(f, fs, text, pad_x, pad_y, tokens::kSkewX);
    prim::skewed_label(dl, f, fs, cursor, text, tokens::kBlack, tokens::kToxic, pad_x, pad_y, tokens::kSkewX);

    const float line_y = cursor.y + sz.y * 0.5f - scale::dp(1.5f);
    const float line_x0 = cursor.x + sz.x + scale::dp(8.0f);
    const float line_x1 = cursor.x + available_width;
    if (line_x1 > line_x0) {
        dl->AddRectFilled(ImVec2(line_x0, line_y),
                          ImVec2(line_x1, line_y + scale::dp(3.0f)),
                          tokens::kBlack);
    }
}

}
