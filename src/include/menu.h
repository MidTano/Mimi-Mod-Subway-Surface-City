#pragma once

#include <imgui.h>

#include "ui/menu_root.hpp"

namespace Menu {

class MenuRenderer {
public:
    static MenuRenderer& getInstance();

    void initialize();
    void shutdown();
    void render();
    void toggleVisibility();
    void pushTouchEvent(int action, float x, float y, int pointerCount, int viewWidth, int viewHeight);

    bool isVisible() const;
    bool wantsCaptureInput() const;

private:
    MenuRenderer();
    ~MenuRenderer() = default;

    MenuRenderer(const MenuRenderer&) = delete;
    MenuRenderer& operator=(const MenuRenderer&) = delete;

    bool initialized = false;
    ::ui::menu_root::State menuState{};
};

}
