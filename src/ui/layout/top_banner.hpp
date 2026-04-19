#pragma once

#include <imgui.h>

namespace ui::layout::top_banner {

struct State {
    float hazard_top_offset = 0.0f;
    float hazard_bot_offset = 0.0f;
    float ticker_state = 0.0f;
};

bool render(State& s,
            const ImVec2& min,
            const ImVec2& max,
            float dt);

}
