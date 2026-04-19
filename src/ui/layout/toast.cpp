#include "toast.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fx/animation.hpp"

#include <cstdio>
#include <cstring>

namespace ui::layout::toast {

void show(State& s, const char* msg) {
    if (!msg) msg = "";
    std::snprintf(s.msg, sizeof(s.msg), "%s", msg);
    s.remaining = s.duration;
    s.enter_phase = 0.0f;
}

void render(State& s, const ImVec2& viewport_min, const ImVec2& viewport_max, float dt) {
    if (s.remaining <= 0.0f) {
        s.enter_phase = fx::anim::spring_to(s.enter_phase, 0.0f, dt, 14.0f);
        if (s.enter_phase < 0.005f) return;
    } else {
        s.remaining -= dt;
        s.enter_phase = fx::anim::spring_to(s.enter_phase, 1.0f, dt, 14.0f);
    }

    ImFont* f = fonts::get(fonts::Face::MonoSm);
    const float fs = f ? f->LegacySize : scale::dp(12.0f);
    ImVec2 sz = f ? f->CalcTextSizeA(fs, FLT_MAX, 0.0f, s.msg) : ImGui::CalcTextSize(s.msg);
    const float pad_x = scale::dp(18.0f);
    const float pad_y = scale::dp(12.0f);
    const float w = sz.x + pad_x * 2.0f;
    const float h = sz.y + pad_y * 2.0f;

    const float anchor_y = viewport_max.y - scale::dp(96.0f);
    const float y_closed = anchor_y + scale::dp(30.0f);
    const float y_open   = anchor_y;
    const float t = fx::anim::clamp01(s.enter_phase);
    const float te = fx::anim::ease_out_back(t);
    const float y = y_closed + (y_open - y_closed) * te;
    const float x = (viewport_min.x + viewport_max.x) * 0.5f - w * 0.5f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(ImVec2(x + 3, y + 3), ImVec2(x + w + 3, y + h + 3), tokens::kBlack);
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), tokens::kBlack);
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), tokens::kToxic, 0.0f, 0, scale::dp(tokens::kBorderThin));
    if (f) dl->AddText(f, fs, ImVec2(x + pad_x, y + pad_y), tokens::kToxic, s.msg);
}

}
