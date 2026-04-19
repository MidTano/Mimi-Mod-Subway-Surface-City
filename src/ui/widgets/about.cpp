#include "about.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../fx/animation.hpp"
#include "../i18n/lang.hpp"
#include "core/runtime/build_stamp_gen.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace ui::widgets::about {

namespace {

bool g_req_pending = false;

struct MetaLine {
    const char* label;
    const char* value;
};

const MetaLine kMeta[] = {
    { "VERSION",  "1.0.0-alpha" },
    { "BUILD",    "ARM64 / ARMv7" },
    { "ENGINE",   "IL2CPP + ImGui" },
    { "AUTHOR",   "MidTano" },
    { "DATE",     build_stamp::kDate },
    { "TIME",     build_stamp::kTime },
    { "ID_BUILD", build_stamp::kId },
};
constexpr int kMetaCount = static_cast<int>(sizeof(kMeta) / sizeof(kMeta[0]));

const char* kDisclaimerLines[] = {
    "unofficial modification.",
    "not affiliated with sybo or kiloo.",
    "use at your own risk.",
};
constexpr int kDisclaimerCount = static_cast<int>(sizeof(kDisclaimerLines) / sizeof(kDisclaimerLines[0]));

bool draw_close_btn(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx, ImFont* font, float alpha) {
    ImGui::SetCursorScreenPos(mn);
    ImGui::InvisibleButton("##about_close", ImVec2(mx.x - mn.x, mx.y - mn.y));
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    const float shadow = scale::dp(3.0f);
    const float inset = active ? scale::dp(2.0f) : 0.0f;
    const ImVec2 bmn(mn.x + inset, mn.y + inset);
    const ImVec2 bmx(mx.x + inset, mx.y + inset);

    const ImU32 c_shadow = tokens::with_alpha_mul(tokens::kBlack, alpha);
    const ImU32 c_fill   = tokens::with_alpha_mul(
        hovered ? tokens::kToxic : IM_COL32(0x1A, 0x1A, 0x1A, 0xFF), alpha);
    const ImU32 c_border = tokens::with_alpha_mul(
        hovered ? tokens::kBlack : tokens::kToxic, alpha);
    const ImU32 c_text   = tokens::with_alpha_mul(
        hovered ? tokens::kBlack : tokens::kToxic, alpha);

    dl->AddRectFilled(ImVec2(mn.x + shadow, mn.y + shadow),
                      ImVec2(mx.x + shadow, mx.y + shadow), c_shadow);
    dl->AddRectFilled(bmn, bmx, c_fill);
    dl->AddRect(bmn, bmx, c_border, 0.0f, 0, scale::dp(tokens::kBorderThin));

    if (font) {
        const float fs = font->LegacySize;
        const char* label = ui::i18n::t("CLOSE");
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, label);
        dl->AddText(font, fs,
                    ImVec2(bmn.x + ((bmx.x - bmn.x) - ts.x) * 0.5f,
                           bmn.y + ((bmx.y - bmn.y) - ts.y) * 0.5f),
                    c_text, label);
    }
    return clicked;
}

}

void open(State& s) {
    s.open = true;
    s.closing = false;
    s.phase = 0.0f;
}

void close(State& s) {
    if (!s.open) return;
    s.closing = true;
}

bool is_visible(const State& s) {
    return s.open || s.closing || s.phase > 0.0025f;
}

void request_open() {
    g_req_pending = true;
}

bool consume_request() {
    if (!g_req_pending) return false;
    g_req_pending = false;
    return true;
}

void render(State& s,
            const ImVec2& viewport_min,
            const ImVec2& viewport_max,
            float dt) {
    const bool visible = s.open || s.closing || s.phase > 0.0025f;
    if (!visible) return;

    const float target = (s.open && !s.closing) ? 1.0f : 0.0f;
    s.phase = fx::anim::spring_to(s.phase, target, dt, 20.0f);
    if (s.closing && s.phase <= 0.01f) {
        s.open = false;
        s.closing = false;
        s.phase = 0.0f;
        return;
    }

    const float t = fx::anim::clamp01(s.phase);
    const float te = (s.open && !s.closing)
                        ? fx::anim::ease_out_back(t)
                        : fx::anim::ease_in_quad(t);
    const float alpha = fx::anim::ease_out_cubic(t);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImU32 dim_col = IM_COL32(0, 0, 0, static_cast<int>(0xCC * alpha));
    dl->AddRectFilled(viewport_min, viewport_max, dim_col);

    const float vw = viewport_max.x - viewport_min.x;
    const float vh = viewport_max.y - viewport_min.y;

    const float panel_w = std::min(scale::dp(340.0f), vw - scale::dp(32.0f));
    const float panel_h = std::min(scale::dp(500.0f), vh - scale::dp(48.0f));
    const float y_offset = (1.0f - te) * scale::dp(44.0f);
    const float px = viewport_min.x + (vw - panel_w) * 0.5f;
    const float py = viewport_min.y + (vh - panel_h) * 0.5f + y_offset;
    const ImVec2 pmin(px, py);
    const ImVec2 pmax(px + panel_w, py + panel_h);

    const float shadow_off = scale::dp(4.0f);
    dl->AddRectFilled(ImVec2(pmin.x + shadow_off, pmin.y + shadow_off),
                      ImVec2(pmax.x + shadow_off, pmax.y + shadow_off),
                      tokens::with_alpha_mul(tokens::kBlack, alpha));
    dl->AddRectFilled(pmin, pmax, tokens::with_alpha_mul(tokens::kCharcoal, alpha));
    dl->AddRect(pmin, pmax, tokens::with_alpha_mul(tokens::kToxic, alpha),
                0.0f, 0, scale::dp(tokens::kBorderThin));

    ImFont* brand_f = fonts::get(fonts::Face::HeroMd);
    ImFont* head_f  = fonts::get(fonts::Face::BrutalistMd);
    ImFont* lbl_f   = fonts::get(fonts::Face::MonoXs);
    ImFont* val_f   = fonts::get(fonts::Face::MonoSm);
    ImFont* icon_f  = fonts::get(fonts::Face::IconXl);

    const float content_pad = scale::dp(20.0f);
    float cur_y = pmin.y + scale::dp(22.0f);

    if (icon_f) {
        const float fs = icon_f->LegacySize;
        ImVec2 ts = icon_f->CalcTextSizeA(fs, FLT_MAX, 0.0f, icons::k_info);
        const float ix = pmin.x + (panel_w - ts.x) * 0.5f;
        dl->AddText(icon_f, fs,
                    ImVec2(ix, cur_y),
                    tokens::with_alpha_mul(tokens::kToxic, alpha),
                    icons::k_info);
        cur_y += ts.y + scale::dp(8.0f);
    }

    if (brand_f) {
        const float fs = brand_f->LegacySize * 0.7f;
        const char* brand = "SUBWAY CITY";
        ImVec2 ts = brand_f->CalcTextSizeA(fs, FLT_MAX, 0.0f, brand);
        dl->AddText(brand_f, fs,
                    ImVec2(pmin.x + (panel_w - ts.x) * 0.5f, cur_y),
                    tokens::with_alpha_mul(tokens::kOffWhite, alpha),
                    brand);
        cur_y += ts.y + scale::dp(4.0f);
    }

    if (lbl_f) {
        const float fs = lbl_f->LegacySize;
        const char* tag = ui::i18n::t("ABOUT_TAG");
        ImVec2 ts = lbl_f->CalcTextSizeA(fs, FLT_MAX, 0.0f, tag);
        dl->AddText(lbl_f, fs,
                    ImVec2(pmin.x + (panel_w - ts.x) * 0.5f, cur_y),
                    tokens::with_alpha_mul(tokens::kOffWhiteDim, alpha),
                    tag);
        cur_y += ts.y + scale::dp(14.0f);
    }

    {
        const float sep_w = scale::dp(40.0f);
        const float sep_y = cur_y;
        const float sep_x = pmin.x + (panel_w - sep_w) * 0.5f;
        dl->AddRectFilled(ImVec2(sep_x, sep_y),
                          ImVec2(sep_x + sep_w, sep_y + scale::dp(2.0f)),
                          tokens::with_alpha_mul(tokens::kToxic, alpha));
        cur_y = sep_y + scale::dp(14.0f);
    }

    const float row_h = scale::dp(20.0f);
    const float lbl_col_w = scale::dp(80.0f);
    const float row_left = pmin.x + content_pad;
    const float row_right = pmax.x - content_pad;

    for (int i = 0; i < kMetaCount; ++i) {
        if (lbl_f && kMeta[i].label) {
            const float fs = lbl_f->LegacySize;
            dl->AddText(lbl_f, fs,
                        ImVec2(row_left, cur_y + (row_h - fs) * 0.5f),
                        tokens::with_alpha_mul(tokens::kToxicDim, alpha),
                        ui::i18n::t(kMeta[i].label));
        }
        if (val_f && kMeta[i].value) {
            const float fs = val_f->LegacySize;
            dl->AddText(val_f, fs,
                        ImVec2(row_left + lbl_col_w, cur_y + (row_h - fs) * 0.5f),
                        tokens::with_alpha_mul(tokens::kOffWhite, alpha),
                        kMeta[i].value);
        }
        cur_y += row_h;
    }

    cur_y += scale::dp(14.0f);

    {
        ImFont* warn_icon_f = fonts::get(fonts::Face::IconMd);
        const float box_pad_x = scale::dp(12.0f);
        const float box_pad_y = scale::dp(10.0f);
        const float icon_col_w = scale::dp(26.0f);
        const float line_gap = scale::dp(3.0f);
        const float box_left = row_left;
        const float box_right = row_right;
        const float text_left = box_left + box_pad_x + icon_col_w;
        const float text_right = box_right - box_pad_x;

        float text_total_h = 0.0f;
        if (lbl_f) {
            const float fs = lbl_f->LegacySize;
            for (int i = 0; i < kDisclaimerCount; ++i) {
                const char* text = ui::i18n::t(kDisclaimerLines[i]);
                ImVec2 ts = lbl_f->CalcTextSizeA(fs, text_right - text_left, text_right - text_left, text);
                text_total_h += ts.y + (i + 1 < kDisclaimerCount ? line_gap : 0.0f);
            }
        }

        const float box_h = std::max(text_total_h + box_pad_y * 2.0f, scale::dp(48.0f));
        const ImVec2 box_mn(box_left, cur_y);
        const ImVec2 box_mx(box_right, cur_y + box_h);

        const ImU32 c_bg = tokens::with_alpha_mul(IM_COL32(0x17, 0x17, 0x17, 0xFF), alpha);
        const ImU32 c_br = tokens::with_alpha_mul(tokens::kToxicDim, alpha);
        dl->AddRectFilled(box_mn, box_mx, c_bg, scale::dp(2.0f));
        dl->AddRect(box_mn, box_mx, c_br, scale::dp(2.0f), 0, scale::dp(1.0f));

        const float accent_w = scale::dp(3.0f);
        dl->AddRectFilled(box_mn, ImVec2(box_mn.x + accent_w, box_mx.y),
                          tokens::with_alpha_mul(tokens::kHazardOrange, alpha));

        if (warn_icon_f) {
            const float fs = warn_icon_f->LegacySize * 0.95f;
            ImVec2 ts = warn_icon_f->CalcTextSizeA(fs, FLT_MAX, 0.0f, icons::k_warning);
            const float ix = box_mn.x + accent_w + (icon_col_w + box_pad_x - ts.x) * 0.5f;
            const float iy = box_mn.y + (box_h - ts.y) * 0.5f;
            dl->AddText(warn_icon_f, fs, ImVec2(ix, iy),
                        tokens::with_alpha_mul(tokens::kHazardOrange, alpha),
                        icons::k_warning);
        }

        if (lbl_f) {
            const float fs = lbl_f->LegacySize;
            float ty = box_mn.y + (box_h - text_total_h) * 0.5f;
            for (int i = 0; i < kDisclaimerCount; ++i) {
                const char* text = ui::i18n::t(kDisclaimerLines[i]);
                ImVec2 ts = lbl_f->CalcTextSizeA(fs, text_right - text_left, text_right - text_left, text);
                dl->AddText(lbl_f, fs, ImVec2(text_left, ty),
                            tokens::with_alpha_mul(tokens::kOffWhiteDim, alpha),
                            text);
                ty += ts.y + line_gap;
            }
        }

        cur_y = box_mx.y + scale::dp(10.0f);
    }

    const float btn_h = scale::dp(40.0f);
    const float btn_w = panel_w - content_pad * 2.0f;
    const ImVec2 btn_mn(pmin.x + content_pad, pmax.y - btn_h - scale::dp(16.0f));
    const ImVec2 btn_mx(btn_mn.x + btn_w, btn_mn.y + btn_h);

    ImGui::SetNextWindowPos(viewport_min);
    ImGui::SetNextWindowSize(ImVec2(viewport_max.x - viewport_min.x,
                                    viewport_max.y - viewport_min.y));
    if (s.open && s.phase < 0.5f) ImGui::SetNextWindowFocus();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    const bool interactive = (s.open && !s.closing) && t >= 0.92f;

    if (ImGui::Begin("##about_overlay", nullptr, flags)) {
        if (!interactive) ImGui::BeginDisabled();

        if (draw_close_btn(dl, btn_mn, btn_mx, head_f, alpha)) {
            close(s);
        }

        if (interactive && ImGui::IsMouseClicked(0) &&
            !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive()) {
            const ImVec2 mp = ImGui::GetIO().MousePos;
            const bool inside = (mp.x >= pmin.x && mp.x <= pmax.x &&
                                 mp.y >= pmin.y && mp.y <= pmax.y);
            if (!inside) close(s);
        }

        if (!interactive) ImGui::EndDisabled();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

}
