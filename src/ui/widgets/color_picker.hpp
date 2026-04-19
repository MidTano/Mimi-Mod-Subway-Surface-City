#pragma once

#include <imgui.h>

namespace ui::widgets::color_picker {

struct State {
    bool open = false;
    float phase = 0.0f;
    char state_key[96] = {0};
    char title[48] = {0};
    ImU32 initial = 0;
    ImU32 current = 0;
};

void open(State& s, const char* state_key, const char* title, ImU32 initial);
void close(State& s);
bool is_visible(const State& s);

void request_open(const char* state_key, const char* title, ImU32 initial);
bool consume_request(char* out_key, size_t key_cap, char* out_title, size_t title_cap, ImU32* out_initial);

void render(State& s,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt);

}
