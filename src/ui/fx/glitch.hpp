#pragma once

#include <imgui.h>

namespace ui::fx::glitch {

struct State {
    float phase = 0.0f;
    float duration = 0.52f;
    char icon_utf8[16] = {0};
    char label[64] = {0};
};

void trigger(State& s, const char* icon_utf8, const char* label);
void render(State& s, const ImVec2& viewport_min, const ImVec2& viewport_max, float dt);

void set_active(State* s);
void notify(const char* icon_utf8, const char* label);

}
