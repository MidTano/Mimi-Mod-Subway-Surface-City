#pragma once

#include <cstdint>

namespace il2cpp::rva {

constexpr std::uintptr_t kTimeSetTimeScale           = 0x73a8b94;
constexpr std::uintptr_t kTimeGetTimeScale           = 0x73a8b64;
constexpr std::uintptr_t kCameraGetMain              = 0x73559dc;
constexpr std::uintptr_t kCameraSetFieldOfView       = 0x735206c;
constexpr std::uintptr_t kCameraGetFieldOfView       = 0x7352030;
constexpr std::uintptr_t kCameraWorldToScreenPointInjected = 0x7354aec;

constexpr std::uintptr_t kPlayerMovementVelocityHelper     = 0x67800f8;
constexpr std::uintptr_t kPlayerInteractionTransitionRoll  = 0x6a24524;
constexpr std::uintptr_t kGameplayGetMultiplier            = 0x682f96c;
constexpr std::uintptr_t kPowerupGetState                  = 0x68312e4;
constexpr std::uintptr_t kIsActivePerkA                    = 0x6a1da78;
constexpr std::uintptr_t kIsActivePerkB                    = 0x6a20a78;
constexpr std::uintptr_t kIsActiveStats                    = 0x675c28c;

constexpr std::uintptr_t kMagnetizationHelperShouldMagnetize = 0x6b0f754;
constexpr std::uintptr_t kMagnetProcessSystemRun             = 0x6773fa8;
constexpr std::uintptr_t kPowerUpsSystemRun                  = 0x6775ae4;
constexpr std::uintptr_t kGameplayStateSystemUpdatePowerup   = 0x675bcdc;

constexpr std::uintptr_t kMagnetTimeFsub                     = 0x67741e4;
constexpr std::uintptr_t kShieldTimeFsub                     = 0x67706f8;
constexpr std::uintptr_t kCoinDoublerTimeFsub                = 0x67714b8;

constexpr std::uintptr_t kForceTransitionToDying             = 0x6a23174;
constexpr std::uintptr_t kQuitRun                            = 0x6a23390;
constexpr std::uintptr_t kCanJump                            = 0x6a23b44;
constexpr std::uintptr_t kIsForgivenGrounded                 = 0x6a22fb0;
constexpr std::uintptr_t kIsJumping                          = 0x6a23ae8;
constexpr std::uintptr_t kScreenMainPresenterFocus           = 0x68156b0;
constexpr std::uintptr_t kScreenMainPresenterStartRun        = 0x6819328;
constexpr std::uintptr_t kScreenMainPresenterOnStartRun      = 0x68192f4;
constexpr std::uintptr_t kScreenMainPresenterRefresh         = 0x68172cc;
constexpr std::uintptr_t kReviveSubPresenterBind             = 0x68e0c84;
constexpr std::uintptr_t kReviveSubPresenterOnContinue       = 0x68e1d40;
constexpr std::uintptr_t kReviveSubPresenterTransitionRunOver = 0x68e1df8;
constexpr std::uintptr_t kScreenReviveSystemRun              = 0x679112c;
constexpr std::uintptr_t kSchedulersProviderTryBuildInfinite = 0x69b73f8;

constexpr std::uintptr_t kScreenRunOverPresenterBind         = 0x67ee060;
constexpr std::uintptr_t kScreenRunOverPresenterFocus        = 0x67ee05c;
constexpr std::uintptr_t kScreenRunOverPresenterRefresh      = 0x67ee91c;
constexpr std::uintptr_t kScreenRunOverPresenterOnContinue   = 0x67f1380;
constexpr std::uintptr_t kScreenRunOverSystemInit            = 0x6792674;
constexpr std::uintptr_t kFiniteRunHomeScreenFlowSystemRun   = 0x679fd28;
constexpr std::uintptr_t kFiniteRunOverSystemRun             = 0x679fe80;

constexpr std::uintptr_t kSubwayFollowCameraMutateCameraState = 0x6a4f5b8;
constexpr std::uintptr_t kPlayerInputSystemRun                = 0x678643c;

constexpr std::uintptr_t kBoardServiceIsBoardUnlocked        = 0x6712a38;
constexpr std::uintptr_t kBoardServiceSetBoardUnlocked       = 0x6712b68;
constexpr std::uintptr_t kBoardServiceSetSelected            = 0x67127ec;

constexpr std::uintptr_t kSurferServiceIsSurferUnlocked      = 0x6743fd8;
constexpr std::uintptr_t kSurferServiceSetSurferUnlocked     = 0x6743a20;
constexpr std::uintptr_t kSurferServiceSetSelectedSurfer     = 0x674452c;
constexpr std::uintptr_t kSurferServiceGetAllSurfers         = 0x674384c;

constexpr std::uintptr_t kSurferSkinServiceUnlockSurferSkin        = 0x67482c0;
constexpr std::uintptr_t kSurferSkinServiceUnlockDefaultSkin       = 0x6748690;
constexpr std::uintptr_t kSurferSkinServiceGetSurferUnlockedSkins  = 0x6748de4;

constexpr std::uintptr_t kUnlockServiceIsUnlocked                  = 0x68f33a8;
constexpr std::uintptr_t kAvailabilityServiceIsSurferSkinUnlocked  = 0x683be7c;
constexpr std::uintptr_t kUpgradableItemsServiceIsUnlocked         = 0x68f92d4;

constexpr std::uintptr_t kBaseInterstitialAdServiceShow            = 0x6b395bc;
constexpr std::uintptr_t kAdServiceV2CanRequestInterstitial        = 0x6b33c08;
constexpr std::uintptr_t kAdServiceV2CanRequestRewarded            = 0x6b33b84;
constexpr std::uintptr_t kAdServiceV2Initialize                    = 0x6b32a50;
constexpr std::uintptr_t kAdServiceV2PreloadInterstitialAd         = 0x6b32e78;
constexpr std::uintptr_t kAdServiceV2PreloadRewardedAd             = 0x6b32da8;
constexpr std::uintptr_t kAdServiceV2CanPlayAd                     = 0x6b33180;
constexpr std::uintptr_t kAdServiceV2IsInterstitialReady           = 0x6b33ae8;
constexpr std::uintptr_t kAdServiceV2IsRewardedVideoReady          = 0x6b3395c;

}
