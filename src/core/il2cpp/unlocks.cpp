#include "unlocks.hpp"
#include "il2cpp_bridge.hpp"
#include "il2cpp_rva.hpp"
#include "../hooks/inline_hook.hpp"

#include <atomic>
#include <cstdint>

namespace il2cpp::unlocks {

namespace {

using IsUnlockedFn       = bool (*)(void* thiz, int datatag_id, void* method);
using SetUnlockedFn      = void (*)(void* thiz, int datatag_id, bool unlocked, void* method);
using UnlockSkinFn       = void (*)(void* thiz, int datatag_id, bool show_as_unseen, void* method);
using GenericIsUnlockedFn = bool (*)(void* thiz, int datatag_id, void* method);

IsUnlockedFn        g_orig_surfer_is_unlocked  = nullptr;
IsUnlockedFn        g_orig_board_is_unlocked   = nullptr;
GenericIsUnlockedFn g_orig_unlock_is_unlocked  = nullptr;
SetUnlockedFn       g_surfer_set_unlocked      = nullptr;
SetUnlockedFn       g_board_set_unlocked       = nullptr;
UnlockSkinFn        g_skin_unlock              = nullptr;
void*               g_skin_service_instance    = nullptr;

std::atomic<bool> g_unlock_all_surfers{false};
std::atomic<bool> g_unlock_all_boards{false};
std::atomic<bool> g_unlock_all_outfits{false};
std::atomic<bool> g_hooks_installed{false};

std::atomic<int>  g_persist_surfers{0};
std::atomic<int>  g_persist_boards{0};
std::atomic<int>  g_persist_outfits{0};

bool hooked_surfer_is_unlocked(void* thiz, int datatag_id, void* method) {
    if (g_orig_surfer_is_unlocked == nullptr) {
        return g_unlock_all_surfers.load();
    }
    const bool is_locked_orig = !g_orig_surfer_is_unlocked(thiz, datatag_id, method);
    if (g_unlock_all_surfers.load()) {
        if (is_locked_orig && g_surfer_set_unlocked != nullptr && thiz != nullptr) {
            g_surfer_set_unlocked(thiz, datatag_id, true, nullptr);
            g_persist_surfers.fetch_add(1);
        }
        return true;
    }
    return !is_locked_orig;
}

bool hooked_board_is_unlocked(void* thiz, int datatag_id, void* method) {
    if (g_orig_board_is_unlocked == nullptr) {
        return g_unlock_all_boards.load();
    }
    const bool is_locked_orig = !g_orig_board_is_unlocked(thiz, datatag_id, method);
    if (g_unlock_all_boards.load()) {
        if (is_locked_orig && g_board_set_unlocked != nullptr && thiz != nullptr) {
            g_board_set_unlocked(thiz, datatag_id, true, nullptr);
            g_persist_boards.fetch_add(1);
        }
        return true;
    }
    return !is_locked_orig;
}

bool hooked_unlock_is_unlocked(void* thiz, int datatag_id, void* method) {
    if (g_orig_unlock_is_unlocked == nullptr) {
        return g_unlock_all_outfits.load();
    }
    const bool orig_unlocked = g_orig_unlock_is_unlocked(thiz, datatag_id, method);
    if (g_unlock_all_outfits.load() && !orig_unlocked) {
        g_persist_outfits.fetch_add(1);
        return true;
    }
    return orig_unlocked;
}

GenericIsUnlockedFn g_orig_avail_skin_unlocked = nullptr;
GenericIsUnlockedFn g_orig_upgradable_unlocked = nullptr;

bool hooked_avail_skin_unlocked(void* thiz, int datatag_id, void* method) {
    if (g_unlock_all_outfits.load()) return true;
    if (g_orig_avail_skin_unlocked == nullptr) return false;
    return g_orig_avail_skin_unlocked(thiz, datatag_id, method);
}

bool hooked_upgradable_unlocked(void* thiz, int datatag_id, void* method) {
    if (g_unlock_all_outfits.load()) return true;
    if (g_orig_upgradable_unlocked == nullptr) return false;
    return g_orig_upgradable_unlocked(thiz, datatag_id, method);
}

using AdShowFn        = void (*)(void* thiz, void* method);
using CanRequestAdFn  = bool (*)(void* thiz, void* placement, void* method);
using CanPlayAdFn     = bool (*)(void* thiz, void* placement, void* requester, void* method);
using PreloadAdFn     = void (*)(void* thiz, void* placement, void* method);
using InitializeAdFn  = void (*)(void* thiz, void* userdata, void* method);
using IsReadyFn       = bool (*)(void* placement, void* method);

AdShowFn         g_orig_ad_show                 = nullptr;
CanRequestAdFn   g_orig_can_request_interstitial = nullptr;
CanRequestAdFn   g_orig_can_request_rewarded    = nullptr;
CanPlayAdFn      g_orig_can_play_ad             = nullptr;
PreloadAdFn      g_orig_preload_interstitial    = nullptr;
PreloadAdFn      g_orig_preload_rewarded        = nullptr;
InitializeAdFn   g_orig_ad_initialize           = nullptr;
IsReadyFn        g_orig_is_interstitial_ready   = nullptr;
IsReadyFn        g_orig_is_rewarded_ready       = nullptr;
std::atomic<bool> g_disable_ads{false};
std::atomic<int>  g_ads_blocked{0};

void hooked_ad_show(void* thiz, void* method) {
    if (g_disable_ads.load()) {
        g_ads_blocked.fetch_add(1);
        return;
    }
    if (g_orig_ad_show != nullptr) g_orig_ad_show(thiz, method);
}

bool hooked_can_request_interstitial(void* thiz, void* placement, void* method) {
    if (g_disable_ads.load()) return false;
    if (g_orig_can_request_interstitial == nullptr) return false;
    return g_orig_can_request_interstitial(thiz, placement, method);
}

bool hooked_can_request_rewarded(void* thiz, void* placement, void* method) {
    if (g_disable_ads.load()) return false;
    if (g_orig_can_request_rewarded == nullptr) return false;
    return g_orig_can_request_rewarded(thiz, placement, method);
}

bool hooked_can_play_ad(void* thiz, void* placement, void* requester, void* method) {
    if (g_disable_ads.load()) return false;
    if (g_orig_can_play_ad == nullptr) return false;
    return g_orig_can_play_ad(thiz, placement, requester, method);
}

void hooked_preload_interstitial(void* thiz, void* placement, void* method) {
    if (g_disable_ads.load()) {
        g_ads_blocked.fetch_add(1);
        return;
    }
    if (g_orig_preload_interstitial != nullptr) {
        g_orig_preload_interstitial(thiz, placement, method);
    }
}

void hooked_preload_rewarded(void* thiz, void* placement, void* method) {
    if (g_disable_ads.load()) {
        g_ads_blocked.fetch_add(1);
        return;
    }
    if (g_orig_preload_rewarded != nullptr) {
        g_orig_preload_rewarded(thiz, placement, method);
    }
}

void hooked_ad_initialize(void* thiz, void* userdata, void* method) {
    if (g_disable_ads.load()) {
        return;
    }
    if (g_orig_ad_initialize != nullptr) {
        g_orig_ad_initialize(thiz, userdata, method);
    }
}

bool hooked_is_interstitial_ready(void* placement, void* method) {
    if (g_disable_ads.load()) return false;
    if (g_orig_is_interstitial_ready == nullptr) return false;
    return g_orig_is_interstitial_ready(placement, method);
}

bool hooked_is_rewarded_ready(void* placement, void* method) {
    if (g_disable_ads.load()) return false;
    if (g_orig_is_rewarded_ready == nullptr) return false;
    return g_orig_is_rewarded_ready(placement, method);
}

}

bool install_hooks() {
    if (g_hooks_installed.load()) return true;
    if (!il2cpp::is_resolved() && !il2cpp::resolve()) {
        return false;
    }
    const std::uintptr_t base = il2cpp::base_address();

    g_surfer_set_unlocked = reinterpret_cast<SetUnlockedFn>(
        base + rva::kSurferServiceSetSurferUnlocked);
    g_board_set_unlocked  = reinterpret_cast<SetUnlockedFn>(
        base + rva::kBoardServiceSetBoardUnlocked);

    {
        void* target = reinterpret_cast<void*>(base + rva::kSurferServiceIsSurferUnlocked);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_surfer_is_unlocked));
        if (tramp) {g_orig_surfer_is_unlocked = reinterpret_cast<IsUnlockedFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kBoardServiceIsBoardUnlocked);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_board_is_unlocked));
        if (tramp) {g_orig_board_is_unlocked = reinterpret_cast<IsUnlockedFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kUnlockServiceIsUnlocked);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_unlock_is_unlocked));
        if (tramp) {g_orig_unlock_is_unlocked = reinterpret_cast<GenericIsUnlockedFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAvailabilityServiceIsSurferSkinUnlocked);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_avail_skin_unlocked));
        if (tramp) {g_orig_avail_skin_unlocked = reinterpret_cast<GenericIsUnlockedFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kUpgradableItemsServiceIsUnlocked);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_upgradable_unlocked));
        if (tramp) {g_orig_upgradable_unlocked = reinterpret_cast<GenericIsUnlockedFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kBaseInterstitialAdServiceShow);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_ad_show));
        if (tramp) {g_orig_ad_show = reinterpret_cast<AdShowFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2CanRequestInterstitial);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_can_request_interstitial));
        if (tramp) {g_orig_can_request_interstitial = reinterpret_cast<CanRequestAdFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2CanRequestRewarded);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_can_request_rewarded));
        if (tramp) {g_orig_can_request_rewarded = reinterpret_cast<CanRequestAdFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2CanPlayAd);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_can_play_ad));
        if (tramp) {g_orig_can_play_ad = reinterpret_cast<CanPlayAdFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2PreloadInterstitialAd);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_preload_interstitial));
        if (tramp) {g_orig_preload_interstitial = reinterpret_cast<PreloadAdFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2PreloadRewardedAd);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_preload_rewarded));
        if (tramp) {g_orig_preload_rewarded = reinterpret_cast<PreloadAdFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2Initialize);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_ad_initialize));
        if (tramp) {g_orig_ad_initialize = reinterpret_cast<InitializeAdFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2IsInterstitialReady);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_is_interstitial_ready));
        if (tramp) {g_orig_is_interstitial_ready = reinterpret_cast<IsReadyFn>(tramp);
        }
    }

    {
        void* target = reinterpret_cast<void*>(base + rva::kAdServiceV2IsRewardedVideoReady);
        void* tramp = inline_hook::install(
            target, reinterpret_cast<void*>(&hooked_is_rewarded_ready));
        if (tramp) {g_orig_is_rewarded_ready = reinterpret_cast<IsReadyFn>(tramp);
        }
    }

    g_hooks_installed.store(true);
    return true;
}

void set_unlock_all_surfers(bool enable) {
    g_unlock_all_surfers.store(enable);
}

void set_unlock_all_boards(bool enable) {
    g_unlock_all_boards.store(enable);
}

void set_unlock_all_outfits(bool enable) {
    g_unlock_all_outfits.store(enable);
}

void set_disable_ads(bool enable) {
    g_disable_ads.store(enable);
}

}
