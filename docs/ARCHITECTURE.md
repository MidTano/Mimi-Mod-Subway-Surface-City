# Architecture

## Layers

The project is split across three layers to keep the UI portable between Android and a desktop debug viewer:

```
Android host process                                   Desktop viewer (Windows)
  ┌──────────────────────────┐                           ┌──────────────────────────┐
  │ core/hooks  (PLT hook)   │                           │ desktop/main.cpp         │
  │ core/menu   (MenuRenderer)│   shared render loop     │  (WGL + win32 backend)   │
  │ core/features, core/config│   ─────────>             │                          │
  └────────┬─────────────────┘                           └──────────────┬───────────┘
           │                                                            │
           ▼                                                            ▼
  ┌──────────────────────────────────── ui ─────────────────────────────────────┐
  │ menu_root       (top-level composition, input plumbing, scroll logic)       │
  │ layout/         (app_shell, top_banner, stat_row, tab_bar, sheet, fqa_dock) │
  │ sections/       (RUN / OPTICS / CORE / POWER / MODES / FQA / CONFIG pages)  │
  │ widgets/        (cards, hero_card, heading, primitives, color_picker)       │
  │ fx/             (glitch, animation, ticker)                                 │
  │ fonts/          (Archivo Black, Space Mono, Material Icons, font_data.h)    │
  │ state/          (mod_state key/value store)                                 │
  │ tokens.hpp      (design tokens: colors, spacing, font sizes)                │
  │ scale.hpp       (dp/sp + safe-area helpers)                                 │
  └─────────────────────────────────────────────────────────────────────────────┘
```

## Rendering model

Each frame is composed of three draw lists:

1. **Background** via `ImGui::GetWindowDrawList()` — charcoal fill + 20 dp grid pattern.
2. **Content** via the same window draw list inside a scrollable `BeginChild` — top banner, stat row, section cards, bottom tab bar.
3. **Overlay** via `ImGui::GetForegroundDrawList()` — bottom sheet, color picker modal, FQA dock, glitch FX.

`layout::app_shell::compute()` produces the rectangles for every region once per frame so every layout component draws in absolute viewport coordinates.

## State

`ui::state` is a flat `std::unordered_map<std::string, std::variant<...>>` keyed by dotted strings (`hud.visible`, `camera.mode`, `fqa.dock_y_norm`, ...). All widgets read / write through this store so:

- Toggling a control from the FQA dock is immediately reflected in the cards section.
- Persistence (JSON config load/save) is centralized.
- The desktop viewer and Android share the exact same state semantics.

## Input

### Android

`Java_com_google_firebase_MessagingUnityPlayerActivity_nativeTouchFromDispatchTouchEvent` receives the raw `MotionEvent` data and pushes it into `MenuRenderer::pushTouchEvent`. Inside the render pass we translate the action code into ImGui mouse events (`DOWN` / `UP` → button toggle, `MOVE` → position only).

### Desktop

`ImGui_ImplWin32_WndProcHandler` handles everything.

### Touch-drag scroll

`menu_root::render_main` wraps the sections in a `BeginChild` and manually converts pointer drags into `ImGui::SetScrollY`. It:

1. Starts a pending scroll gesture on `IsMouseClicked(0)` when no item is active.
2. Locks to vertical once the drag exceeds an 8 dp slop and vertical dominates horizontal.
3. While locked, calls `ClearActiveID()` so any widget that claimed focus on the initial press releases it.
4. On release, applies inertial decay (`v *= exp(-dt * 6)`) until the velocity falls below a threshold.

## FX pipeline

Glitch FX is a 520 ms animation driven from `fx::glitch::State::phase`. The heavy part of the original implementation was the 16-tap glow ring; it is now reduced to 4 cardinal taps at a single ring, which lowered the per-frame text submission count by ~70 % without visible quality loss.

## Performance notes

- Font atlases are built once at startup (`fonts::load_defaults`). No runtime re-bake.
- Background grid uses `AddLine` per grid line; density is `scale::dp(20.0f)` which translates to ~60 lines on a 1080p phone.
- Hazard scroll and ticker marquee are clipped and state-driven so the clip region bounds are minimal.
- FQA dock computes its visible feature list once per frame and short-circuits when empty.
