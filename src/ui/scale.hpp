#pragma once

#include <imgui.h>

namespace ui::scale {

void set_density(float dpi_scale);
float density();
float dp(float value);
float sp(float value);
ImVec2 dp(float x, float y);
ImVec2 dp(const ImVec2& v);

void set_safe_area(float top, float bottom, float left, float right);
float safe_top();
float safe_bottom();
float safe_left();
float safe_right();

}
