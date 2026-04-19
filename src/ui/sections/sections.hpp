#pragma once

#include <imgui.h>

#include "../layout/tab_bar.hpp"

namespace ui::sections {

void render_active(layout::tab_bar::Tab active, const ImVec2& min, const ImVec2& max);

void render_run(const ImVec2& min, const ImVec2& max);
void render_visuals(const ImVec2& min, const ImVec2& max);
void render_gameplay(const ImVec2& min, const ImVec2& max);
void render_powerups(const ImVec2& min, const ImVec2& max);
void render_cosmetics(const ImVec2& min, const ImVec2& max);

}
