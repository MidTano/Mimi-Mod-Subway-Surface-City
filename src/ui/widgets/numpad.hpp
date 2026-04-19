#pragma once

#include <imgui.h>

namespace ui::widgets::numpad {

struct State {
    bool open = false;
    bool closing = false;
    float phase = 0.0f;
    char state_key[96] = {0};
    char title[48] = {0};
    char buffer[24] = {0};
    int  max_len = 10;
};

void open(State& s, const char* state_key, const char* title,
          const char* current, int max_len);
void close(State& s);
bool is_visible(const State& s);

void request_open(const char* state_key, const char* title,
                  const char* current, int max_len);
bool consume_request(char* out_key, size_t key_cap,
                     char* out_title, size_t title_cap,
                     char* out_current, size_t cur_cap,
                     int* out_max_len);

void render(State& s,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt);

}
