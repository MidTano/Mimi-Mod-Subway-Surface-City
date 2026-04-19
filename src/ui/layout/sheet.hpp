#pragma once

#include <imgui.h>

namespace ui::layout::sheet {

struct ItemDef {
    const char* id;
    const char* icon_utf8;
    const char* title;
    const char* subtitle;
};

struct State {
    bool open = false;
    float phase = 0.0f;
    int last_selection = -1;
};

void open(State& s);
void close(State& s);
bool is_visible(const State& s);

int render(State& s,
           const ImVec2& viewport_min, const ImVec2& viewport_max,
           const char* heading,
           const ItemDef* items, int item_count,
           float dt);

}
