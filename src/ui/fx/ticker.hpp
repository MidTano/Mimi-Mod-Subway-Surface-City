#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace ui::fx::ticker {

struct TickerSegment {
    std::string text;
    bool dot = false;
};

void render_marquee(ImDrawList* dl,
                    const ImVec2& bounds_min,
                    const ImVec2& bounds_max,
                    const std::vector<TickerSegment>& segments,
                    float scroll_speed_px_per_sec,
                    float dt,
                    float& scroll_state,
                    ImU32 text_color,
                    ImU32 dot_color,
                    ImFont* font,
                    float gap_px);

void render_hazard(ImDrawList* dl,
                   const ImVec2& bounds_min,
                   const ImVec2& bounds_max,
                   bool reverse,
                   float speed_px_per_sec,
                   float dt,
                   float& offset_state,
                   ImU32 hazard_color,
                   ImU32 dark_color,
                   float stripe_w);

}
