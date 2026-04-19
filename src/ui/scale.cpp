#include "scale.hpp"

namespace ui::scale {

namespace {
float g_density = 1.0f;
float g_safe_top = 0.0f;
float g_safe_bottom = 0.0f;
float g_safe_left = 0.0f;
float g_safe_right = 0.0f;
}

void set_density(float v) { g_density = v > 0.01f ? v : 1.0f; }
float density() { return g_density; }
float dp(float v) { return v * g_density; }
float sp(float v) { return v * g_density; }
ImVec2 dp(float x, float y) { return ImVec2(x * g_density, y * g_density); }
ImVec2 dp(const ImVec2& v) { return ImVec2(v.x * g_density, v.y * g_density); }

void set_safe_area(float top, float bottom, float left, float right) {
    g_safe_top = top; g_safe_bottom = bottom; g_safe_left = left; g_safe_right = right;
}
float safe_top()    { return g_safe_top; }
float safe_bottom() { return g_safe_bottom; }
float safe_left()   { return g_safe_left; }
float safe_right()  { return g_safe_right; }

}
