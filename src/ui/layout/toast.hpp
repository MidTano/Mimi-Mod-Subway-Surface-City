#pragma once

#include <imgui.h>

namespace ui::layout::toast {

struct State {
    char msg[160] = {0};
    float remaining = 0.0f;
    float duration = 1.6f;
    float enter_phase = 0.0f;
};

void show(State& s, const char* msg);
void render(State& s, const ImVec2& viewport_min, const ImVec2& viewport_max, float dt);

}
