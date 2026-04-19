#include "sections.hpp"
#include "../tokens.hpp"
#include "../scale.hpp"
#include "../fonts/fonts.hpp"
#include "../fonts/icons.hpp"
#include "../state/mod_state.hpp"
#include "../widgets/heading.hpp"
#include "../widgets/cards.hpp"
#include "../widgets/hero_card.hpp"
#include "../widgets/numpad.hpp"
#include "../widgets/about.hpp"
#include "../i18n/lang.hpp"
#include "core/platform/android_bridge.hpp"
#include "core/il2cpp/il2cpp_bridge.hpp"
#include "core/runtime/build_stamp_gen.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace ui::sections {

namespace {

struct Ctx {
    float x;
    float y;
    float content_w;
};

Ctx begin_section(const ImVec2& min, const ImVec2& max, const char* heading_text) {
    const float pad_x = scale::dp(12.0f);
    const float pad_y = scale::dp(16.0f);
    Ctx c{};
    c.x = min.x + pad_x;
    c.y = min.y + pad_y;
    c.content_w = (max.x - min.x) - pad_x * 2.0f;

    ui::widgets::heading::render(ImVec2(c.x, c.y), ui::i18n::t(heading_text), c.content_w);
    c.y += ui::widgets::heading::height() + scale::dp(12.0f);
    return c;
}

void advance(Ctx& c, float consumed) {
    c.y += consumed + ui::widgets::cards::card_gap();
}

void format_speed(char* buf, size_t sz, float v) { std::snprintf(buf, sz, "%.1f", v); }
void format_dist(char* buf, size_t sz, int v) {
    if (v < 10000) std::snprintf(buf, sz, "%04d", v);
    else if (v < 1000000) std::snprintf(buf, sz, "%.1fK", v / 1000.0f);
    else std::snprintf(buf, sz, "%.1fM", v / 1000000.0f);
}
void format_time(char* buf, size_t sz, float v) {
    int total = static_cast<int>(v);
    int mins = total / 60;
    int secs = total % 60;
    if (mins > 99) mins = 99;
    std::snprintf(buf, sz, "%02d:%02d", mins, secs);
}

void sub_heading(Ctx& c, const char* key) {
    const char* text = ui::i18n::t(key);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* f = ui::fonts::get(ui::fonts::Face::MonoXs);
    const float fs = f ? f->LegacySize : scale::dp(11.0f);
    const float h = fs + scale::dp(8.0f);
    const float top = c.y + scale::dp(4.0f);
    const float bottom = top + h;

    const float chip_pad_x = scale::dp(8.0f);
    const float chip_pad_y = scale::dp(2.0f);
    ImVec2 tsz = f ? f->CalcTextSizeA(fs, FLT_MAX, 0.0f, text)
                   : ImVec2(scale::dp(40.0f), fs);
    const float chip_w = tsz.x + chip_pad_x * 2.0f;
    const float chip_h = tsz.y + chip_pad_y * 2.0f;
    const float cy = (top + bottom) * 0.5f;

    dl->AddRectFilled(ImVec2(c.x, cy - chip_h * 0.5f),
                      ImVec2(c.x + chip_w, cy + chip_h * 0.5f),
                      tokens::kToxic);
    if (f) {
        dl->AddText(f, fs,
                    ImVec2(c.x + chip_pad_x, cy - tsz.y * 0.5f),
                    tokens::kBlack, text);
    }
    dl->AddLine(ImVec2(c.x + chip_w + scale::dp(8.0f), cy),
                ImVec2(c.x + c.content_w, cy),
                tokens::kToxicDim, scale::dp(1.5f));

    c.y = bottom + scale::dp(6.0f);
}

void render_dashed_placeholder(Ctx& c, const char* text) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = scale::dp(46.0f);
    ImVec2 pos(c.x, c.y);
    ImVec2 end(c.x + c.content_w, c.y + h);
    dl->AddRectFilled(pos, end, IM_COL32(0, 0, 0, 0));
    ImFont* mono = ui::fonts::get(ui::fonts::Face::MonoSm);
    if (mono) {
        ImVec2 tsz = mono->CalcTextSizeA(mono->LegacySize, FLT_MAX, 0.0f, text);
        dl->AddText(mono, mono->LegacySize,
                    ImVec2(pos.x + (c.content_w - tsz.x) * 0.5f, pos.y + (h - tsz.y) * 0.5f),
                    tokens::kOffWhiteDim, text);
    }
    advance(c, h);
}

}

void render_run(const ImVec2& min, const ImVec2& max) {
    Ctx c = begin_section(min, max, "RUN_CONTROL");

    const float spd = il2cpp::get_current_player_speed();
    const int   dst = static_cast<int>(il2cpp::get_current_run_distance());
    const float tim = il2cpp::get_run_time_seconds();
    const std::uint32_t seed = il2cpp::get_effective_seed();

    char a_buf[16], b_buf[16], t_buf[16], seed_buf[24];
    format_speed(a_buf, sizeof(a_buf), spd);
    format_dist(b_buf, sizeof(b_buf), dst);
    format_time(t_buf, sizeof(t_buf), tim);
    const char* seed_lbl = ui::i18n::t("SEED");
    if (seed == 0) {
        std::snprintf(seed_buf, sizeof(seed_buf), "%s ----", seed_lbl);
    } else {
        std::uint32_t s = (seed > 9999999u) ? (seed % 10000000u) : seed;
        std::snprintf(seed_buf, sizeof(seed_buf), "%s %u", seed_lbl, s);
    }

    float used = ui::widgets::hero_card::render(
        ImVec2(c.x, c.y), c.content_w,
        "LIVE_TELEMETRY", seed_buf,
        "SPEED", a_buf, "DIST", b_buf, "TIME", t_buf);
    advance(c, used);

    sub_heading(c, "HUD");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "hud.visible", icons::k_analytics,
        "HUD_OVERLAY", "Live telemetry panel next to the menu button"));
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "hud.opacity", icons::k_opacity,
        "HUD_OPACITY", 0.0f, 1.0f, 0.01f, 1.0f, "%"));

    sub_heading(c, "FQA DOCK");
    if (!ui::state::has("fqa.enabled")) ui::state::set_bool("fqa.enabled", true);
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "fqa.enabled", icons::k_apps,
        "FQA_ENABLED", "Floating quick actions dock visibility"));
    advance(c, ui::widgets::cards::card_fqa_size(ImVec2(c.x, c.y), c.content_w));

    sub_heading(c, "MODES");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "mode.mirror", icons::k_flip,
        "MIRROR_MODE", "Swipes inverted: left becomes right, right becomes left"));
    if (!ui::state::has("ads.disable")) ui::state::set_bool("ads.disable", true);
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "ads.disable", icons::k_block,
        "NO_ADS", "Block interstitial ads"));

    sub_heading(c, "APP");
    static const char* lang_opts[] = { "EN", "RU" };
    advance(c, ui::widgets::cards::card_segmented(
        ImVec2(c.x, c.y), c.content_w,
        "cfg.lang", "LANGUAGE", lang_opts, 2));

    bool about_clicked = false;
    advance(c, ui::widgets::cards::card_nav(
        ImVec2(c.x, c.y), c.content_w,
        "cfg.about", icons::k_info,
        "ABOUT", "Mod details, credits and legal notice", &about_clicked));
    if (about_clicked) {
        ui::widgets::about::request_open();
    }

    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImFont* mono = ui::fonts::get(ui::fonts::Face::MonoXs);
        if (mono) {
            const float fs = mono->LegacySize;
            char line1[64];
            char line2[64];
            std::snprintf(line1, sizeof(line1), "BUILD  %s  %s",
                          build_stamp::kDate, build_stamp::kTime);
            std::snprintf(line2, sizeof(line2), "ID     %s",
                          build_stamp::kId);
            const float pad_top = scale::dp(10.0f);
            const float row_gap = scale::dp(2.0f);
            const float y0 = c.y + pad_top;
            ImVec2 ts1 = mono->CalcTextSizeA(fs, FLT_MAX, 0.0f, line1);
            ImVec2 ts2 = mono->CalcTextSizeA(fs, FLT_MAX, 0.0f, line2);
            dl->AddText(mono, fs,
                        ImVec2(c.x + (c.content_w - ts1.x) * 0.5f, y0),
                        tokens::kToxicDim, line1);
            dl->AddText(mono, fs,
                        ImVec2(c.x + (c.content_w - ts2.x) * 0.5f, y0 + ts1.y + row_gap),
                        tokens::kOffWhiteDim, line2);
            advance(c, pad_top + ts1.y + row_gap + ts2.y);
        }
    }

    (void)max;
    ImGui::SetCursorScreenPos(ImVec2(min.x, c.y));
    ImGui::Dummy(ImVec2(c.content_w, scale::dp(16.0f)));
}

void render_visuals(const ImVec2& min, const ImVec2& max) {
    Ctx c = begin_section(min, max, "VISUALS");

    sub_heading(c, "PALETTE");
    advance(c, ui::widgets::cards::card_color(
        ImVec2(c.x, c.y), c.content_w,
        "visuals.trail_color", "TRAIL_COLOR", tokens::kOffWhite));

    sub_heading(c, "CAMERA");
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "camera.fov", icons::k_photo_camera, "FIELD_OF_VIEW",
        50.0f, 90.0f, 1.0f, 65.0f, "\xC2\xB0"));
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "camera.distance", icons::k_zoom_out_map, "CAMERA_DISTANCE",
        0.7f, 1.5f, 0.01f, 1.0f, "x"));

    static const char* cam_opts[] = { "DEFAULT", "FPV", "TOP", "CINEMA" };
    advance(c, ui::widgets::cards::card_segmented(
        ImVec2(c.x, c.y), c.content_w,
        "camera.mode", "CAMERA_MODE", cam_opts, 4, true));

    ImGui::SetCursorScreenPos(ImVec2(min.x, c.y));
    ImGui::Dummy(ImVec2(c.content_w, scale::dp(16.0f)));
}

void render_gameplay(const ImVec2& min, const ImVec2& max) {
    Ctx c = begin_section(min, max, "GAMEPLAY");

    sub_heading(c, "TEMPO");
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "time_scale.value", icons::k_schedule, "TIME_SCALE",
        0.05f, 3.0f, 0.01f, 1.0f, "x"));

    sub_heading(c, "SURVIVAL");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "run.invincibility", icons::k_shield,
        "INVINCIBLE_MODE", "Immune to obstacles and never die"));
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "death.skip_screen", icons::k_visibility_off,
        "SKIP_DEATH_SCREEN", "Skip run-over screen after death"));
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "run.auto_restart", icons::k_autorenew,
        "AUTO_RESTART", "Instantly start new run after death"));

    sub_heading(c, "RANDOMNESS");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "seed.override", icons::k_tune,
        "SEED_OVERRIDE", "Use fixed seed for infinite runs"));
    {
        const unsigned observed =
            static_cast<unsigned>(ui::state::get_int("seed.last_observed", 0));
        const std::string input_val = ui::state::get_string("seed.value", "");
        const bool override_on = ui::state::get_bool("seed.override", false);

        char subtitle[160];
        const char* s_next   = ui::i18n::t("SEED_NEXT");
        const char* s_last   = ui::i18n::t("SEED_LAST");
        const char* s_rand   = ui::i18n::t("SEED_RANDOM");
        const char* s_tap    = ui::i18n::t("SEED_TAP_EDIT");
        if (override_on && !input_val.empty()) {
            std::snprintf(subtitle, sizeof(subtitle),
                          "%s=%s  %s=%u  %s",
                          s_next, input_val.c_str(), s_last, observed, s_tap);
        } else {
            std::snprintf(subtitle, sizeof(subtitle),
                          "%s=%s  %s=%u  %s",
                          s_next, s_rand, s_last, observed, s_tap);
        }

        bool seed_clicked = false;
        advance(c, ui::widgets::cards::card_nav(
            ImVec2(c.x, c.y), c.content_w,
            "seed.value", icons::k_tune,
            "SEED_VALUE", subtitle, &seed_clicked));
        if (seed_clicked) {
            if (platform::android::has_java_vm()) {
                platform::android::request_seed_input(input_val);
            } else {
                ui::widgets::numpad::request_open(
                    "seed.value", "SEED_VALUE",
                    input_val.c_str(), 10);
            }
        }
    }

    sub_heading(c, "MOVEMENT");
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "jump.gravity", icons::k_arrow_downward, "JUMP_GRAVITY",
        0.3f, 2.0f, 0.05f, 1.0f, "x"));
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "jump.roll", icons::k_autorenew, "ROLL_DURATION",
        0.5f, 2.0f, 0.05f, 1.0f, "x"));
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "jump.double", icons::k_keyboard_double_arrow_up,
        "INFINITE_JUMPS", "Unlimited mid-air jumps"));

    sub_heading(c, "REWARDS");
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "mult.coin", icons::k_paid, "COIN_MULTIPLIER",
        1.0f, 10.0f, 0.1f, 1.0f, "x"));
    advance(c, ui::widgets::cards::card_slider(
        ImVec2(c.x, c.y), c.content_w,
        "mult.score", icons::k_emoji_events, "SCORE_MULTIPLIER",
        1.0f, 10.0f, 0.1f, 1.0f, "x"));

    ImGui::SetCursorScreenPos(ImVec2(min.x, c.y));
    ImGui::Dummy(ImVec2(c.content_w, scale::dp(16.0f)));
}

void render_powerups(const ImVec2& min, const ImVec2& max) {
    Ctx c = begin_section(min, max, "POWER_UPS");

    sub_heading(c, "MASTER");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "pw.all", icons::k_all_inclusive,
        "FOREVER_ALL", "Master toggle: keeps every powerup active"));

    sub_heading(c, "BOOSTERS");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "pw.magnet", icons::k_near_me,
        "COIN_MAGNET", "Magnet never drains after pickup"));
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "pw.double", icons::k_close_fullscreen,
        "2X_MULTIPLIER", "Score doubler never ends"));
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "pw.bubble_gum", icons::k_blur_on,
        "BUBBLE_GUM", "Bubble gum effect stays forever"));

    ImGui::SetCursorScreenPos(ImVec2(min.x, c.y));
    ImGui::Dummy(ImVec2(c.content_w, scale::dp(16.0f)));
}

void render_cosmetics(const ImVec2& min, const ImVec2& max) {
    Ctx c = begin_section(min, max, "SKINS");

    sub_heading(c, "UNLOCKS");
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "unlock.surfers", icons::k_directions_run,
        "UNLOCK_ALL_SURFERS", "All characters treated as unlocked in menus"));
    advance(c, ui::widgets::cards::card_toggle(
        ImVec2(c.x, c.y), c.content_w,
        "unlock.boards", icons::k_skateboarding,
        "UNLOCK_ALL_BOARDS", "All hoverboards treated as unlocked in menus"));

    ImGui::SetCursorScreenPos(ImVec2(min.x, c.y));
    ImGui::Dummy(ImVec2(c.content_w, scale::dp(16.0f)));
}

void render_active(ui::layout::tab_bar::Tab active, const ImVec2& min, const ImVec2& max) {
    using T = ui::layout::tab_bar::Tab;
    switch (active) {
        case T::Run:       render_run(min, max);       break;
        case T::Visuals:   render_visuals(min, max);   break;
        case T::Gameplay:  render_gameplay(min, max);  break;
        case T::PowerUps:  render_powerups(min, max);  break;
        case T::Cosmetics: render_cosmetics(min, max); break;
        default:           render_run(min, max);       break;
    }
}

}
