#pragma once

#include <imgui.h>

#include <cstddef>

namespace ui::fonts {

enum class Face : int {
    BrutalistLg = 0,
    BrutalistMd,
    BrutalistSm,
    BrutalistXs,
    MonoMd,
    MonoSm,
    MonoXs,
    HeroLg,
    HeroMd,
    IconXl,
    IconLg,
    IconMd,
    IconSm,
    Count
};

struct FontBlob {
    const unsigned char* data = nullptr;
    std::size_t size = 0;
};

struct FontSources {
    FontBlob brutalist;
    FontBlob mono;
    FontBlob icons;
};

void set_sources(const FontSources& src);

bool load(float dpi_scale);
bool load_defaults(float dpi_scale);
ImFont* get(Face f);

void push(Face f);
void pop();

}
