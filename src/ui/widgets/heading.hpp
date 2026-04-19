#pragma once

#include <imgui.h>

namespace ui::widgets::heading {

void render(const ImVec2& cursor, const char* text, float available_width);

float height();

}
