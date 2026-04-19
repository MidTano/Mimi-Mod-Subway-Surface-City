#include "fonts.hpp"
#include "icons.hpp"
#include "font_data.h"
#include "font_noto.h"
#include "../tokens.hpp"

namespace ui::fonts {

namespace {
ImFont* g_fonts[static_cast<int>(Face::Count)] = {};
FontSources g_src{};
}

void set_sources(const FontSources& src) { g_src = src; }

bool load(float dpi) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    static ImVector<ImWchar> s_icon_ranges;
    if (s_icon_ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        for (int i = 0; i < icons::kAllIconsCount; ++i) {
            builder.AddText(icons::kAllIcons[i]);
        }
        builder.BuildRanges(&s_icon_ranges);
    }
    const ImWchar* icon_ranges = s_icon_ranges.Data;

    static const ImWchar text_ranges_arr[] = {
        0x0020, 0x00FF,
        0x0400, 0x04FF,
        0,
    };
    const ImWchar* text_ranges = text_ranges_arr;

    static const ImWchar cyrillic_range_arr[] = {
        0x0400, 0x04FF,
        0,
    };
    const ImWchar* cyrillic_range = cyrillic_range_arr;

    auto add_face = [&](const FontBlob& blob, float px, const ImWchar* ranges) -> ImFont* {
        if (!blob.data || blob.size == 0) return nullptr;
        ImFontConfig cfg{};
        cfg.FontDataOwnedByAtlas = false;
        return io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(blob.data),
                                              static_cast<int>(blob.size),
                                              px * dpi, &cfg, ranges);
    };

    const FontBlob& brut = g_src.brutalist.data ? g_src.brutalist : g_src.mono;
    const FontBlob& mono = g_src.mono.data ? g_src.mono : g_src.brutalist;
    const FontBlob& icns = g_src.icons.data ? g_src.icons : mono;
    const FontBlob  cyr  = { g_font_noto, g_font_noto_size };

    auto merge_cyrillic = [&](float px) {
        if (!cyr.data || cyr.size == 0) return;
        ImFontConfig cfg{};
        cfg.MergeMode = true;
        cfg.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(cyr.data),
                                       static_cast<int>(cyr.size),
                                       px * dpi, &cfg, cyrillic_range);
    };

    g_fonts[(int)Face::BrutalistLg] = add_face(brut, tokens::kFontHeading,   text_ranges);
    merge_cyrillic(tokens::kFontHeading);
    g_fonts[(int)Face::BrutalistMd] = add_face(brut, tokens::kFontHeadingSm, text_ranges);
    merge_cyrillic(tokens::kFontHeadingSm);
    g_fonts[(int)Face::BrutalistSm] = add_face(brut, tokens::kFontHeadingXs, text_ranges);
    merge_cyrillic(tokens::kFontHeadingXs);
    g_fonts[(int)Face::BrutalistXs] = add_face(brut, 10.0f,                  text_ranges);
    merge_cyrillic(10.0f);
    g_fonts[(int)Face::MonoMd]      = add_face(mono, tokens::kFontMono,      text_ranges);
    merge_cyrillic(tokens::kFontMono);
    g_fonts[(int)Face::MonoSm]      = add_face(mono, tokens::kFontMonoSm,    text_ranges);
    merge_cyrillic(tokens::kFontMonoSm);
    g_fonts[(int)Face::MonoXs]      = add_face(mono, tokens::kFontMonoXs,    text_ranges);
    merge_cyrillic(tokens::kFontMonoXs);
    g_fonts[(int)Face::HeroLg]      = add_face(brut, tokens::kFontHero,      text_ranges);
    merge_cyrillic(tokens::kFontHero);
    g_fonts[(int)Face::HeroMd]      = add_face(brut, tokens::kFontHeroSm,    text_ranges);
    merge_cyrillic(tokens::kFontHeroSm);
    g_fonts[(int)Face::IconXl]      = add_face(icns, 112.0f,                 icon_ranges);
    g_fonts[(int)Face::IconLg]      = add_face(icns, tokens::kFontIcon,      icon_ranges);
    g_fonts[(int)Face::IconMd]      = add_face(icns, tokens::kFontIconSm,    icon_ranges);
    g_fonts[(int)Face::IconSm]      = add_face(icns, 14.0f,                  icon_ranges);

    if (!g_fonts[0]) {
        io.Fonts->AddFontDefault();
        io.FontDefault = nullptr;
        return false;
    }
    io.FontDefault = g_fonts[(int)Face::MonoMd];
    return true;
}

bool load_defaults(float dpi) {
    FontSources src;
    src.brutalist = { g_font_archivo_black,  g_font_archivo_black_size };
    src.mono      = { g_font_space_mono,     g_font_space_mono_size };
    src.icons     = { g_font_material_icons, g_font_material_icons_size };
    set_sources(src);
    return load(dpi);
}

ImFont* get(Face f) {
    int i = static_cast<int>(f);
    if (i < 0 || i >= (int)Face::Count) return nullptr;
    return g_fonts[i];
}

void push(Face f) {
    if (auto* fnt = get(f)) ImGui::PushFont(fnt);
}
void pop() { ImGui::PopFont(); }

}
