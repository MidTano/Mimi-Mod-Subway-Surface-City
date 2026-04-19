#pragma once

#include <imgui.h>

namespace ui::widgets::prim {

void brutalist_border(ImDrawList* dl,
                      const ImVec2& min,
                      const ImVec2& max,
                      ImU32 fill,
                      ImU32 border,
                      ImU32 shadow,
                      float border_thick,
                      float shadow_offset);

void brutalist_rect(ImDrawList* dl,
                    const ImVec2& min,
                    const ImVec2& max,
                    ImU32 fill,
                    float thick = 3.0f,
                    float shadow_offset = 4.0f);

void brutalist_rect_thin(ImDrawList* dl,
                         const ImVec2& min,
                         const ImVec2& max,
                         ImU32 fill,
                         float thick = 2.0f,
                         float shadow_offset = 3.0f);

void asymmetric_clip(ImDrawList* dl,
                     const ImVec2& min,
                     const ImVec2& max,
                     ImU32 fill,
                     ImU32 border,
                     ImU32 shadow,
                     float border_thick,
                     float shadow_offset,
                     float clip_w_pct = 0.05f,
                     float clip_h_pct = 0.15f);

void dashed_rect(ImDrawList* dl,
                 const ImVec2& min,
                 const ImVec2& max,
                 ImU32 color,
                 float thick,
                 float dash_len,
                 float gap_len);

void skewed_label(ImDrawList* dl,
                  ImFont* font,
                  float font_size,
                  const ImVec2& origin,
                  const char* text,
                  ImU32 bg,
                  ImU32 fg,
                  float pad_x,
                  float pad_y,
                  float skew_x);

ImVec2 measure_skewed_label(ImFont* font,
                            float font_size,
                            const char* text,
                            float pad_x,
                            float pad_y,
                            float skew_x);

bool press_button(const char* id,
                  const ImVec2& pos,
                  const ImVec2& size,
                  ImVec2& out_offset);

}
