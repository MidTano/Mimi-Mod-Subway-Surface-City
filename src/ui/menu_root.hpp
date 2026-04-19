#pragma once

#include <imgui.h>

#include "fonts/fonts.hpp"
#include "fx/glitch.hpp"
#include "layout/tab_bar.hpp"
#include "layout/top_banner.hpp"
#include "layout/fqa_dock.hpp"
#include "widgets/color_picker.hpp"
#include "widgets/numpad.hpp"
#include "widgets/about.hpp"

namespace ui::menu_root {

struct State {
    bool initialized = false;
    bool menu_visible = false;
    layout::tab_bar::State tabs{};
    layout::top_banner::State banner{};
    layout::fqa_dock::State fqa{};
    widgets::color_picker::State color_picker{};
    widgets::numpad::State numpad{};
    widgets::about::State about{};
    fx::glitch::State glitch{};
    float scroll_offset = 0.0f;
    float scroll_target = 0.0f;
    bool touch_tracking = false;
    bool touch_scroll_active = false;
    float touch_down_x = 0.0f;
    float touch_down_y = 0.0f;
    float touch_start_scroll = 0.0f;
    float touch_last_y = 0.0f;
    float touch_velocity = 0.0f;
};

void initialize(State& s);
void render(State& s, const ImVec2& viewport_size, float dt);

void set_font_sources(const fonts::FontSources& src);
void set_dpi(float dpi);

}
