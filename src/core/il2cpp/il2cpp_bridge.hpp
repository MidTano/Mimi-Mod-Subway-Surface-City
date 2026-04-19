#pragma once

#include <cstdint>

namespace il2cpp {

bool resolve();
bool is_resolved();
std::uintptr_t base_address();

void set_time_scale(float value);
float get_time_scale();

void* get_main_camera();
void camera_set_fov(void* camera, float fov);
float camera_get_fov(void* camera);

void set_fov_override(float value);
void clear_fov_override();
bool install_fov_hook();

void set_camera_mode(int mode);
void set_camera_distance_mult(float mult);
void set_mirror_mode(bool enable);
bool install_camera_follow_hook();
bool install_player_input_hook();

void set_jump_gravity_mult(float mult);
bool install_jump_gravity_hook();

void set_roll_duration_mult(float mult);
bool install_roll_duration_hook();

void set_score_mult(float mult);
bool install_score_hook();

void set_coin_pickup_multiplier(float mult);

enum PowerupBit : unsigned {
    PWR_MAGNET      = 1u << 1,
    PWR_COIN_DBL    = 1u << 3,
    PWR_BUBBLE_GUM  = 1u << 5,
};

void set_powerup_forever_mask(unsigned mask);
bool install_should_magnetize_hook();
bool install_update_powerup_hook();

void set_magnet_forever(bool enable);
void set_shield_forever(bool enable);
void set_coin_doubler_forever(bool enable);

void set_invincibility(bool enable);
void set_double_jump(bool enable);
void set_auto_restart(bool enable);
void set_skip_death_screen(bool enable);

void tick_death_skip();

void set_seed_override(bool enable, std::uint32_t value);
std::uint32_t get_last_observed_seed();
std::uint32_t get_effective_seed();
bool install_seed_hook();

bool   is_run_active();
float  get_run_time_seconds();
double run_elapsed_seconds();
float get_current_player_speed();
float get_current_run_distance();
bool  install_run_lifecycle_hooks();
bool  install_gameplay_mult_hook();

}
