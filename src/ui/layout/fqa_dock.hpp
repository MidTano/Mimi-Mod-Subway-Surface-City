#pragma once

#include <imgui.h>

namespace ui::fx::glitch { struct State; }

namespace ui::layout::fqa_dock {

struct State {
    bool dragging = false;
    ImVec2 offset = ImVec2(-16.0f, 96.0f);
    float collapse_phase = 0.0f;
    bool hub_expanded = false;
    float hub_timer = 0.0f;

    int engaged_idx = -1;
    float engaged_anim = 0.0f;
    float engaged_value = 0.0f;
    int engaged_segment = -1;
    bool engaged_had_move = false;
    ImVec2 engaged_center = ImVec2(0.0f, 0.0f);
};

void render(State& s,
            fx::glitch::State* glitch,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt);

}
