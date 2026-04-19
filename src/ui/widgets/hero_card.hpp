#pragma once

#include <imgui.h>

namespace ui::widgets::hero_card {

float render(const ImVec2& pos, float width,
             const char* title,
             const char* status,
             const char* col_a_label, const char* col_a_value,
             const char* col_b_label, const char* col_b_value,
             const char* col_c_label, const char* col_c_value);

}
