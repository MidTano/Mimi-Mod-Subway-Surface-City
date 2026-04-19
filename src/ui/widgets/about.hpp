#pragma once

#include <imgui.h>

namespace ui::widgets::about {

struct State {
    bool open = false;
    bool closing = false;
    float phase = 0.0f;
};

void open(State& s);
void close(State& s);
bool is_visible(const State& s);

void request_open();
bool consume_request();

void render(State& s,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt);

}
