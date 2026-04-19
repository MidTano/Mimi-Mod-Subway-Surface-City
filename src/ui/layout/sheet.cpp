#include "sheet.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../fx/animation.hpp"
#include "../widgets/heading.hpp"

#include <cmath>
#include <cstdio>

namespace ui::layout::sheet {

namespace {

float row_height() {
    ImFont* t = fonts::get(fonts::Face::BrutalistMd);
    ImFont* s = fonts::get(fonts::Face::MonoSm);
    const float ts = t ? t->LegacySize : scale::dp(15.0f);
    const float ss = s ? s->LegacySize : scale::dp(12.0f);
    return scale::dp(28.0f) + ts + ss;
}

float total_height(int rows) {
    const float pad = scale::dp(16.0f);
    const float heading_h = widgets::heading::height() + scale::dp(14.0f);
    const float rh = row_height();
    const float gap = scale::dp(10.0f);
    return pad * 2.0f + heading_h + rows * rh + (rows - 1) * gap;
}

void draw_row(ImDrawList* dl, const ImVec2& pos, float width,
              const ItemDef& item, bool hovered, bool pressed)
{
    const float h = row_height();
    const float pad = scale::dp(14.0f);
    const ImU32 border = hovered ? tokens::kToxic : tokens::kToxicDim;
    const ImU32 bg = hovered ? tokens::kToxicFaint : IM_COL32(0, 0, 0, 0);
    ImVec2 p = pos;
    if (pressed) { p.x += 2; p.y += 2; }

    if (bg & IM_COL32_A_MASK) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), bg);
    }
    dl->AddRect(p, ImVec2(p.x + width, p.y + h), border, 0.0f, 0, scale::dp(tokens::kBorderThin));

    ImFont* icon_f = fonts::get(fonts::Face::IconLg);
    const float icon_sz = icon_f ? icon_f->LegacySize : scale::dp(22.0f);
    if (icon_f && item.icon_utf8) {
        ImVec2 isz = icon_f->CalcTextSizeA(icon_sz, FLT_MAX, 0.0f, item.icon_utf8);
        dl->AddText(icon_f, icon_sz,
                    ImVec2(p.x + pad, p.y + (h - isz.y) * 0.5f),
                    tokens::kToxic, item.icon_utf8);
    }

    ImFont* t_font = fonts::get(fonts::Face::BrutalistMd);
    ImFont* s_font = fonts::get(fonts::Face::MonoSm);
    const float ts = t_font ? t_font->LegacySize : scale::dp(15.0f);
    const float ss = s_font ? s_font->LegacySize : scale::dp(12.0f);
    const float tx = p.x + pad + icon_sz + scale::dp(12.0f);
    const float total_ch = ts + (item.subtitle ? ss + scale::dp(3.0f) : 0.0f);
    const float y = p.y + (h - total_ch) * 0.5f;
    if (t_font) dl->AddText(t_font, ts, ImVec2(tx, y), tokens::kOffWhite, item.title);
    if (s_font && item.subtitle) dl->AddText(s_font, ss, ImVec2(tx, y + ts + scale::dp(3.0f)),
                                             tokens::kOffWhiteDim, item.subtitle);

    const float cv = scale::dp(9.0f);
    const ImVec2 cc(p.x + width - pad - cv * 0.5f, p.y + h * 0.5f);
    dl->AddLine(ImVec2(cc.x - cv * 0.4f, cc.y - cv),
                ImVec2(cc.x + cv * 0.6f, cc.y), tokens::kToxic, scale::dp(2.0f));
    dl->AddLine(ImVec2(cc.x + cv * 0.6f, cc.y),
                ImVec2(cc.x - cv * 0.4f, cc.y + cv), tokens::kToxic, scale::dp(2.0f));
}

}

void open(State& s)  { s.open = true; s.last_selection = -1; }
void close(State& s) { s.open = false; }
bool is_visible(const State& s) { return s.open || s.phase > 0.0025f; }

int render(State& s,
           const ImVec2& viewport_min, const ImVec2& viewport_max,
           const char* heading,
           const ItemDef* items, int item_count,
           float dt)
{
    const float target = s.open ? 1.0f : 0.0f;
    s.phase = fx::anim::spring_to(s.phase, target, dt, 14.0f);
    if (!s.open && s.phase <= 0.0025f) return -1;

    const float t = fx::anim::clamp01(s.phase);
    const float te = fx::anim::ease_out_cubic(t);

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    dl->AddRectFilled(viewport_min, viewport_max,
                      IM_COL32(0, 0, 0, static_cast<int>(0xB0 * te)));

    const float pad = scale::dp(16.0f);
    const float h = total_height(item_count);
    const float y_closed = viewport_max.y;
    const float y_open   = viewport_max.y - h;
    const float y_top    = y_closed + (y_open - y_closed) * te;

    const float x_min = viewport_min.x;
    const float x_max = viewport_max.x;

    const ImVec2 p_min(x_min, y_top);
    const ImVec2 p_max(x_max, y_top + h);

    dl->AddRectFilled(ImVec2(p_min.x + 3, p_min.y + 3), ImVec2(p_max.x + 3, p_max.y + 3), tokens::kBlack);
    dl->AddRectFilled(p_min, p_max, tokens::kCharcoal);
    dl->AddRect(p_min, p_max, tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThick));
    dl->AddRectFilled(ImVec2(p_min.x, p_min.y), ImVec2(p_max.x, p_min.y + scale::dp(4.0f)),
                      tokens::kToxic);

    ImGui::SetNextWindowPos(viewport_min);
    ImGui::SetNextWindowSize(ImVec2(viewport_max.x - viewport_min.x, viewport_max.y - viewport_min.y));
    if (s.open && s.phase < 0.5f) ImGui::SetNextWindowFocus();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::Begin("##sheet_panel", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoSavedSettings);

    int clicked_index = -1;

    ImGui::SetCursorScreenPos(viewport_min);
    ImGui::InvisibleButton("##sheet_backdrop", ImVec2(x_max - x_min, y_top - viewport_min.y));
    if (ImGui::IsItemClicked()) {
        s.open = false;
    }

    const float heading_x = p_min.x + pad;
    const float heading_y = p_min.y + pad;
    widgets::heading::render(ImVec2(heading_x, heading_y), heading, (x_max - x_min) - pad * 2.0f);

    const float rh = row_height();
    const float gap = scale::dp(10.0f);
    float cur_y = heading_y + widgets::heading::height() + scale::dp(14.0f);

    for (int i = 0; i < item_count; ++i) {
        char id[64]; std::snprintf(id, sizeof(id), "##sheet_%s", items[i].id);
        ImGui::SetCursorScreenPos(ImVec2(p_min.x + pad, cur_y));
        ImGui::InvisibleButton(id, ImVec2((x_max - x_min) - pad * 2.0f, rh));
        const bool hov = ImGui::IsItemHovered();
        const bool act = ImGui::IsItemActive();
        if (ImGui::IsItemClicked()) {
            clicked_index = i;
            s.last_selection = i;
        }
        draw_row(dl, ImVec2(p_min.x + pad, cur_y), (x_max - x_min) - pad * 2.0f,
                 items[i], hov, act);
        cur_y += rh + gap;
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    return clicked_index;
}

}
