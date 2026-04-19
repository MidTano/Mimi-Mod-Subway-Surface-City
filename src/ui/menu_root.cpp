#include "menu_root.hpp"
#include "tokens.hpp"
#include "scale.hpp"
#include "layout/app_shell.hpp"
#include "layout/top_banner.hpp"
#include "layout/stat_row.hpp"
#include "layout/tab_bar.hpp"
#include "layout/fqa_dock.hpp"
#include "sections/sections.hpp"
#include "state/mod_state.hpp"
#include "fx/animation.hpp"
#include "fx/glitch.hpp"
#include "fonts/icons.hpp"
#include "widgets/color_picker.hpp"
#include "widgets/numpad.hpp"
#include "widgets/about.hpp"
#include "i18n/lang.hpp"
#include "core/runtime/runtime_state.hpp"

#include <imgui_internal.h>

#include <cmath>
#include <cstdio>

namespace ui::menu_root {

namespace {

void render_main(State& s, const ImVec2& min, const ImVec2& max, float dt) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(min, max, tokens::kCharcoal);

    const bool modal_open = s.color_picker.open || s.numpad.open || s.about.open;

    ImGui::SetCursorScreenPos(min);
    char child_id[32];
    std::snprintf(child_id, sizeof(child_id), "##sec_%d", static_cast<int>(s.tabs.active));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, tokens::kToxic);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, tokens::kToxic);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, tokens::kToxic);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, scale::dp(4.0f));

    ImGui::BeginChild(child_id,
                      ImVec2(max.x - min.x, max.y - min.y),
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground);

    const ImVec2 scroll_origin = ImGui::GetCursorScreenPos();
    const ImVec2 virt_min = scroll_origin;
    const ImVec2 virt_max(scroll_origin.x + (max.x - min.x), scroll_origin.y + (max.y - min.y));
    if (modal_open) ImGui::BeginDisabled();
    sections::render_active(s.tabs.active, virt_min, virt_max);
    if (modal_open) ImGui::EndDisabled();

    ImGuiIO& io = ImGui::GetIO();
    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                                                ImGuiHoveredFlags_ChildWindows);
    const ImVec2 mpos = io.MousePos;
    const float slop = scale::dp(8.0f);

    if (modal_open) {
        s.touch_tracking = false;
        s.touch_scroll_active = false;
        s.touch_velocity = 0.0f;
    }

    const bool any_widget_active = ImGui::IsAnyItemActive();
    if (!modal_open && !any_widget_active && ImGui::IsMouseClicked(0) && hovered) {
        s.touch_tracking = true;
        s.touch_scroll_active = false;
        s.touch_down_x = mpos.x;
        s.touch_down_y = mpos.y;
        s.touch_last_y = mpos.y;
        s.touch_start_scroll = ImGui::GetScrollY();
        s.touch_velocity = 0.0f;
    }

    if (!modal_open && s.touch_tracking) {
        if (!ImGui::IsMouseDown(0)) {
            s.touch_tracking = false;
            s.touch_scroll_active = false;
        } else {
            const float dx = std::fabs(mpos.x - s.touch_down_x);
            const float dy = std::fabs(mpos.y - s.touch_down_y);
            if (!s.touch_scroll_active && dy > slop && dy > dx * 1.25f) {
                s.touch_scroll_active = true;
            }
            if (s.touch_scroll_active) {
                const float new_scroll = s.touch_start_scroll - (mpos.y - s.touch_down_y);
                ImGui::SetScrollY(new_scroll);
                s.touch_velocity = s.touch_last_y - mpos.y;
                s.touch_last_y = mpos.y;
                ImGui::ClearActiveID();
            }
        }
    } else if (std::fabs(s.touch_velocity) > 0.1f) {
        const float friction = std::exp(-dt * 6.0f);
        s.touch_velocity *= friction;
        ImGui::SetScrollY(ImGui::GetScrollY() + s.touch_velocity);
        if (std::fabs(s.touch_velocity) < 0.1f) s.touch_velocity = 0.0f;
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
}

}

void initialize(State& s) {
    if (s.initialized) return;
    s.initialized = true;
    state::set_bool("hud.visible", true);
    state::set_float("hud.opacity", 1.0f);
}

void set_font_sources(const fonts::FontSources& src) {
    fonts::set_sources(src);
}
void set_dpi(float dpi) {
    scale::set_density(dpi);
}

void render(State& s, const ImVec2& vs, float dt) {
    if (!s.initialized) initialize(s);

    fx::glitch::set_active(&s.glitch);

    auto& rt = runtime::state();
    rt.user_time_scale.store(state::get_float("time_scale.value", 1.0f));
    rt.menu_visible.store(s.menu_visible);
    rt.fqa_overlay_active.store(!s.menu_visible && s.fqa.engaged_idx >= 0);

    const auto rects = layout::app_shell::compute(vs);

    {
        static float s_dim = 0.0f;
        const bool want_dim = rt.fqa_overlay_active.load();
        if (want_dim) {
            s_dim = 1.0f;
        } else {
            const float fade_per_sec = 1.0f / 1.5f;
            s_dim -= fade_per_sec * dt;
            if (s_dim < 0.0f) s_dim = 0.0f;
        }
        rt.dim_amount.store(s_dim);
    }

    if (!s.menu_visible) {
        ImGui::SetNextWindowPos(rects.viewport_min, ImGuiCond_Always);
        ImGui::SetNextWindowSize(vs, ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        const ImGuiWindowFlags hidden_flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        if (ImGui::Begin("##menu_hidden", nullptr, hidden_flags)) {
            const float btn_size = scale::dp(40.0f);
            const float x = rects.viewport_min.x + scale::dp(16.0f);
            const float y = rects.viewport_min.y + scale::dp(16.0f);
            const bool hud_on = state::get_bool("hud.visible", true);
            float hud_alpha = state::get_float("hud.opacity", 1.0f);
            if (!(hud_alpha > 0.0f)) hud_alpha = 0.0f;
            if (hud_alpha > 1.0f) hud_alpha = 1.0f;
            const float right_margin = scale::dp(16.0f);

            ImVec2 panel_min(x, y);
            ImVec2 panel_max;
            if (hud_on && (rects.viewport_max.x - right_margin) - x > scale::dp(180.0f)) {
                panel_max = ImVec2(rects.viewport_max.x - right_margin, y + btn_size);
            } else {
                panel_max = ImVec2(x + btn_size, y + btn_size);
            }

            ImGui::SetCursorScreenPos(ImVec2(x, y));
            ImGui::InvisibleButton("##reopen_menu", ImVec2(btn_size, btn_size));
            const bool hov = ImGui::IsItemHovered();
            const bool act = ImGui::IsItemActive();
            const bool clk = ImGui::IsItemClicked();

            if (hud_on && panel_max.x > panel_min.x + btn_size) {
                layout::stat_row::UnifiedStyle st{};
                st.button_width = btn_size;
                st.btn_hovered = hov;
                st.btn_active = act;
                st.alpha = hud_alpha;
                layout::stat_row::render_unified(panel_min, panel_max, st);
            } else {
                ImDrawList* dl = ImGui::GetForegroundDrawList();
                const float offx = act ? scale::dp(1.0f) : 0.0f;
                const float offy = act ? scale::dp(1.0f) : 0.0f;
                const float shadow_off = scale::dp(3.0f);
                const ImU32 c_shadow = tokens::with_alpha_mul(IM_COL32(0x00, 0x00, 0x00, 0xAA), hud_alpha);
                const ImU32 c_panel  = tokens::with_alpha_mul(IM_COL32(0x14, 0x14, 0x14, 0xFF), hud_alpha);
                const ImU32 c_toxic  = tokens::with_alpha_mul(tokens::kToxic, hud_alpha);
                const ImU32 c_black  = tokens::with_alpha_mul(tokens::kBlack, hud_alpha);
                dl->AddRectFilled(ImVec2(x + shadow_off, y + shadow_off),
                                  ImVec2(x + btn_size + shadow_off, y + btn_size + shadow_off),
                                  c_shadow);
                dl->AddRectFilled(ImVec2(x + offx, y + offy),
                                  ImVec2(x + btn_size + offx, y + btn_size + offy),
                                  hov ? c_toxic : c_panel);
                dl->AddRect(ImVec2(x + offx, y + offy),
                            ImVec2(x + btn_size + offx, y + btn_size + offy),
                            c_toxic, 0.0f, 0, scale::dp(tokens::kBorderThin));
                ImFont* ico = fonts::get(fonts::Face::IconLg);
                if (ico) {
                    const float isz = ico->LegacySize;
                    ImVec2 esz = ico->CalcTextSizeA(isz, FLT_MAX, 0.0f, icons::k_bolt);
                    dl->AddText(ico, isz,
                                ImVec2(x + offx + (btn_size - esz.x) * 0.5f,
                                       y + offy + (btn_size - esz.y) * 0.5f),
                                hov ? c_black : c_toxic, icons::k_bolt);
                }
            }

            if (clk) {
                s.menu_visible = true;
                fx::glitch::trigger(s.glitch, icons::k_bolt, ui::i18n::t("MENU_RESTORED"));
            }
            layout::fqa_dock::render(s.fqa, &s.glitch, rects.viewport_min, rects.viewport_max, dt);
            fx::glitch::render(s.glitch, rects.viewport_min, rects.viewport_max, dt);
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        return;
    }

    ImGui::SetNextWindowPos(rects.viewport_min, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vs, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    const bool any_modal_open = s.color_picker.open || s.numpad.open || s.about.open;

    if (ImGui::Begin("##menu_root", nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        layout::app_shell::render_background(dl, rects);

        if (any_modal_open) ImGui::BeginDisabled();

        if (layout::top_banner::render(s.banner, rects.banner_min, rects.banner_max, dt)) {
            s.menu_visible = false;
            fx::glitch::trigger(s.glitch, icons::k_close, ui::i18n::t("MENU_HIDDEN"));
        }

        if (any_modal_open) ImGui::EndDisabled();

        render_main(s, rects.main_min, rects.main_max, dt);

        if (any_modal_open) ImGui::BeginDisabled();
        layout::tab_bar::render(s.tabs, rects.nav_min, rects.nav_max, dt);
        if (any_modal_open) ImGui::EndDisabled();

        {
            char key[96]; char title[48]; ImU32 init = 0;
            if (widgets::color_picker::consume_request(key, sizeof(key), title, sizeof(title), &init)) {
                widgets::color_picker::open(s.color_picker, key, title, init);
            }
        }

        {
            char key[96]; char title[48]; char cur[24]; int max_len = 10;
            if (widgets::numpad::consume_request(key, sizeof(key), title, sizeof(title),
                                                 cur, sizeof(cur), &max_len)) {
                widgets::numpad::open(s.numpad, key, title, cur, max_len);
            }
        }

        if (widgets::about::consume_request()) {
            widgets::about::open(s.about);
        }

        fx::glitch::render(s.glitch, rects.viewport_min, rects.viewport_max, dt);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    if (widgets::color_picker::is_visible(s.color_picker)) {
        widgets::color_picker::render(s.color_picker, rects.viewport_min, rects.viewport_max, dt);
    }

    if (widgets::numpad::is_visible(s.numpad)) {
        widgets::numpad::render(s.numpad, rects.viewport_min, rects.viewport_max, dt);
    }

    if (widgets::about::is_visible(s.about)) {
        widgets::about::render(s.about, rects.viewport_min, rects.viewport_max, dt);
    }
}

}
