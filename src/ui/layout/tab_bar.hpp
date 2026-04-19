#pragma once

#include <imgui.h>

#include <string>

namespace ui::layout::tab_bar {

enum class Tab : int {
    Run = 0,
    Visuals,
    Gameplay,
    PowerUps,
    Cosmetics,
    Count
};

struct State {
    Tab active = Tab::Run;
    Tab last_active = Tab::Run;
    float anim_phase = 1.0f;
};

void render(State& s, const ImVec2& min, const ImVec2& max, float dt);

const char* tab_id(Tab t);
const char* tab_label(Tab t);

}
