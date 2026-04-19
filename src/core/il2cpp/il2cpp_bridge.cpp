#include "il2cpp_bridge.hpp"
#include "il2cpp_rva.hpp"
#include "../hooks/inline_hook.hpp"

#include <dlfcn.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <ctime>
#include <mutex>
#include <unordered_map>

namespace il2cpp {

namespace {

using namespace rva;

constexpr std::ptrdiff_t kPlayerInputPlayersFilterOff = 0x10;
constexpr std::ptrdiff_t kPlayerInputTryLanesPoolOff  = 0x20;
constexpr std::ptrdiff_t kEcsFilterDenseEntitiesOff   = 0x20;
constexpr std::ptrdiff_t kEcsFilterEntitiesCountOff   = 0x28;
constexpr std::ptrdiff_t kEcsPoolDenseItemsOff        = 0x30;
constexpr std::ptrdiff_t kEcsPoolSparseItemsOff       = 0x38;
constexpr std::ptrdiff_t kEcsPoolDenseCountOff        = 0x40;
constexpr std::ptrdiff_t kIl2CppArrayDataOff          = 0x20;

constexpr std::uint32_t kFmovS0FromS10 = 0x1e204140u;
constexpr std::uint32_t kFmovS0FromS0  = 0x1e204000u;
constexpr std::uint32_t kArm64Ret       = 0xd65f03c0u;
constexpr std::uint32_t kArm64MovW0One  = 0x52800020u;

using SetFloatFn      = void  (*)(float value, void* method);
using GetFloatFn      = float (*)(void* method);
using GetObjFn        = void* (*)(void* method);
using InstSetFloatFn  = void  (*)(void* thiz, float value, void* method);
using InstGetFloatFn  = float (*)(void* thiz, void* method);

std::atomic<std::uintptr_t> g_base{0};
std::atomic<bool> g_resolved{false};

SetFloatFn     g_time_set_time_scale = nullptr;
GetFloatFn     g_time_get_time_scale = nullptr;
GetObjFn       g_camera_get_main     = nullptr;
InstSetFloatFn g_camera_set_fov      = nullptr;
InstGetFloatFn g_camera_get_fov      = nullptr;

InstSetFloatFn g_orig_camera_set_fov = nullptr;
std::atomic<float> g_fov_override{-1.0f};
std::atomic<bool>  g_fov_hook_installed{false};

void hooked_camera_set_fov(void* thiz, float value, void* method) {
    const float ov = g_fov_override.load();
    const float use = (ov > 0.0f) ? ov : value;
    if (g_orig_camera_set_fov) g_orig_camera_set_fov(thiz, use, method);
}

std::atomic<bool>   g_run_active{false};
std::atomic<double> g_run_start_monotonic_sec{0.0};

static double monotonic_now_sec() {
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1e9;
}

using GameplayMultFn = float (*)(void* thiz, void* method);
GameplayMultFn g_orig_gameplay_multiplier = nullptr;
std::atomic<bool>  g_gameplay_mult_hook_installed{false};
std::atomic<void*> g_gameplay_this{nullptr};

std::atomic<float> g_snap_run_time{0.0f};
std::atomic<float> g_snap_run_distance{0.0f};
std::atomic<float> g_snap_player_speed{0.0f};

constexpr std::uintptr_t kGameplayRunTimeValue       = 0x04;
constexpr std::uintptr_t kGameplayRunDistanceValue   = 0x13c;
constexpr std::uintptr_t kGameplayCurrentPlayerSpeed = 0x144;
constexpr std::ptrdiff_t kGameplayCoinsValueOffset   = 0x28;

std::atomic<float>        g_score_mult{1.0f};
std::atomic<int>          g_coin_mult_permil{1000};
std::atomic<unsigned>     g_powerup_mask{0};
std::atomic<std::int32_t> g_coin_last_seen{-1};
std::atomic<void*>        g_coin_last_gameplay{nullptr};

float hooked_gameplay_get_multiplier(void* thiz, void* method) {
    float result = 1.0f;
    if (g_orig_gameplay_multiplier != nullptr) {
        result = g_orig_gameplay_multiplier(thiz, method);
    }
    if (thiz != nullptr) {
        auto* p = reinterpret_cast<std::uint8_t*>(thiz);
        const float rt   = *reinterpret_cast<float*>(p + kGameplayRunTimeValue);
        const float dist = *reinterpret_cast<float*>(p + kGameplayRunDistanceValue);
        const float spd  = *reinterpret_cast<float*>(p + kGameplayCurrentPlayerSpeed);
        const bool rt_ok   = std::isfinite(rt)   && rt   >= 0.0f && rt   < 1.0e6f;
        const bool dist_ok = std::isfinite(dist) && dist >= 0.0f && dist < 1.0e7f;
        const bool spd_ok  = std::isfinite(spd)  && spd  >= 0.0f && spd  < 500.0f;
        if (rt_ok && dist_ok && spd_ok && (rt > 0.0f || dist > 0.0f || spd > 0.0f)) {
            g_snap_run_time.store(rt);
            g_snap_run_distance.store(dist);
            g_snap_player_speed.store(spd);
            g_gameplay_this.store(thiz);
        }
        static std::atomic<float> s_prev_rt{0.0f};
        const float prev_rt = s_prev_rt.load();
        if (rt_ok) {
            if (prev_rt < 0.05f && rt > 0.01f && !g_run_active.load()) {
                g_run_start_monotonic_sec.store(monotonic_now_sec());
                g_run_active.store(true);
            }
            if (prev_rt > 0.05f && rt < 0.01f && g_run_active.load()) {
                g_run_active.store(false);
                g_run_start_monotonic_sec.store(0.0);
            }
            s_prev_rt.store(rt);
        }
        const int permil = g_coin_mult_permil.load();
        if (permil > 1000) {
            std::int32_t* coins_value = reinterpret_cast<std::int32_t*>(
                reinterpret_cast<char*>(thiz) + kGameplayCoinsValueOffset);
            const std::int32_t cur = *coins_value;
            void* prev_gp = g_coin_last_gameplay.load();
            std::int32_t last = g_coin_last_seen.load();
            if (prev_gp != thiz) {
                g_coin_last_gameplay.store(thiz);
                g_coin_last_seen.store(cur);
            } else if (cur >= 0 && cur < 10'000'000 && last >= 0) {
                const std::int32_t delta = cur - last;
                if (delta > 0 && delta < 500) {
                    const std::int64_t boosted =
                        (static_cast<std::int64_t>(delta) * permil) / 1000;
                    const std::int64_t bonus = boosted - delta;
                    if (bonus > 0) {
                        std::int64_t new_val = static_cast<std::int64_t>(cur) + bonus;
                        if (new_val > 2'000'000'000LL) new_val = 2'000'000'000LL;
                        *coins_value = static_cast<std::int32_t>(new_val);
                        g_coin_last_seen.store(static_cast<std::int32_t>(new_val));
                    } else {
                        g_coin_last_seen.store(cur);
                    }
                } else {
                    g_coin_last_seen.store(cur);
                }
            } else {
                g_coin_last_seen.store(cur);
            }
        }

        float smult = g_score_mult.load();
        if (smult < 0.1f) smult = 0.1f;
        if (smult > 20.0f) smult = 20.0f;
        result *= smult;
    }
    return result;
}

using SubwayMutateFn = void (*)(void* thiz, void* state, float dt, void* method);
SubwayMutateFn g_orig_subway_mutate = nullptr;
std::atomic<bool>  g_subway_mutate_hook_installed{false};
std::atomic<int>   g_camera_mode{0};
std::atomic<float> g_camera_distance_mult{1.0f};
std::atomic<bool>  g_mirror_mode{false};

struct CameraOffsets {
    float wox, woy, woz;
    float pox, poy, poz;
    float tox, toy, toz;
    float xlerp;
    float ylerp_g_lt;
    float ylerp_g_gt;
    float ylerp_jump;
    bool  always_y;
    bool  instant_snap;
    bool  captured;
};
CameraOffsets g_cam_orig{};

constexpr std::uintptr_t kCamWorldOffset      = 0x34;
constexpr std::uintptr_t kCamPositionOffset   = 0x44;
constexpr std::uintptr_t kCamTargetPosOffset  = 0x50;
constexpr std::uintptr_t kCamXLerp            = 0x5c;
constexpr std::uintptr_t kCamYLerpGroundedLT  = 0x64;
constexpr std::uintptr_t kCamYLerpGroundedGT  = 0x68;
constexpr std::uintptr_t kCamYLerpLtJumpH     = 0x6c;
constexpr std::uintptr_t kCamAlwaysUpdateY    = 0x71;
constexpr std::uintptr_t kCamInstantSnap      = 0x72;

inline float& cam_field(void* thiz, std::uintptr_t off) {
    return *reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(thiz) + off);
}

inline bool& cam_bool(void* thiz, std::uintptr_t off) {
    return *reinterpret_cast<bool*>(reinterpret_cast<std::uint8_t*>(thiz) + off);
}

void hooked_subway_mutate(void* thiz, void* state, float dt, void* method) {
    if (thiz != nullptr) {
        const int mode = g_camera_mode.load();
        const float dist = g_camera_distance_mult.load();
        const bool is_default = (mode == 0) && (std::fabs(dist - 1.0f) < 0.0005f);

        if (is_default) {
            if (g_orig_subway_mutate != nullptr) {
                g_orig_subway_mutate(thiz, state, dt, method);
            }
            return;
        }

        if (!g_cam_orig.captured) {
            g_cam_orig.wox = cam_field(thiz, kCamWorldOffset + 0);
            g_cam_orig.woy = cam_field(thiz, kCamWorldOffset + 4);
            g_cam_orig.woz = cam_field(thiz, kCamWorldOffset + 8);
            g_cam_orig.pox = cam_field(thiz, kCamPositionOffset + 0);
            g_cam_orig.poy = cam_field(thiz, kCamPositionOffset + 4);
            g_cam_orig.poz = cam_field(thiz, kCamPositionOffset + 8);
            g_cam_orig.tox = cam_field(thiz, kCamTargetPosOffset + 0);
            g_cam_orig.toy = cam_field(thiz, kCamTargetPosOffset + 4);
            g_cam_orig.toz = cam_field(thiz, kCamTargetPosOffset + 8);
            g_cam_orig.xlerp       = cam_field(thiz, kCamXLerp);
            g_cam_orig.ylerp_g_lt  = cam_field(thiz, kCamYLerpGroundedLT);
            g_cam_orig.ylerp_g_gt  = cam_field(thiz, kCamYLerpGroundedGT);
            g_cam_orig.ylerp_jump  = cam_field(thiz, kCamYLerpLtJumpH);
            g_cam_orig.always_y    = cam_bool(thiz, kCamAlwaysUpdateY);
            g_cam_orig.instant_snap= cam_bool(thiz, kCamInstantSnap);
            g_cam_orig.captured = true;
        }

        cam_field(thiz, kCamXLerp)           = g_cam_orig.xlerp;
        cam_field(thiz, kCamYLerpGroundedLT) = g_cam_orig.ylerp_g_lt;
        cam_field(thiz, kCamYLerpGroundedGT) = g_cam_orig.ylerp_g_gt;
        cam_field(thiz, kCamYLerpLtJumpH)    = g_cam_orig.ylerp_jump;
        cam_bool(thiz, kCamAlwaysUpdateY)    = g_cam_orig.always_y;
        cam_bool(thiz, kCamInstantSnap)      = g_cam_orig.instant_snap;

        switch (mode) {
            case 0: {
                cam_field(thiz, kCamPositionOffset + 0) = g_cam_orig.pox * dist;
                cam_field(thiz, kCamPositionOffset + 4) = g_cam_orig.poy * dist;
                cam_field(thiz, kCamPositionOffset + 8) = g_cam_orig.poz * dist;
                cam_field(thiz, kCamTargetPosOffset + 0) = g_cam_orig.tox;
                cam_field(thiz, kCamTargetPosOffset + 4) = g_cam_orig.toy;
                cam_field(thiz, kCamTargetPosOffset + 8) = g_cam_orig.toz;
                break;
            }
            case 1: {
                cam_field(thiz, kCamPositionOffset + 0) = 0.0f;
                cam_field(thiz, kCamPositionOffset + 4) = 2.4f;
                cam_field(thiz, kCamPositionOffset + 8) = 0.0f;
                cam_field(thiz, kCamTargetPosOffset + 0) = 0.0f;
                cam_field(thiz, kCamTargetPosOffset + 4) = 2.4f;
                cam_field(thiz, kCamTargetPosOffset + 8) = 8.0f;
                cam_field(thiz, kCamXLerp)           = 100.0f;
                cam_field(thiz, kCamYLerpGroundedLT) = 100.0f;
                cam_field(thiz, kCamYLerpGroundedGT) = 100.0f;
                cam_field(thiz, kCamYLerpLtJumpH)    = 100.0f;
                cam_bool(thiz, kCamAlwaysUpdateY)    = true;
                cam_bool(thiz, kCamInstantSnap)      = true;
                break;
            }
            case 2: {
                const float h = 18.0f * dist;
                cam_field(thiz, kCamPositionOffset + 0) = 0.0f;
                cam_field(thiz, kCamPositionOffset + 4) = h;
                cam_field(thiz, kCamPositionOffset + 8) = -2.0f;
                cam_field(thiz, kCamTargetPosOffset + 0) = 0.0f;
                cam_field(thiz, kCamTargetPosOffset + 4) = 0.0f;
                cam_field(thiz, kCamTargetPosOffset + 8) = 0.0f;
                cam_bool(thiz, kCamAlwaysUpdateY)    = true;
                break;
            }
            case 3: {
                cam_field(thiz, kCamPositionOffset + 0) = 4.0f * dist;
                cam_field(thiz, kCamPositionOffset + 4) = (g_cam_orig.poy + 1.5f) * dist;
                cam_field(thiz, kCamPositionOffset + 8) = (g_cam_orig.poz - 2.0f) * dist;
                cam_field(thiz, kCamTargetPosOffset + 0) = g_cam_orig.tox;
                cam_field(thiz, kCamTargetPosOffset + 4) = g_cam_orig.toy + 0.5f;
                cam_field(thiz, kCamTargetPosOffset + 8) = g_cam_orig.toz + 4.0f;
                break;
            }
            default: break;
        }
    }
    if (g_orig_subway_mutate != nullptr) {
        g_orig_subway_mutate(thiz, state, dt, method);
    }
}

using PlayerInputRunFn = void (*)(void* thiz, void* systems, void* method);
PlayerInputRunFn g_orig_player_input_run = nullptr;
std::atomic<bool> g_player_input_hook_installed{false};
std::atomic<int>  g_try_lane_flips{0};

void hooked_player_input_run(void* thiz, void* systems, void* method) {
    if (g_orig_player_input_run != nullptr) {
        g_orig_player_input_run(thiz, systems, method);
    }
    if (thiz == nullptr || !g_mirror_mode.load()) return;

    auto* sys_bytes = reinterpret_cast<std::uint8_t*>(thiz);
    void* filter = *reinterpret_cast<void**>(sys_bytes + kPlayerInputPlayersFilterOff);
    void* pool   = *reinterpret_cast<void**>(sys_bytes + kPlayerInputTryLanesPoolOff);
    if (filter == nullptr || pool == nullptr) return;

    auto* filter_bytes = reinterpret_cast<std::uint8_t*>(filter);
    void* dense_entities = *reinterpret_cast<void**>(filter_bytes + kEcsFilterDenseEntitiesOff);
    const int fcount     = *reinterpret_cast<int*>(filter_bytes + kEcsFilterEntitiesCountOff);
    if (dense_entities == nullptr || fcount <= 0) return;

    auto* pool_bytes = reinterpret_cast<std::uint8_t*>(pool);
    void* pool_dense_arr  = *reinterpret_cast<void**>(pool_bytes + kEcsPoolDenseItemsOff);
    void* pool_sparse_arr = *reinterpret_cast<void**>(pool_bytes + kEcsPoolSparseItemsOff);
    if (pool_dense_arr == nullptr || pool_sparse_arr == nullptr) return;

    auto* entity_ids = reinterpret_cast<int*>(
        reinterpret_cast<std::uint8_t*>(dense_entities) + kIl2CppArrayDataOff);
    auto* sparse = reinterpret_cast<int*>(
        reinterpret_cast<std::uint8_t*>(pool_sparse_arr) + kIl2CppArrayDataOff);
    auto* dense_data = reinterpret_cast<std::uint8_t*>(pool_dense_arr) + kIl2CppArrayDataOff;

    for (int i = 0; i < fcount; ++i) {
        const int entity = entity_ids[i];
        if (entity <= 0) continue;
        const int dense_idx = sparse[entity];
        if (dense_idx <= 0) continue;

        int* dir = reinterpret_cast<int*>(dense_data + static_cast<std::size_t>(dense_idx) * sizeof(int));
        if (*dir == 1) {
            *dir = -1;
            g_try_lane_flips.fetch_add(1);
        } else if (*dir == -1) {
            *dir = 1;
            g_try_lane_flips.fetch_add(1);
        }
    }
}

using VelocityHelperFn = float (*)(float height, float gravity);
VelocityHelperFn g_orig_velocity_helper = nullptr;
std::atomic<float> g_jump_gravity_mult{1.0f};
std::atomic<bool>  g_jump_gravity_hook_installed{false};

float hooked_velocity_helper(float height, float gravity) {
    if (g_orig_velocity_helper == nullptr) return 0.0f;
    const float mult = g_jump_gravity_mult.load();
    return g_orig_velocity_helper(height, gravity * mult);
}

using TransitionRollFn = void (*)(void* thiz, int entity, float duration, void* method);
TransitionRollFn g_orig_transition_roll = nullptr;
std::atomic<float> g_roll_duration_mult{1.0f};
std::atomic<bool>  g_roll_duration_hook_installed{false};

void hooked_transition_roll(void* thiz, int entity, float duration, void* method) {
    if (g_orig_transition_roll == nullptr) return;
    const float mult = g_roll_duration_mult.load();
    g_orig_transition_roll(thiz, entity, duration * mult, method);
}

using ShouldMagnetizeFn = int (*)(void* thiz, int entity,
                                  float tmx, float tmy, float tmz,
                                  float mpx, float mpy, float mpz,
                                  void* shape, int source, void* method);
ShouldMagnetizeFn g_orig_should_magnetize = nullptr;
std::atomic<bool> g_should_magnetize_hook_installed{false};

int hooked_should_magnetize(void* thiz, int entity,
                            float tmx, float tmy, float tmz,
                            float mpx, float mpy, float mpz,
                            void* shape, int source, void* method) {
    if (g_powerup_mask.load() & PWR_MAGNET) return 1;
    if (g_orig_should_magnetize == nullptr) return 0;
    return g_orig_should_magnetize(thiz, entity, tmx, tmy, tmz,
                                   mpx, mpy, mpz, shape, source, method);
}

using UpdatePowerupFn = void (*)(void* thiz, void* gpe, int powerup_state,
                                  float distance_change, void* method);
UpdatePowerupFn g_orig_update_powerup = nullptr;
std::atomic<bool> g_update_powerup_hook_installed{false};

void hooked_update_powerup(void* thiz, void* gpe, int powerup_state,
                           float distance_change, void* method) {
    if (g_orig_update_powerup != nullptr) {
        g_orig_update_powerup(thiz, gpe, powerup_state, distance_change, method);
    }
}

bool patch_arm64_instruction(std::uintptr_t addr, std::uint32_t new_opcode) {
    const long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) return false;
    void* page = reinterpret_cast<void*>(addr & ~static_cast<std::uintptr_t>(pagesize - 1));
    if (mprotect(page, static_cast<size_t>(pagesize),
                 PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return false;
    }
    *reinterpret_cast<volatile std::uint32_t*>(addr) = new_opcode;
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr + 4));
    mprotect(page, static_cast<size_t>(pagesize),
             PROT_READ | PROT_EXEC);
    return true;
}

int phdrCallback(dl_phdr_info* info, size_t, void* data) {
    if (info->dlpi_name == nullptr) return 0;
    if (std::strstr(info->dlpi_name, "libil2cpp.so") == nullptr) return 0;
    *static_cast<std::uintptr_t*>(data) = static_cast<std::uintptr_t>(info->dlpi_addr);
    return 1;
}

std::uintptr_t find_libil2cpp_base() {
    std::uintptr_t addr = 0;
    dl_iterate_phdr(phdrCallback, &addr);
    return addr;
}

}

bool resolve() {
    if (g_resolved.load()) return true;

    const std::uintptr_t base = find_libil2cpp_base();
    if (base == 0) return false;

    g_base.store(base);
    g_time_set_time_scale = reinterpret_cast<SetFloatFn>    (base + kTimeSetTimeScale);
    g_time_get_time_scale = reinterpret_cast<GetFloatFn>    (base + kTimeGetTimeScale);
    g_camera_get_main     = reinterpret_cast<GetObjFn>      (base + kCameraGetMain);
    g_camera_set_fov      = reinterpret_cast<InstSetFloatFn>(base + kCameraSetFieldOfView);
    g_camera_get_fov      = reinterpret_cast<InstGetFloatFn>(base + kCameraGetFieldOfView);

    g_resolved.store(true);

    return true;
}

bool is_resolved() {
    return g_resolved.load();
}

std::uintptr_t base_address() {
    return g_base.load();
}

void set_time_scale(float value) {
    if (!g_resolved.load() && !resolve()) return;
    if (g_time_set_time_scale) g_time_set_time_scale(value, nullptr);
}

float get_time_scale() {
    if (!g_resolved.load() && !resolve()) return 0.0f;
    if (!g_time_get_time_scale) return 0.0f;
    return g_time_get_time_scale(nullptr);
}

void* get_main_camera() {
    if (!g_resolved.load() && !resolve()) return nullptr;
    if (!g_camera_get_main) return nullptr;
    return g_camera_get_main(nullptr);
}

void camera_set_fov(void* camera, float fov) {
    if (camera == nullptr || g_camera_set_fov == nullptr) return;
    g_camera_set_fov(camera, fov, nullptr);
}

float camera_get_fov(void* camera) {
    if (camera == nullptr || g_camera_get_fov == nullptr) return 0.0f;
    return g_camera_get_fov(camera, nullptr);
}

void set_fov_override(float value) {
    g_fov_override.store(value);
}

void clear_fov_override() {
    g_fov_override.store(-1.0f);
}

bool install_fov_hook() {
    if (g_fov_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    if (g_camera_set_fov == nullptr) return false;

    void* tramp = inline_hook::install(
        reinterpret_cast<void*>(g_camera_set_fov),
        reinterpret_cast<void*>(&hooked_camera_set_fov));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_camera_set_fov = reinterpret_cast<InstSetFloatFn>(tramp);
    g_fov_hook_installed.store(true);
    return true;
}

void set_camera_mode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 3) mode = 3;
    g_camera_mode.store(mode);
}

void set_mirror_mode(bool enable) {
    g_mirror_mode.store(enable);
}

void set_camera_distance_mult(float mult) {
    if (mult < 0.1f) mult = 0.1f;
    if (mult > 5.0f) mult = 5.0f;
    g_camera_distance_mult.store(mult);
}

bool install_camera_follow_hook() {
    if (g_subway_mutate_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kSubwayFollowCameraMutateCameraState);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_subway_mutate));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_subway_mutate = reinterpret_cast<SubwayMutateFn>(tramp);
    g_subway_mutate_hook_installed.store(true);
    return true;
}

bool install_player_input_hook() {
    if (g_player_input_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kPlayerInputSystemRun);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_player_input_run));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_player_input_run = reinterpret_cast<PlayerInputRunFn>(tramp);
    g_player_input_hook_installed.store(true);
    return true;
}

bool install_gameplay_mult_hook() {
    if (g_gameplay_mult_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kGameplayGetMultiplier);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_gameplay_get_multiplier));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_gameplay_multiplier = reinterpret_cast<GameplayMultFn>(tramp);
    g_gameplay_mult_hook_installed.store(true);
    return true;
}

float get_current_player_speed() {
    return g_snap_player_speed.load();
}

float get_current_run_distance() {
    return g_snap_run_distance.load();
}

void set_jump_gravity_mult(float mult) {
    g_jump_gravity_mult.store(mult);
}

bool install_jump_gravity_hook() {
    if (g_jump_gravity_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kPlayerMovementVelocityHelper);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_velocity_helper));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_velocity_helper = reinterpret_cast<VelocityHelperFn>(tramp);
    g_jump_gravity_hook_installed.store(true);
    return true;
}

void set_roll_duration_mult(float mult) {
    g_roll_duration_mult.store(mult);
}

bool install_roll_duration_hook() {
    if (g_roll_duration_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kPlayerInteractionTransitionRoll);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_transition_roll));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_transition_roll = reinterpret_cast<TransitionRollFn>(tramp);
    g_roll_duration_hook_installed.store(true);
    return true;
}

void set_score_mult(float mult) {
    g_score_mult.store(mult);
}

bool install_score_hook() {
    return install_gameplay_mult_hook();
}

void set_coin_pickup_multiplier(float mult) {
    if (mult < 1.0f)  mult = 1.0f;
    if (mult > 20.0f) mult = 20.0f;
    const int permil = static_cast<int>(std::lround(mult * 1000.0f));
    g_coin_mult_permil.store(permil);
}

void set_powerup_forever_mask(unsigned mask) {
    g_powerup_mask.store(mask & 0x7Fu);
}

bool install_should_magnetize_hook() {
    if (g_should_magnetize_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kMagnetizationHelperShouldMagnetize);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_should_magnetize));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_should_magnetize = reinterpret_cast<ShouldMagnetizeFn>(tramp);
    g_should_magnetize_hook_installed.store(true);
    return true;
}

bool install_update_powerup_hook() {
    if (g_update_powerup_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kGameplayStateSystemUpdatePowerup);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_update_powerup));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_update_powerup = reinterpret_cast<UpdatePowerupFn>(tramp);
    g_update_powerup_hook_installed.store(true);
    return true;
}

struct PatchSlot {
    std::uint32_t original = 0;
    bool captured = false;
    bool applied = false;
};

std::mutex g_patch_mutex;
std::unordered_map<std::uintptr_t, PatchSlot> g_patch_slots;

bool apply_patch_locked(std::uintptr_t rva, std::uint32_t new_opcode) {
    if (!g_resolved.load() && !resolve()) return false;
    const std::uintptr_t addr = g_base.load() + rva;

    auto& slot = g_patch_slots[rva];
    if (!slot.captured) {
        slot.original = *reinterpret_cast<volatile std::uint32_t*>(addr);
        slot.captured = true;
    }
    if (slot.applied) return true;
    if (!patch_arm64_instruction(addr, new_opcode)) {
        return false;
    }
    slot.applied = true;
    return true;
}

bool revert_patch_locked(std::uintptr_t rva) {
    if (!g_resolved.load() && !resolve()) return false;
    auto it = g_patch_slots.find(rva);
    if (it == g_patch_slots.end() || !it->second.captured) return true;
    if (!it->second.applied) return true;
    const std::uintptr_t addr = g_base.load() + rva;
    if (!patch_arm64_instruction(addr, it->second.original)) {
        return false;
    }
    it->second.applied = false;
    return true;
}

void set_patch(std::uintptr_t rva, std::uint32_t new_opcode, bool enable) {
    std::lock_guard<std::mutex> lock(g_patch_mutex);
    if (enable) {
        apply_patch_locked(rva, new_opcode);
    } else {
        revert_patch_locked(rva);
    }
}

void set_magnet_forever(bool enable) {
    set_patch(kMagnetTimeFsub, kFmovS0FromS10, enable);
}

void set_shield_forever(bool enable) {
    set_patch(kShieldTimeFsub, kFmovS0FromS0, enable);
}

void set_coin_doubler_forever(bool enable) {
    set_patch(kCoinDoublerTimeFsub, kFmovS0FromS0, enable);
}

void set_double_jump(bool enable) {
    set_patch(kCanJump,     kArm64MovW0One, enable);
    set_patch(kCanJump + 4, kArm64Ret,      enable);
}

namespace {

using DyingFn = void (*)(void* thiz, std::int32_t entity,
                         void* hit, bool recoverable,
                         std::int32_t reason, void* method);
using VoidThisFn = void (*)(void* thiz, void* method);
using EcsRunFn = void (*)(void* thiz, void* systems, void* method);
using TryBuildInfFn = bool (*)(void* thiz, void* ctx, void* desc, void* output, void* method);

DyingFn       g_orig_force_dying            = nullptr;
VoidThisFn    g_orig_main_focus             = nullptr;
VoidThisFn    g_orig_revive_bind            = nullptr;
EcsRunFn      g_orig_revive_system_run      = nullptr;
TryBuildInfFn g_orig_try_build_infinite     = nullptr;
VoidThisFn    g_main_start_run_fn           = nullptr;
VoidThisFn    g_revive_on_continue_fn       = nullptr;
VoidThisFn    g_revive_transition_over_fn   = nullptr;

VoidThisFn    g_orig_runover_bind           = nullptr;
VoidThisFn    g_orig_runover_focus          = nullptr;
VoidThisFn    g_orig_runover_refresh        = nullptr;
VoidThisFn    g_orig_runover_on_continue    = nullptr;
EcsRunFn      g_orig_runover_system_init    = nullptr;
VoidThisFn    g_orig_main_on_start_run      = nullptr;
EcsRunFn      g_orig_finite_home_flow_run   = nullptr;
EcsRunFn      g_orig_finite_runover_run     = nullptr;
VoidThisFn    g_orig_main_refresh           = nullptr;

std::atomic<void*> g_main_presenter_this{nullptr};
std::atomic<void*> g_revive_sub_this{nullptr};
std::atomic<bool>  g_invincibility_enabled{false};
std::atomic<bool>  g_skip_death_enabled{false};
std::atomic<bool>  g_auto_restart_enabled{false};
std::atomic<bool>  g_auto_start_pending{false};
std::atomic<int>   g_revive_skip_delay_ticks{-1};
std::atomic<bool>  g_force_dying_hook_installed{false};
std::atomic<bool>  g_main_focus_hook_installed{false};
std::atomic<bool>  g_revive_bind_hook_installed{false};
std::atomic<bool>  g_revive_system_run_hook_installed{false};
std::atomic<int>   g_revive_system_run_count{0};
std::atomic<bool>  g_try_build_infinite_hook_installed{false};
std::atomic<bool>  g_seed_override_enabled{false};
std::atomic<std::uint32_t> g_seed_override_value{0};
std::atomic<std::uint32_t> g_seed_last_observed{0};

std::atomic<void*> g_runover_this{nullptr};
std::atomic<int>   g_runover_continue_delay_ticks{-1};
std::atomic<int>   g_runover_refresh_count{0};
std::atomic<bool>  g_runover_bind_hook_installed{false};
std::atomic<bool>  g_runover_focus_hook_installed{false};
std::atomic<bool>  g_runover_refresh_hook_installed{false};
std::atomic<bool>  g_runover_on_continue_hook_installed{false};
std::atomic<bool>  g_runover_system_init_hook_installed{false};
std::atomic<bool>  g_main_on_start_run_hook_installed{false};
std::atomic<bool>  g_finite_home_flow_hook_installed{false};
std::atomic<bool>  g_finite_runover_run_hook_installed{false};
std::atomic<int>   g_finite_home_flow_count{0};
std::atomic<int>   g_finite_runover_run_count{0};
std::atomic<int>   g_main_start_run_delay_ticks{-1};
std::atomic<bool>  g_main_refresh_hook_installed{false};
std::atomic<bool>  g_main_start_run_pending{false};
std::atomic<int>   g_main_refresh_count_since_focus{0};

constexpr int kReviveSkipDelayTicks = 1;
constexpr int kRunOverContinueDelayTicks = 1;
constexpr int kMainStartRunDelayTicks = 12;
constexpr int kMainRefreshFireOn = 2;
constexpr int kMainRefreshFallbackTicks = 30;

void hooked_force_dying(void* thiz, std::int32_t entity,
                        void* hit, bool recoverable,
                        std::int32_t reason, void* method) {
    const bool invincibility = g_invincibility_enabled.load();
    const bool ar    = g_auto_restart_enabled.load();
    if (invincibility) {
        return;
    }
    g_run_active.store(false);
    g_run_start_monotonic_sec.store(0.0);
    g_snap_run_time.store(0.0f);
    g_snap_run_distance.store(0.0f);
    g_snap_player_speed.store(0.0f);
    set_patch(kCanJump,     kArm64MovW0One, false);
    set_patch(kCanJump + 4, kArm64Ret,      false);
    if (ar) {
        g_auto_start_pending.store(true);
    }
    if (g_orig_force_dying != nullptr) {
        g_orig_force_dying(thiz, entity, hit, recoverable, reason, method);
    }
}

void try_fire_main_start_run_now(const char* tag) {
    if (!g_main_start_run_pending.exchange(false)) return;
    g_main_start_run_delay_ticks.store(-1);
    g_auto_start_pending.store(false);
    g_main_refresh_count_since_focus.store(0);

    void* main_this = g_main_presenter_this.load();
    if (main_this == nullptr) {
        return;
    }
    if (g_orig_main_on_start_run != nullptr) {
        g_orig_main_on_start_run(main_this, nullptr);
        return;
    }
    if (g_main_start_run_fn != nullptr) {
        g_main_start_run_fn(main_this, nullptr);
        return;
    }
}

void hooked_main_focus(void* thiz, void* method) {
    const bool pending = g_auto_start_pending.load();
    g_main_presenter_this.store(thiz);
    if (g_orig_main_focus != nullptr) {
        g_orig_main_focus(thiz, method);
    }
    if (pending && g_auto_restart_enabled.load() && thiz != nullptr) {
        g_main_start_run_pending.store(true);
        g_main_refresh_count_since_focus.store(0);
        g_main_start_run_delay_ticks.store(kMainRefreshFallbackTicks);
    }
}

void hooked_main_refresh(void* thiz, void* method) {
    if (g_orig_main_refresh != nullptr) {
        g_orig_main_refresh(thiz, method);
    }
    if (!g_main_start_run_pending.load()) return;

    const int n = g_main_refresh_count_since_focus.fetch_add(1) + 1;
    if (n >= kMainRefreshFireOn) {
        try_fire_main_start_run_now("main_refresh");
    }
}

void hooked_revive_bind(void* thiz, void* method) {
    const bool sd = g_skip_death_enabled.load();
    const bool ar = g_auto_restart_enabled.load();
    g_revive_sub_this.store(thiz);
    g_revive_system_run_count.store(0);
    if (g_orig_revive_bind != nullptr) {
        g_orig_revive_bind(thiz, method);
    }
    if (sd || ar) {
        g_revive_skip_delay_ticks.store(kReviveSkipDelayTicks);
    }
}

void hooked_revive_system_run(void* thiz, void* systems, void* method) {
    g_revive_system_run_count.fetch_add(1);

    if (g_orig_revive_system_run != nullptr) {
        g_orig_revive_system_run(thiz, systems, method);
    }

    const int t = g_revive_skip_delay_ticks.load();
    if (t < 0) return;

    if (!(g_skip_death_enabled.load() || g_auto_restart_enabled.load())) {
        g_revive_skip_delay_ticks.store(-1);
        return;
    }

    if (t > 0) {
        g_revive_skip_delay_ticks.store(t - 1);
        return;
    }

    g_revive_skip_delay_ticks.store(-1);

    void* sub_this = g_revive_sub_this.load();
    if (sub_this == nullptr) {
        return;
    }
    if (g_revive_on_continue_fn == nullptr) {
        return;
    }
    g_revive_on_continue_fn(sub_this, nullptr);
}

bool install_force_dying_hook_locked() {
    if (g_force_dying_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kForceTransitionToDying);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_force_dying));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_force_dying = reinterpret_cast<DyingFn>(tramp);
    g_force_dying_hook_installed.store(true);
    return true;
}

bool install_main_focus_hook_locked() {
    if (g_main_focus_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kScreenMainPresenterFocus);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_main_focus));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_main_focus = reinterpret_cast<VoidThisFn>(tramp);
    g_main_start_run_fn = reinterpret_cast<VoidThisFn>(g_base.load() + kScreenMainPresenterStartRun);
    g_main_focus_hook_installed.store(true);
    return true;
}

bool install_revive_bind_hook_locked() {
    if (g_revive_bind_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kReviveSubPresenterBind);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_revive_bind));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_revive_bind = reinterpret_cast<VoidThisFn>(tramp);
    g_revive_on_continue_fn = reinterpret_cast<VoidThisFn>(g_base.load() + kReviveSubPresenterOnContinue);
    g_revive_transition_over_fn = reinterpret_cast<VoidThisFn>(g_base.load() + kReviveSubPresenterTransitionRunOver);
    g_revive_bind_hook_installed.store(true);
    return true;
}

bool install_revive_system_run_hook_locked() {
    if (g_revive_system_run_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kScreenReviveSystemRun);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_revive_system_run));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_revive_system_run = reinterpret_cast<EcsRunFn>(tramp);
    g_revive_system_run_hook_installed.store(true);
    return true;
}

void hooked_runover_bind(void* thiz, void* method) {
    g_run_active.store(false);
    g_run_start_monotonic_sec.store(0.0);
    g_snap_run_time.store(0.0f);
    g_snap_run_distance.store(0.0f);
    g_snap_player_speed.store(0.0f);
    g_gameplay_this.store(nullptr);
    g_runover_this.store(thiz);
    g_runover_refresh_count.store(0);
    if (g_orig_runover_bind != nullptr) {
        g_orig_runover_bind(thiz, method);
    }
    if (g_auto_restart_enabled.load() || g_skip_death_enabled.load()) {
        g_runover_continue_delay_ticks.store(kRunOverContinueDelayTicks);
    }
}

void hooked_runover_focus(void* thiz, void* method) {
    g_runover_this.store(thiz);
    if (g_orig_runover_focus != nullptr) {
        g_orig_runover_focus(thiz, method);
    }
    if (g_auto_restart_enabled.load()) {
        g_runover_continue_delay_ticks.store(kRunOverContinueDelayTicks);
    }
}

void hooked_runover_refresh(void* thiz, void* method) {
    g_runover_refresh_count.fetch_add(1);
    if (g_orig_runover_refresh != nullptr) {
        g_orig_runover_refresh(thiz, method);
    }

    int t = g_runover_continue_delay_ticks.load();
    if (t < 0) return;
    if (!g_auto_restart_enabled.load()) {
        g_runover_continue_delay_ticks.store(-1);
        return;
    }
    if (t > 0) {
        g_runover_continue_delay_ticks.store(t - 1);
        return;
    }
    g_runover_continue_delay_ticks.store(-1);

    void* sub_this = g_runover_this.load();
    if (sub_this == nullptr || g_orig_runover_on_continue == nullptr) {
        return;
    }
    g_orig_runover_on_continue(sub_this, nullptr);
}

void hooked_runover_on_continue(void* thiz, void* method) {
    if (g_orig_runover_on_continue != nullptr) {
        g_orig_runover_on_continue(thiz, method);
    }
}

void hooked_runover_system_init(void* thiz, void* systems, void* method) {
    if (g_orig_runover_system_init != nullptr) {
        g_orig_runover_system_init(thiz, systems, method);
    }
}

void hooked_main_on_start_run(void* thiz, void* method) {
    g_run_start_monotonic_sec.store(monotonic_now_sec());
    g_run_active.store(true);
    set_patch(kCanJump,     kArm64MovW0One, false);
    set_patch(kCanJump + 4, kArm64Ret,      false);
    if (g_orig_main_on_start_run != nullptr) {
        g_orig_main_on_start_run(thiz, method);
    }
}

bool install_runover_bind_hook_locked() {
    if (g_runover_bind_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kScreenRunOverPresenterBind);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_runover_bind));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_runover_bind = reinterpret_cast<VoidThisFn>(tramp);
    g_runover_bind_hook_installed.store(true);
    return true;
}

bool install_runover_focus_hook_locked() {
    if (g_runover_focus_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kScreenRunOverPresenterFocus);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_runover_focus));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_runover_focus = reinterpret_cast<VoidThisFn>(tramp);
    g_runover_focus_hook_installed.store(true);
    return true;
}

bool install_runover_refresh_hook_locked() {
    if (g_runover_refresh_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kScreenRunOverPresenterRefresh);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_runover_refresh));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_runover_refresh = reinterpret_cast<VoidThisFn>(tramp);
    g_runover_refresh_hook_installed.store(true);
    return true;
}

bool install_runover_on_continue_resolve_locked() {
    if (g_orig_runover_on_continue != nullptr) return true;
    if (!g_resolved.load() && !resolve()) return false;
    g_orig_runover_on_continue = reinterpret_cast<VoidThisFn>(
        g_base.load() + kScreenRunOverPresenterOnContinue);
    return true;
}

bool install_runover_system_init_hook_locked() {
    if (g_runover_system_init_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kScreenRunOverSystemInit);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_runover_system_init));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_runover_system_init = reinterpret_cast<EcsRunFn>(tramp);
    g_runover_system_init_hook_installed.store(true);
    return true;
}

bool install_main_on_start_run_hook_locked() {
    if (g_main_on_start_run_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kScreenMainPresenterOnStartRun);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_main_on_start_run));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_main_on_start_run = reinterpret_cast<VoidThisFn>(tramp);
    g_main_on_start_run_hook_installed.store(true);
    return true;
}

bool install_main_refresh_hook_locked() {
    if (g_main_refresh_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kScreenMainPresenterRefresh);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_main_refresh));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_main_refresh = reinterpret_cast<VoidThisFn>(tramp);
    g_main_refresh_hook_installed.store(true);
    return true;
}

void try_fire_runover_continue(const char* tag) {
    int t = g_runover_continue_delay_ticks.load();
    if (t < 0) return;
    if (!g_auto_restart_enabled.load() && !g_skip_death_enabled.load()) {
        g_runover_continue_delay_ticks.store(-1);
        return;
    }
    if (t > 0) {
        g_runover_continue_delay_ticks.store(t - 1);
        return;
    }
    g_runover_continue_delay_ticks.store(-1);

    void* sub_this = g_runover_this.load();
    if (sub_this == nullptr || g_orig_runover_on_continue == nullptr) {
        return;
    }
    g_orig_runover_on_continue(sub_this, nullptr);
}

void try_fire_main_start_run(const char* tag) {
    int t = g_main_start_run_delay_ticks.load();
    if (t < 0) return;
    if (!g_auto_restart_enabled.load()) {
        g_main_start_run_delay_ticks.store(-1);
        g_main_start_run_pending.store(false);
        return;
    }
    if (t > 0) {
        g_main_start_run_delay_ticks.store(t - 1);
        return;
    }
    try_fire_main_start_run_now(tag);
}

void hooked_finite_home_flow_run(void* thiz, void* systems, void* method) {
    g_finite_home_flow_count.fetch_add(1);
    if (g_orig_finite_home_flow_run != nullptr) {
        g_orig_finite_home_flow_run(thiz, systems, method);
    }
    try_fire_runover_continue("home_flow_run");
    try_fire_main_start_run("home_flow_run");
}

void hooked_finite_runover_run(void* thiz, void* systems, void* method) {
    g_finite_runover_run_count.fetch_add(1);
    if (g_orig_finite_runover_run != nullptr) {
        g_orig_finite_runover_run(thiz, systems, method);
    }
    try_fire_runover_continue("finite_runover_run");
    try_fire_main_start_run("finite_runover_run");
}

bool install_finite_home_flow_hook_locked() {
    if (g_finite_home_flow_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kFiniteRunHomeScreenFlowSystemRun);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_finite_home_flow_run));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_finite_home_flow_run = reinterpret_cast<EcsRunFn>(tramp);
    g_finite_home_flow_hook_installed.store(true);
    return true;
}

bool install_finite_runover_run_hook_locked() {
    if (g_finite_runover_run_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;
    void* target = reinterpret_cast<void*>(g_base.load() + kFiniteRunOverSystemRun);
    void* tramp = inline_hook::install(target, reinterpret_cast<void*>(&hooked_finite_runover_run));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_finite_runover_run = reinterpret_cast<EcsRunFn>(tramp);
    g_finite_runover_run_hook_installed.store(true);
    return true;
}

bool hooked_try_build_infinite_scheduler(void* thiz, void* ctx, void* desc,
                                         void* output, void* method) {
    if (g_orig_try_build_infinite == nullptr) return false;

    if (desc == nullptr) {
        return g_orig_try_build_infinite(thiz, ctx, desc, output, method);
    }

    auto* bytes = reinterpret_cast<std::uint8_t*>(desc);
    const std::uint32_t orig_seed = *reinterpret_cast<std::uint32_t*>(bytes);
    g_seed_last_observed.store(orig_seed);

    const bool override_on = g_seed_override_enabled.load();
    const std::uint32_t override_val = g_seed_override_value.load();

    if (!override_on) {
        return g_orig_try_build_infinite(thiz, ctx, desc, output, method);
    }

    std::uint8_t safe_copy[64];
    std::memcpy(safe_copy, desc, sizeof(safe_copy));
    *reinterpret_cast<std::uint32_t*>(safe_copy) = override_val;
    bool result = g_orig_try_build_infinite(thiz, ctx, safe_copy, output, method);
    return result;
}

bool install_try_build_infinite_hook_locked() {
    if (g_try_build_infinite_hook_installed.load()) return true;
    if (!g_resolved.load() && !resolve()) return false;

    void* target = reinterpret_cast<void*>(g_base.load() + kSchedulersProviderTryBuildInfinite);
    void* tramp = inline_hook::install(
        target, reinterpret_cast<void*>(&hooked_try_build_infinite_scheduler));
    if (tramp == nullptr) {
        return false;
    }
    g_orig_try_build_infinite = reinterpret_cast<TryBuildInfFn>(tramp);
    g_try_build_infinite_hook_installed.store(true);
    return true;
}

}

void set_invincibility(bool enable) {
    install_force_dying_hook_locked();
    g_invincibility_enabled.store(enable);
}

void set_skip_death_screen(bool enable) {
    install_force_dying_hook_locked();
    install_revive_bind_hook_locked();
    install_revive_system_run_hook_locked();
    install_runover_bind_hook_locked();
    install_runover_on_continue_resolve_locked();
    install_runover_system_init_hook_locked();
    install_finite_home_flow_hook_locked();
    install_finite_runover_run_hook_locked();
    g_skip_death_enabled.store(enable);
    if (!enable) {
        g_revive_skip_delay_ticks.store(-1);
        g_runover_continue_delay_ticks.store(-1);
    }
}

void set_auto_restart(bool enable) {
    install_force_dying_hook_locked();
    install_revive_bind_hook_locked();
    install_revive_system_run_hook_locked();
    install_main_focus_hook_locked();
    install_main_refresh_hook_locked();
    install_runover_bind_hook_locked();
    install_runover_on_continue_resolve_locked();
    install_runover_system_init_hook_locked();
    install_main_on_start_run_hook_locked();
    install_finite_home_flow_hook_locked();
    install_finite_runover_run_hook_locked();
    g_auto_restart_enabled.store(enable);
    if (!enable) {
        g_auto_start_pending.store(false);
        g_revive_skip_delay_ticks.store(-1);
        g_runover_continue_delay_ticks.store(-1);
        g_main_start_run_pending.store(false);
        g_main_start_run_delay_ticks.store(-1);
    }
}

void tick_death_skip() {
}

void set_seed_override(bool enable, std::uint32_t value) {
    install_try_build_infinite_hook_locked();
    g_seed_override_value.store(value);
    g_seed_override_enabled.store(enable);
}

std::uint32_t get_last_observed_seed() {
    return g_seed_last_observed.load();
}

std::uint32_t get_effective_seed() {
    if (g_seed_override_enabled.load()) {
        const std::uint32_t v = g_seed_override_value.load();
        if (v != 0) return v;
    }
    return g_seed_last_observed.load();
}

bool is_run_active() {
    return g_run_active.load();
}

float get_run_time_seconds() {
    if (!g_run_active.load()) return 0.0f;
    const double el = monotonic_now_sec() - g_run_start_monotonic_sec.load();
    if (el < 0.0) return 0.0f;
    return static_cast<float>(el);
}

double run_elapsed_seconds() {
    if (!g_run_active.load()) return 0.0;
    return monotonic_now_sec() - g_run_start_monotonic_sec.load();
}

bool install_run_lifecycle_hooks() {
    bool ok = true;
    if (!install_force_dying_hook_locked())       ok = false;
    if (!install_main_on_start_run_hook_locked()) ok = false;
    if (!install_runover_bind_hook_locked())      ok = false;
    if (!install_gameplay_mult_hook())            ok = false;
    return ok;
}

bool install_seed_hook() {
    return install_try_build_infinite_hook_locked();
}

}
