#pragma once

#include <imgui.h>

namespace ui::tokens {

constexpr ImU32 kToxic        = IM_COL32(0xCC, 0xFF, 0x00, 0xFF);
constexpr ImU32 kToxicDim     = IM_COL32(0xCC, 0xFF, 0x00, 0x66);
constexpr ImU32 kToxicFaint   = IM_COL32(0xCC, 0xFF, 0x00, 0x1A);
constexpr ImU32 kCharcoal     = IM_COL32(0x12, 0x12, 0x12, 0xFF);
constexpr ImU32 kBlack        = IM_COL32(0x00, 0x00, 0x00, 0xFF);
constexpr ImU32 kBlack60      = IM_COL32(0x00, 0x00, 0x00, 0x99);
constexpr ImU32 kOffWhite     = IM_COL32(0xE5, 0xE5, 0xE5, 0xFF);
constexpr ImU32 kOffWhiteDim  = IM_COL32(0xE5, 0xE5, 0xE5, 0x80);
constexpr ImU32 kOffWhiteFaint= IM_COL32(0xE5, 0xE5, 0xE5, 0x66);
constexpr ImU32 kErrorRed     = IM_COL32(0xFF, 0x33, 0x55, 0xFF);
constexpr ImU32 kHazardOrange = IM_COL32(0xF7, 0xA9, 0x00, 0xFF);
constexpr ImU32 kCardBg       = IM_COL32(0xFF, 0xFF, 0xFF, 0x0D);
constexpr ImU32 kCyan         = IM_COL32(0x00, 0xE5, 0xFF, 0xFF);

constexpr float kBorderThick      = 3.0f;
constexpr float kBorderThin       = 2.0f;
constexpr float kBorderHair       = 1.5f;
constexpr float kShadowOffsetThick= 4.0f;
constexpr float kShadowOffsetThin = 3.0f;
constexpr float kShadowOffsetHair = 2.0f;

constexpr float kAppMaxWidth   = 440.0f;
constexpr float kBannerHeight  = 30.0f;
constexpr float kHazardHeight  = 10.0f;
constexpr float kHazardThin    = 8.0f;
constexpr float kStatRowHeight = 40.0f;
constexpr float kTabBarHeight  = 72.0f;
constexpr float kFqaButtonSize = 48.0f;

constexpr float kScrollPadX  = 12.0f;
constexpr float kScrollPadY  = 16.0f;
constexpr float kCardSpacing = 12.0f;
constexpr float kCardPadding = 12.0f;

constexpr float kFontHeading    = 20.0f;
constexpr float kFontHeadingSm  = 15.0f;
constexpr float kFontHeadingXs  = 13.0f;
constexpr float kFontMono       = 13.0f;
constexpr float kFontMonoSm     = 12.0f;
constexpr float kFontMonoXs     = 11.0f;
constexpr float kFontHero       = 32.0f;
constexpr float kFontHeroSm     = 24.0f;
constexpr float kFontIcon       = 22.0f;
constexpr float kFontIconSm     = 18.0f;

constexpr float kAnimFast   = 0.08f;
constexpr float kAnimMedium = 0.16f;
constexpr float kAnimSlow   = 0.26f;
constexpr float kAnimGlitch = 0.52f;

constexpr float kSkewX = -0.2125f;

inline ImU32 with_alpha_mul(ImU32 col, float mul) {
    if (mul >= 1.0f) return col;
    if (mul <= 0.0f) return col & 0x00FFFFFF;
    unsigned a = (col >> IM_COL32_A_SHIFT) & 0xFF;
    a = static_cast<unsigned>(static_cast<float>(a) * mul + 0.5f);
    if (a > 255) a = 255;
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

}
