#pragma once

#include <imgui.h>

namespace ui::widgets::cards {

float card_toggle(const ImVec2& pos, float width,
                  const char* state_key,
                  const char* icon_utf8,
                  const char* title,
                  const char* subtitle);

float card_slider(const ImVec2& pos, float width,
                  const char* state_key,
                  const char* icon_utf8,
                  const char* title,
                  float min_v, float max_v, float step, float def_v,
                  const char* fmt);

float card_segmented(const ImVec2& pos, float width,
                     const char* state_key,
                     const char* title,
                     const char* const* options, int option_count,
                     bool silent = false);

float card_color(const ImVec2& pos, float width,
                 const char* state_key,
                 const char* title,
                 ImU32 default_color);

float card_nav(const ImVec2& pos, float width,
               const char* state_key,
               const char* icon_utf8,
               const char* title,
               const char* subtitle,
               bool* out_clicked);

float card_button_action(const ImVec2& pos, float width,
                         const char* id,
                         const char* label,
                         ImU32 bg_color,
                         ImU32 fg_color,
                         bool* out_clicked);

float card_text_input(const ImVec2& pos, float width,
                      const char* state_key,
                      const char* title,
                      const char* hint,
                      const char* footer,
                      int max_len);

float card_fqa_size(const ImVec2& pos, float width);

float card_gap();

}
