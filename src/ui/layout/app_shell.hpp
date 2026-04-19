#pragma once

#include <imgui.h>

namespace ui::layout::app_shell {

struct Rects {
    ImVec2 viewport_min;
    ImVec2 viewport_max;
    ImVec2 app_min;
    ImVec2 app_max;
    ImVec2 banner_min;
    ImVec2 banner_max;
    ImVec2 stat_min;
    ImVec2 stat_max;
    ImVec2 main_min;
    ImVec2 main_max;
    ImVec2 nav_min;
    ImVec2 nav_max;
};

Rects compute(const ImVec2& viewport_size);

void render_background(ImDrawList* dl, const Rects& r);
void render_grid_pattern(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 line_color, float spacing);

}
