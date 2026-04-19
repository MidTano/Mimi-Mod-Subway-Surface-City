#pragma once

#include <imgui.h>

namespace ui::layout::stat_row {

void render(const ImVec2& min, const ImVec2& max);

struct UnifiedStyle {
    float button_width;
    bool  btn_hovered;
    bool  btn_active;
    float alpha;
};

void render_unified(const ImVec2& min, const ImVec2& max, const UnifiedStyle& style);

}
