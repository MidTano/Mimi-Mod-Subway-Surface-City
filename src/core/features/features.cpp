#include "mod_features.h"
#include "../il2cpp/il2cpp_bridge.hpp"
#include "../il2cpp/unlocks.hpp"
#include "../platform/android_bridge.hpp"
#include "../runtime/runtime_state.hpp"
#include "ui/state/mod_state.hpp"
#include "ui/i18n/lang.hpp"

#include <chrono>
#include <cmath>

namespace Features {

FeatureManager::FeatureManager() {
}

FeatureManager::~FeatureManager() {
}

FeatureManager& FeatureManager::getInstance() {
    static FeatureManager instance;
    return instance;
}

void FeatureManager::initialize() {
}

void FeatureManager::update() {
    if (!il2cpp::is_resolved()) {
        if (!il2cpp::resolve()) return;
    }

    using Clock = std::chrono::steady_clock;
    static Clock::time_point s_last_time = Clock::now();
    const Clock::time_point now = Clock::now();
    const float dt = std::chrono::duration<float>(now - s_last_time).count();
    s_last_time = now;

    constexpr float kMenuFreeze = 0.01f;
    constexpr float kFqaFreeze  = 0.20f;
    constexpr float kEaseDuration = 1.5f;

    auto& rt = runtime::state();
    const float user_ts = rt.user_time_scale.load();
    const bool menu_vis = rt.menu_visible.load();
    const bool fqa_on  = rt.fqa_overlay_active.load();

    static float s_current = 1.0f;
    static int   s_freeze_prev = 0;
    static float s_ease_from = 1.0f;
    static float s_ease_elapsed = 0.0f;

    const int freeze_kind = menu_vis ? 2 : (fqa_on ? 1 : 0);

    float target;
    if (freeze_kind != 0) {
        const float freeze_val = (freeze_kind == 2) ? kMenuFreeze : kFqaFreeze;
        target = freeze_val;
        s_current = freeze_val;
        s_ease_from = freeze_val;
        s_ease_elapsed = 0.0f;
        s_freeze_prev = freeze_kind;
    } else {
        if (s_freeze_prev != 0) {
            s_ease_from = s_current;
            s_ease_elapsed = 0.0f;
            s_freeze_prev = 0;
        }
        if (s_ease_elapsed < kEaseDuration) {
            s_ease_elapsed += dt;
            float t = s_ease_elapsed / kEaseDuration;
            if (t > 1.0f) t = 1.0f;
            const float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            s_current = s_ease_from + (user_ts - s_ease_from) * eased;
        } else {
            s_current = user_ts;
        }
        target = s_current;
    }

    il2cpp::set_time_scale(target);
    rt.applied_time_scale.store(target);


    {
        static bool s_lifecycle_tried = false;
        if (!s_lifecycle_tried) {
            s_lifecycle_tried = true;
            il2cpp::install_run_lifecycle_hooks();
        }
    }

    {
        static bool s_fov_hook_tried = false;
        if (!s_fov_hook_tried) {
            s_fov_hook_tried = true;
            il2cpp::install_fov_hook();
        }
        const float fov = ui::state::get_float("camera.fov", 65.0f);
        il2cpp::set_fov_override(fov);
    }

    {
        static bool s_camfollow_hook_tried = false;
        if (!s_camfollow_hook_tried) {
            s_camfollow_hook_tried = true;
            il2cpp::install_camera_follow_hook();
        }
        static bool s_player_input_hook_tried = false;
        if (!s_player_input_hook_tried) {
            s_player_input_hook_tried = true;
            il2cpp::install_player_input_hook();
        }
        static bool s_unlocks_hook_tried = false;
        if (!s_unlocks_hook_tried) {
            s_unlocks_hook_tried = true;
            il2cpp::unlocks::install_hooks();
        }
        il2cpp::unlocks::set_unlock_all_surfers(ui::state::get_bool("unlock.surfers", false));
        il2cpp::unlocks::set_unlock_all_boards(ui::state::get_bool("unlock.boards", false));
        if (!ui::state::has("ads.disable")) ui::state::set_bool("ads.disable", true);
        il2cpp::unlocks::set_disable_ads(ui::state::get_bool("ads.disable", true));
        ui::i18n::set_lang(static_cast<ui::i18n::Lang>(ui::state::get_int("cfg.lang", 0)));
        const int mode = ui::state::get_int("camera.mode", 0);
        const float dist = ui::state::get_float("camera.distance", 1.0f);
        const bool mirror = ui::state::get_bool("mode.mirror", false);
        il2cpp::set_camera_mode(mode);
        il2cpp::set_camera_distance_mult(dist);
        il2cpp::set_mirror_mode(mirror);
    }

    {
        static bool s_gravity_hook_tried = false;
        if (!s_gravity_hook_tried) {
            s_gravity_hook_tried = true;
            il2cpp::install_jump_gravity_hook();
        }
        const float mult = ui::state::get_float("jump.gravity", 1.0f);
        il2cpp::set_jump_gravity_mult(mult);
    }

    {
        static bool s_roll_hook_tried = false;
        if (!s_roll_hook_tried) {
            s_roll_hook_tried = true;
            il2cpp::install_roll_duration_hook();
        }
        const float mult = ui::state::get_float("jump.roll", 1.0f);
        il2cpp::set_roll_duration_mult(mult);
    }

    {
        static bool s_score_hook_tried = false;
        if (!s_score_hook_tried) {
            s_score_hook_tried = true;
            il2cpp::install_score_hook();
        }
        const float mult = ui::state::get_float("mult.score", 1.0f);
        il2cpp::set_score_mult(mult);
    }

    {
        const float mult_f = ui::state::get_float("mult.coin", 1.0f);
        il2cpp::set_coin_pickup_multiplier(mult_f);
    }

    {
        unsigned mask = 0;
        const bool all = ui::state::get_bool("pw.all", false);
        if (all || ui::state::get_bool("pw.magnet", false))     mask |= il2cpp::PWR_MAGNET;
        if (all || ui::state::get_bool("pw.double", false))     mask |= il2cpp::PWR_COIN_DBL;
        if (all || ui::state::get_bool("pw.bubble_gum", false)) mask |= il2cpp::PWR_BUBBLE_GUM;
        il2cpp::set_powerup_forever_mask(mask);

        const bool magnet_on = (mask & il2cpp::PWR_MAGNET) != 0;
        const bool double_on = (mask & il2cpp::PWR_COIN_DBL) != 0;
        const bool gum_on    = (mask & il2cpp::PWR_BUBBLE_GUM) != 0;

        static int s_magnet_last = -1;
        static int s_double_last = -1;
        static int s_gum_last    = -1;

        const int mv = magnet_on ? 1 : 0;
        const int dv = double_on ? 1 : 0;
        const int gv = gum_on    ? 1 : 0;

        if (mv != s_magnet_last) {
            il2cpp::set_magnet_forever(magnet_on);
            s_magnet_last = mv;
        }
        if (dv != s_double_last) {
            il2cpp::set_coin_doubler_forever(double_on);
            s_double_last = dv;
        }
        if (gv != s_gum_last) {
            il2cpp::set_shield_forever(gum_on);
            s_gum_last = gv;
        }
    }

    {
        const bool invincibility = ui::state::get_bool("run.invincibility", false);
        static int s_invincibility_last = -1;
        if (static_cast<int>(invincibility) != s_invincibility_last) {
            il2cpp::set_invincibility(invincibility);
            s_invincibility_last = invincibility ? 1 : 0;
        }
    }

    {
        const bool dj_ui = ui::state::get_bool("jump.double", false);
        const bool run_act = il2cpp::is_run_active();
        const double run_el = il2cpp::run_elapsed_seconds();
        bool dj_active = false;
        if (dj_ui && run_act && run_el > 8.0) {
            dj_active = true;
        }
        static int s_dj_last = -1;
        const int v = dj_active ? 1 : 0;
        if (v != s_dj_last) {
            il2cpp::set_double_jump(dj_active);
            s_dj_last = v;
        }
    }

    {
        const bool ar = ui::state::get_bool("run.auto_restart", false);
        static int s_ar_last = -1;
        if (static_cast<int>(ar) != s_ar_last) {
            il2cpp::set_auto_restart(ar);
            s_ar_last = ar ? 1 : 0;
        }
    }

    {
        const bool sd = ui::state::get_bool("death.skip_screen", false);
        static int s_sd_last = -1;
        if (static_cast<int>(sd) != s_sd_last) {
            il2cpp::set_skip_death_screen(sd);
            s_sd_last = sd ? 1 : 0;
        }
    }

    {
        static bool s_seed_hook_tried = false;
        if (!s_seed_hook_tried) {
            s_seed_hook_tried = true;
            il2cpp::install_seed_hook();
        }

        std::string polled;
        if (platform::android::poll_seed_input(polled)) {
            std::string trimmed;
            trimmed.reserve(polled.size());
            for (char ch : polled) {
                if (ch >= '0' && ch <= '9') trimmed.push_back(ch);
            }
            if (trimmed.size() > 10) trimmed.resize(10);
            ui::state::set_string("seed.value", trimmed);
        }

        const bool seed_on = ui::state::get_bool("seed.override", false);
        const std::string seed_str = ui::state::get_string("seed.value", "");

        std::uint32_t seed_val = 0;
        bool have_numeric = false;
        if (!seed_str.empty()) {
            std::uint64_t parsed = 0;
            for (char ch : seed_str) {
                if (ch < '0' || ch > '9') { have_numeric = false; parsed = 0; break; }
                parsed = parsed * 10 + static_cast<std::uint64_t>(ch - '0');
                if (parsed > 0xFFFFFFFFull) { parsed = 0; break; }
                have_numeric = true;
            }
            if (have_numeric) seed_val = static_cast<std::uint32_t>(parsed);
        }

        const bool effective_enable = seed_on && have_numeric;
        static int s_seed_last_enable = -1;
        static std::uint32_t s_seed_last_val = 0xFFFFFFFFu;
        const int enable_int = effective_enable ? 1 : 0;
        if (enable_int != s_seed_last_enable || seed_val != s_seed_last_val) {
            il2cpp::set_seed_override(effective_enable, seed_val);
            s_seed_last_enable = enable_int;
            s_seed_last_val = seed_val;
        }

        ui::state::set_int("seed.last_observed",
                           static_cast<int>(il2cpp::get_last_observed_seed()));
    }

    il2cpp::tick_death_skip();
}

void FeatureManager::shutdown() {
}

}
