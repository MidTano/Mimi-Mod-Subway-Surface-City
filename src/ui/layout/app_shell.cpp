#include "app_shell.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"

namespace ui::layout::app_shell {

Rects compute(const ImVec2& vs) {
    Rects r{};
    r.viewport_min = ImVec2(0, 0);
    r.viewport_max = vs;

    const float max_w = scale::dp(tokens::kAppMaxWidth);
    const float app_w = vs.x < max_w ? vs.x : max_w;
    const float app_x = (vs.x - app_w) * 0.5f;
    const float top    = scale::safe_top();
    const float bottom = scale::safe_bottom();

    r.app_min = ImVec2(app_x, top);
    r.app_max = ImVec2(app_x + app_w, vs.y - bottom);

    const float banner_h = scale::dp(tokens::kBannerHeight + tokens::kHazardHeight + tokens::kHazardThin);
    const float stat_h   = scale::dp(tokens::kStatRowHeight);
    const float nav_h    = scale::dp(tokens::kTabBarHeight);

    r.banner_min = r.app_min;
    r.banner_max = ImVec2(r.app_max.x, r.app_min.y + banner_h);

    r.stat_min = ImVec2(r.app_min.x, r.banner_max.y);
    r.stat_max = ImVec2(r.app_max.x, r.stat_min.y + stat_h);

    r.nav_min = ImVec2(r.app_min.x, r.app_max.y - nav_h);
    r.nav_max = ImVec2(r.app_max.x, r.app_max.y);

    r.main_min = ImVec2(r.app_min.x, r.banner_max.y);
    r.main_max = ImVec2(r.app_max.x, r.nav_min.y);
    return r;
}

void render_grid_pattern(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 color, float spacing) {
    if (!(color & IM_COL32_A_MASK) || spacing <= 0.0f) return;
    for (float x = min.x; x < max.x; x += spacing) {
        dl->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), color, 1.0f);
    }
    for (float y = min.y; y < max.y; y += spacing) {
        dl->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), color, 1.0f);
    }
}

void render_background(ImDrawList* dl, const Rects& r) {
    dl->AddRectFilled(r.viewport_min, r.viewport_max, tokens::kCharcoal);
    const ImU32 grid_col = IM_COL32(0xCC, 0xFF, 0x00, 8);
    render_grid_pattern(dl, r.viewport_min, r.viewport_max, grid_col, scale::dp(40.0f));
}

}
