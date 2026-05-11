# mtype-sfml demo

Minimal SFML 3 app written in mType, driven by the `mtype-sfml`
runtime plugin.

## Prerequisites

Build the plugin first (`cmake -B build && cmake --build build --config Release`).
After building, the plugin and its SFML runtime DLLs land in
`build/Release/`:

```
build/Release/mtype_sfml.dll
build/Release/sfml-graphics-3.dll  (Windows: copied by post-build)
build/Release/sfml-window-3.dll
build/Release/sfml-system-3.dll
```

Copy them into a directory mType can find (or run from the workspace
root so `mt/mtype_sfml.dll` resolves).

## Run

From the project root:

```
mType.exe mt/demo/demo.mt
```

You should see a 900×600 window with a red player rectangle and a
blue orbiting circle. WASD moves the player. Esc or window-close
exits.

## Files

| File | Purpose |
|---|---|
| `lib/Sfml.mt`     | mType wrappers around `__native__sfml_*` natives — `RenderWindow`, event poll + tag IDs, `Event` payload accessors, realtime `Keyboard`/`Mouse`, plus `Key` / `MouseButton` constant classes. |
| `lib/Graphics.mt` | Wrappers for `Texture`, `Sprite`, `RectangleShape`, `CircleShape`, `Font`, `Text`, and a flat `Draw::sprite/rect/circle/text(window, …)` helper. |
| `demo/demo.mt`    | The demo above — event loop, realtime WASD movement, shape drawing. |
| `demo/demo_vertex_view.mt` | Phase 2 — custom rendering. A 64-segment rainbow triangle-strip ribbon built once with `VertexArray`, viewed through a `View` (camera) that pans (WASD), zooms (Q/E), and rotates (R). Shows the world-space-then-HUD pattern (`Camera::setView` ↔ `Camera::resetView`). |
| `demo/demo_post_fx.mt`     | Phase 3 + 4 — offscreen rendering + GLSL post-processing. Renders the rainbow ribbon into a `RenderTexture`, then composites that to the window through an inline fragment shader (wavy distortion + scanlines, driven by a `time` uniform). Space toggles the shader, Up/Down adjust the wave amplitude. |
| `demo/demo_sfml_imgui.mt`  | SFML + ImGui in the same window (via `imgui-sfml`). Rainbow ribbon (`VertexArray`) rendered underneath an ImGui controls panel whose sliders adjust the ribbon's Y, height, and animation speed live. Demonstrates the `ImGui::init/processSfmlEvent/update/render/shutdown` lifecycle. |
| `lib/ImGui.mt`             | mType wrapper around the `__imgui_*` natives — lifecycle (`init`/`processSfmlEvent`/`update`/`render`/`shutdown`) plus the full ImGui widget surface (windows, buttons, sliders, popups, tables, tabs, docking, trees, menus, tooltips, fonts, images, …). |
