/*
 * ImGui bindings via imgui-sfml.
 *
 * Lifecycle is collapsed compared to the SDL plugin: ImGui::SFML::Init
 * creates the context AND wires both platform + renderer backends in
 * one call, so there's no separate __imgui_create_context. Per-frame:
 *
 *   ImGui::init(window);          // once at startup
 *   ...loop:
 *     while (poll event) { ImGui::processSfmlEvent(window); ... }
 *     ImGui::update(window, dtMs);
 *     ...widget calls...
 *     window.clear(...);
 *     ...world draws...
 *     ImGui::render(window);      // draws ImGui on top
 *     window.display();
 *   ImGui::shutdown();
 */

#include "PluginGlobals.hpp"
#include "BindingHelpers.hpp"

#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace mtypesfml
{
    namespace
    {
        constexpr const char* kEx = "ImGuiError";

        inline bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            return detail::requireArgs(ctx, argc, expected, name, kEx);
        }
        inline const char* getStr(const MTypeValue* v, size_t* outLen = nullptr)
        {
            return detail::getStr(v, outLen);
        }

        /* ---------------------------------------------------------------- */

        /* ImGui-SFML init: creates the ImGuiContext AND initializes the
         * platform + renderer backends bound to the given RenderWindow. */
        MTypeValue* nImGuiInit(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_init")) return g_host->makeBool(ctx, 0);
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) {
                g_host->raiseError(ctx, kEx, "__native__imgui_init: invalid window id");
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::SFML::Init(*w) ? 1 : 0);
        }

        MTypeValue* nImGuiShutdown(void*, MTypeContext* ctx,
                                     const MTypeValue* const*, int)
        {
            ImGui::SFML::Shutdown();
            return g_host->makeVoid(ctx);
        }

        /* Hand the most-recently-polled SFML event to ImGui. Call from
         * the event loop after Sfml.pollEvent() returns non-zero, BEFORE
         * the script reads Event::* itself. */
        MTypeValue* nImGuiProcessSfmlEvent(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_process_sfml_event")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) imguiProcessLastEvent(*w);
            return g_host->makeVoid(ctx);
        }

        /* ImGui::SFML::Update — combines NewFrame + platform/renderer
         * NewFrame and advances ImGui's internal clock. dtMs is in
         * milliseconds (mType has no native duration type). */
        MTypeValue* nImGuiUpdate(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_update")) return g_host->makeVoid(ctx);
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return g_host->makeVoid(ctx);
            float dtMs = static_cast<float>(g_host->getFloat(args[1]));
            ImGui::SFML::Update(*w, sf::milliseconds(static_cast<int32_t>(dtMs)));
            return g_host->makeVoid(ctx);
        }

        /* ImGui::SFML::Render — composites ImGui's draw data on top of
         * the window. Call AFTER your world-space draws, BEFORE display. */
        MTypeValue* nImGuiRender(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_render")) return g_host->makeVoid(ctx);
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) ImGui::SFML::Render(*w);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiBegin(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin")) return g_host->makeBool(ctx, 0);
            const char* name = getStr(args[0]);
            return g_host->makeBool(ctx, ImGui::Begin(name) ? 1 : 0);
        }

        MTypeValue* nImGuiSetNextWindowPos(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_set_next_window_pos")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::SetNextWindowPos(
                ImVec2(static_cast<float>(g_host->getFloat(args[0])),
                       static_cast<float>(g_host->getFloat(args[1]))),
                ImGuiCond_Always);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSetNextWindowSize(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_set_next_window_size")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(g_host->getFloat(args[0])),
                       static_cast<float>(g_host->getFloat(args[1]))),
                ImGuiCond_Always);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiBeginFixed(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_fixed")) return g_host->makeBool(ctx, 0);
            const char* name = getStr(args[0]);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                                   | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                                   | ImGuiWindowFlags_NoSavedSettings;
            return g_host->makeBool(ctx, ImGui::Begin(name, nullptr, flags) ? 1 : 0);
        }

        /* Closable window: ImGui::Begin(label, &p_open) — adds the X close
         * button to the title bar. The C ABI can't pass a bool reference,
         * so we fold the in/out into a 2-element bool array:
         *
         *   ret[0] = shouldDraw  (Begin's own return — false when window is
         *                         collapsed; caller skips inner widgets)
         *   ret[1] = newOpen     (false when user clicked the X this frame)
         *
         * Caller pattern (always call End regardless of shouldDraw):
         *
         *   bool[] s = ImGui::beginClosable("Title", windowOpen);
         *   if (s[0]) { ... widgets ... }
         *   ImGui::end();
         *   windowOpen = s[1];
         */
        MTypeValue* nImGuiBeginClosable(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_begin_closable")) {
                return g_host->makeNull(ctx);
            }
            const char* name = getStr(args[0]);
            bool open = g_host->getBool(args[1]) != 0;
            bool shouldDraw = ImGui::Begin(name, &open);

            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_BOOL, 2);
            g_host->arraySet(out, 0, g_host->makeBool(ctx, shouldDraw ? 1 : 0));
            g_host->arraySet(out, 1, g_host->makeBool(ctx, open ? 1 : 0));
            return out;
        }

        MTypeValue* nImGuiEnd(void*, MTypeContext* ctx,
                               const MTypeValue* const*, int)
        {
            ImGui::End();
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 6: Popups.
         *
         * Two-step pattern: trigger with openPopup(id), then on EVERY frame
         * call beginPopup(id) — it returns true while the popup is open.
         *
         *   if (ImGui::button("Open")) ImGui::openPopup("##my_popup");
         *   if (ImGui::beginPopup("##my_popup")) {
         *       ImGui::text("hello"); if (ImGui::button("Close")) ImGui::closeCurrentPopup();
         *       ImGui::endPopup();
         *   }
         *
         * Modal popups (beginPopupModal) block input behind them and
         * support the X close button — same bool[2] [shouldDraw, newOpen]
         * pattern as beginClosable.
         *
         * Context-menu popups (beginPopupContextItem / Window) auto-trigger
         * on a right-click of the previous item / window background, so no
         * matching openPopup() is required.
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiOpenPopup(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_open_popup")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::OpenPopup(getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBeginPopup(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_popup")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginPopup(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiBeginPopupModal(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_begin_popup_modal")) {
                return g_host->makeNull(ctx);
            }
            const char* name = getStr(args[0]);
            bool open = g_host->getBool(args[1]) != 0;
            bool shouldDraw = ImGui::BeginPopupModal(name, &open);
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_BOOL, 2);
            g_host->arraySet(out, 0, g_host->makeBool(ctx, shouldDraw ? 1 : 0));
            g_host->arraySet(out, 1, g_host->makeBool(ctx, open ? 1 : 0));
            return out;
        }
        MTypeValue* nImGuiBeginPopupContextItem(void*, MTypeContext* ctx,
                                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_popup_context_item")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginPopupContextItem(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiBeginPopupContextWindow(void*, MTypeContext* ctx,
                                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_popup_context_window")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginPopupContextWindow(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiEndPopup(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndPopup();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiCloseCurrentPopup(void*, MTypeContext* ctx,
                                              const MTypeValue* const*, int)
        {
            ImGui::CloseCurrentPopup();
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 6: Styles.
         *
         * Theme switches are direct calls. Per-section overrides use
         * push/pop, with the index referenced by NAME so .mt code doesn't
         * have to track ImGui's enum re-numberings between versions.
         *
         * Color names: "Text", "WindowBg", "ChildBg", "PopupBg", "Border",
         *   "FrameBg", "FrameBgHovered", "FrameBgActive", "TitleBg",
         *   "TitleBgActive", "MenuBarBg", "ScrollbarBg", "CheckMark",
         *   "SliderGrab", "SliderGrabActive", "Button", "ButtonHovered",
         *   "ButtonActive", "Header", "HeaderHovered", "HeaderActive",
         *   "Separator", "Tab", "TabHovered", "TabActive", "DockingPreview".
         *
         * StyleVar (float) names: "Alpha", "DisabledAlpha", "WindowRounding",
         *   "WindowBorderSize", "ChildRounding", "ChildBorderSize",
         *   "PopupRounding", "PopupBorderSize", "FrameRounding",
         *   "FrameBorderSize", "IndentSpacing", "ScrollbarSize",
         *   "ScrollbarRounding", "GrabMinSize", "GrabRounding", "TabRounding".
         *
         * StyleVar (vec2) names: "WindowPadding", "WindowMinSize",
         *   "WindowTitleAlign", "FramePadding", "ItemSpacing",
         *   "ItemInnerSpacing", "CellPadding", "ButtonTextAlign",
         *   "SelectableTextAlign".
         *
         * Unknown names raise ImGuiError so typos surface immediately.
         * ---------------------------------------------------------------- */

        ImGuiCol resolveColorIdx(const char* name)
        {
            #define M(N) if (std::strcmp(name, #N) == 0) return ImGuiCol_##N
            M(Text); M(TextDisabled); M(WindowBg); M(ChildBg); M(PopupBg);
            M(Border); M(BorderShadow); M(FrameBg); M(FrameBgHovered);
            M(FrameBgActive); M(TitleBg); M(TitleBgActive); M(TitleBgCollapsed);
            M(MenuBarBg); M(ScrollbarBg); M(ScrollbarGrab); M(ScrollbarGrabHovered);
            M(ScrollbarGrabActive); M(CheckMark); M(SliderGrab); M(SliderGrabActive);
            M(Button); M(ButtonHovered); M(ButtonActive);
            M(Header); M(HeaderHovered); M(HeaderActive);
            M(Separator); M(SeparatorHovered); M(SeparatorActive);
            M(ResizeGrip); M(ResizeGripHovered); M(ResizeGripActive);
            M(Tab); M(TabHovered); M(TabSelected); M(TabSelectedOverline);
            M(TabDimmed); M(TabDimmedSelected); M(TabDimmedSelectedOverline);
            M(DockingPreview); M(DockingEmptyBg);
            M(PlotLines); M(PlotLinesHovered); M(PlotHistogram); M(PlotHistogramHovered);
            M(TableHeaderBg); M(TableBorderStrong); M(TableBorderLight);
            M(TableRowBg); M(TableRowBgAlt);
            M(TextSelectedBg); M(DragDropTarget);
            M(NavCursor); M(NavWindowingHighlight); M(NavWindowingDimBg);
            M(ModalWindowDimBg);
            #undef M
            return static_cast<ImGuiCol>(-1);
        }

        ImGuiStyleVar resolveStyleVarIdx(const char* name)
        {
            #define M(N) if (std::strcmp(name, #N) == 0) return ImGuiStyleVar_##N
            M(Alpha); M(DisabledAlpha);
            M(WindowPadding); M(WindowRounding); M(WindowBorderSize);
            M(WindowMinSize); M(WindowTitleAlign);
            M(ChildRounding); M(ChildBorderSize);
            M(PopupRounding); M(PopupBorderSize);
            M(FramePadding); M(FrameRounding); M(FrameBorderSize);
            M(ItemSpacing); M(ItemInnerSpacing);
            M(IndentSpacing); M(CellPadding);
            M(ScrollbarSize); M(ScrollbarRounding);
            M(GrabMinSize); M(GrabRounding);
            M(TabRounding); M(TabBorderSize);
            M(ButtonTextAlign); M(SelectableTextAlign);
            M(SeparatorTextBorderSize); M(SeparatorTextAlign); M(SeparatorTextPadding);
            #undef M
            return static_cast<ImGuiStyleVar>(-1);
        }

        MTypeValue* nImGuiStyleDark(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::StyleColorsDark();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiStyleLight(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::StyleColorsLight();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiStyleClassic(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::StyleColorsClassic();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiPushStyleColor(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__imgui_push_style_color")) {
                return g_host->makeVoid(ctx);
            }
            const char* name = getStr(args[0]);
            ImGuiCol idx = resolveColorIdx(name);
            if (static_cast<int>(idx) < 0) {
                std::string m = std::string("__native__imgui_push_style_color: unknown color '")
                              + name + "'";
                g_host->raiseError(ctx, "ImGuiError", m.c_str());
                return g_host->makeVoid(ctx);
            }
            ImVec4 c(static_cast<float>(g_host->getFloat(args[1])),
                     static_cast<float>(g_host->getFloat(args[2])),
                     static_cast<float>(g_host->getFloat(args[3])),
                     static_cast<float>(g_host->getFloat(args[4])));
            ImGui::PushStyleColor(idx, c);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPopStyleColor(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_pop_style_color")) {
                return g_host->makeVoid(ctx);
            }
            int count = static_cast<int>(g_host->getInt(args[0]));
            ImGui::PopStyleColor(count);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPushStyleVarFloat(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_push_style_var_float")) {
                return g_host->makeVoid(ctx);
            }
            const char* name = getStr(args[0]);
            ImGuiStyleVar idx = resolveStyleVarIdx(name);
            if (static_cast<int>(idx) < 0) {
                std::string m = std::string("__native__imgui_push_style_var_float: unknown var '")
                              + name + "'";
                g_host->raiseError(ctx, "ImGuiError", m.c_str());
                return g_host->makeVoid(ctx);
            }
            ImGui::PushStyleVar(idx, static_cast<float>(g_host->getFloat(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPushStyleVarVec2(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_push_style_var_vec2")) {
                return g_host->makeVoid(ctx);
            }
            const char* name = getStr(args[0]);
            ImGuiStyleVar idx = resolveStyleVarIdx(name);
            if (static_cast<int>(idx) < 0) {
                std::string m = std::string("__native__imgui_push_style_var_vec2: unknown var '")
                              + name + "'";
                g_host->raiseError(ctx, "ImGuiError", m.c_str());
                return g_host->makeVoid(ctx);
            }
            ImVec2 v(static_cast<float>(g_host->getFloat(args[1])),
                     static_cast<float>(g_host->getFloat(args[2])));
            ImGui::PushStyleVar(idx, v);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPopStyleVar(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_pop_style_var")) {
                return g_host->makeVoid(ctx);
            }
            int count = static_cast<int>(g_host->getInt(args[0]));
            ImGui::PopStyleVar(count);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiText(void*, MTypeContext* ctx,
                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_text")) return g_host->makeVoid(ctx);
            const char* s = getStr(args[0]);
            ImGui::TextUnformatted(s);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiButton(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_button")) return g_host->makeBool(ctx, 0);
            const char* label = getStr(args[0]);
            return g_host->makeBool(ctx, ImGui::Button(label) ? 1 : 0);
        }

        /* ----------------------------------------------------------------
         * Phase 2: input widgets. The C ABI can't pass C++ references, so
         * the in/out pattern from native ImGui is folded into:
         *
         *   in:  the current value (caller's copy)
         *   out: the new value (== current if widget did not mutate)
         *
         * Whether the widget actually changed this frame is recorded in
         * g_lastWidgetChanged and surfaced via __native__imgui_widget_changed.
         * Both pieces are needed: returned-value comparison can't tell when
         * a slider was dragged back to its starting value within one frame.
         * ---------------------------------------------------------------- */

        bool g_lastWidgetChanged = false;

        MTypeValue* nImGuiWidgetChanged(void*, MTypeContext* ctx,
                                          const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, g_lastWidgetChanged ? 1 : 0);
        }

        MTypeValue* nImGuiSliderFloat(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_slider_float")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            const char* label = getStr(args[0]);
            float v = static_cast<float>(g_host->getFloat(args[1]));
            float lo = static_cast<float>(g_host->getFloat(args[2]));
            float hi = static_cast<float>(g_host->getFloat(args[3]));
            g_lastWidgetChanged = ImGui::SliderFloat(label, &v, lo, hi);
            return g_host->makeFloat(ctx, static_cast<double>(v));
        }

        MTypeValue* nImGuiSliderInt(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_slider_int")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* label = getStr(args[0]);
            int v  = static_cast<int>(g_host->getInt(args[1]));
            int lo = static_cast<int>(g_host->getInt(args[2]));
            int hi = static_cast<int>(g_host->getInt(args[3]));
            g_lastWidgetChanged = ImGui::SliderInt(label, &v, lo, hi);
            return g_host->makeInt(ctx, static_cast<int64_t>(v));
        }

        MTypeValue* nImGuiCheckbox(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_checkbox")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* label = getStr(args[0]);
            bool v = g_host->getBool(args[1]) != 0;
            g_lastWidgetChanged = ImGui::Checkbox(label, &v);
            return g_host->makeBool(ctx, v ? 1 : 0);
        }

        /* Combo accepts items as a string[] mType array. The plugin builds
         * a single \\0-separated buffer that ImGui's Combo() expects.
         * Returns the new selected index. */
        MTypeValue* nImGuiCombo(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_combo")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* label = getStr(args[0]);
            int currentIdx = static_cast<int>(g_host->getInt(args[1]));
            const MTypeValue* itemsArr = args[2];

            std::string buf;
            int count = 0;
            if (g_host->getTag(itemsArr) == MT_TAG_ARRAY) {
                size_t n = g_host->arrayLen(itemsArr);
                count = static_cast<int>(n);
                for (size_t i = 0; i < n; ++i) {
                    MTypeValue* el = g_host->arrayGet(ctx, itemsArr, i);
                    size_t slen = 0;
                    const char* s = g_host->getString(el, &slen);
                    buf.append(s, slen);
                    buf.push_back('\0');
                }
                buf.push_back('\0');  /* second null terminates the items list */
            }
            g_lastWidgetChanged = ImGui::Combo(label, &currentIdx,
                                               count > 0 ? buf.c_str() : "\0\0",
                                               count);
            return g_host->makeInt(ctx, static_cast<int64_t>(currentIdx));
        }

        /* InputText. Plugin sizes its temp buffer to max(256, capCap) so
         * malicious capCap can't blow heap. Returns the modified string. */
        MTypeValue* nImGuiInputText(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_input_text")) {
                return g_host->makeString(ctx, "", 0);
            }
            const char* label = getStr(args[0]);
            size_t curLen = 0;
            const char* curStr = getStr(args[1], &curLen);
            int requestedCap = static_cast<int>(g_host->getInt(args[2]));

            constexpr size_t kMinCap = 256;
            constexpr size_t kMaxCap = 1 << 16;  /* 64 KiB hard ceiling */
            size_t cap = static_cast<size_t>(std::max(0, requestedCap));
            if (cap < kMinCap) cap = kMinCap;
            if (cap > kMaxCap) cap = kMaxCap;

            std::vector<char> buf(cap, 0);
            size_t copy = std::min(curLen, cap - 1);
            std::memcpy(buf.data(), curStr, copy);
            buf[copy] = '\0';

            g_lastWidgetChanged = ImGui::InputText(label, buf.data(), cap);
            return g_host->makeString(ctx, buf.data(), std::strlen(buf.data()));
        }

        /* ColorEdit3. Returns a 3-element float[] array (r,g,b in [0,1]). */
        MTypeValue* nImGuiColorEdit3(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_color_edit3")) {
                return g_host->makeNull(ctx);
            }
            const char* label = getStr(args[0]);
            float c[3] = {
                static_cast<float>(g_host->getFloat(args[1])),
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
            };
            g_lastWidgetChanged = ImGui::ColorEdit3(label, c);

            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 3);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, c[0]));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, c[1]));
            g_host->arraySet(out, 2, g_host->makeFloat(ctx, c[2]));
            return out;
        }

        /* Layout / formatting helpers. All void-returning. */
        MTypeValue* nImGuiSameLine(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::SameLine();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiSeparator(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::Separator();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiSpacing(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::Spacing();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBulletText(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_bullet_text")) {
                return g_host->makeVoid(ctx);
            }
            const char* s = getStr(args[0]);
            ImGui::BulletText("%s", s);
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Tables. Preferred over the deprecated Columns API.
         * Pattern:
         *   if (beginTable("id", 3)) {
         *     setupColumn("a"); setupColumn("b"); setupColumn("c");
         *     headersRow();
         *     for each row { nextRow(); nextColumn(); text(...); ... }
         *     endTable();
         *   }
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiBeginTable(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_begin_table")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* id = getStr(args[0]);
            int cols = static_cast<int>(g_host->getInt(args[1]));
            ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                  | ImGuiTableFlags_RowBg
                                  | ImGuiTableFlags_Resizable;
            return g_host->makeBool(ctx, ImGui::BeginTable(id, cols, flags) ? 1 : 0);
        }
        MTypeValue* nImGuiEndTable(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTable();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableSetupColumn(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_table_setup_column")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::TableSetupColumn(getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableHeadersRow(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::TableHeadersRow();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableNextRow(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::TableNextRow();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTableNextColumn(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::TableNextColumn() ? 1 : 0);
        }
        MTypeValue* nImGuiTableSetColumnIndex(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_table_set_column_index")) {
                return g_host->makeBool(ctx, 0);
            }
            int idx = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, ImGui::TableSetColumnIndex(idx) ? 1 : 0);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Child windows.
         * Pattern:
         *   if (beginChild("id", w, h, true)) {  // last arg = border
         *     // ...content...
         *   }
         *   endChild();   // ALWAYS call, even if begin returned false.
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiBeginChild(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_begin_child")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* id = getStr(args[0]);
            ImVec2 size(static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2])));
            bool border = g_host->getBool(args[3]) != 0;
            ImGuiChildFlags childFlags = border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
            return g_host->makeBool(ctx, ImGui::BeginChild(id, size, childFlags) ? 1 : 0);
        }
        MTypeValue* nImGuiEndChild(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndChild();
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Tab bars.
         * Pattern:
         *   if (beginTabBar("##bar")) {
         *     if (beginTabItem("one")) { … endTabItem(); }
         *     if (beginTabItem("two")) { … endTabItem(); }
         *     endTabBar();
         *   }
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiBeginTabBar(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_tab_bar")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginTabBar(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiEndTabBar(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTabBar();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBeginTabItem(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_tab_item")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginTabItem(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiEndTabItem(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTabItem();
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Docking. Requires ImGuiConfigFlags_DockingEnable on the
         * IO before any NewFrame; the engine-side wrapper exposes
         * enableDocking() to set it once at startup.
         *
         * dockSpaceOverViewport() is the easy mode: makes the entire
         * platform window a dockspace. Subsequent regular Begin() windows
         * automatically become dockable into it.
         *
         * dockSpace(id, w, h) spawns a dockspace inside an existing window
         * (use after a normal Begin()).
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiEnableDocking(void*, MTypeContext* ctx,
                                          const MTypeValue* const*, int)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiDockSpaceOverViewport(void*, MTypeContext* ctx,
                                                  const MTypeValue* const*, int)
        {
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiDockSpace(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_dock_space")) {
                return g_host->makeVoid(ctx);
            }
            const char* id = getStr(args[0]);
            ImVec2 size(static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2])));
            ImGui::DockSpace(ImGui::GetID(id), size);
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3: Splitter (hand-rolled — no imgui_internal dependency).
         *
         * Creates an InvisibleButton sized to (thickness × full available
         * height) for vertical splits, or (full width × thickness) for
         * horizontal. While dragged, redistributes the delta between
         * size1 and size2, clamping at min1/min2.
         *
         * Returns float[2] = [newSize1, newSize2]. The bool change-flag
         * is also written to g_lastWidgetChanged so widgetChanged() works
         * for splitters too.
         *
         * Caller pattern:
         *   float[] sz = ImGui.splitter(true, 4.0, leftW, rightW, 100.0, 100.0);
         *   leftW = sz[0]; rightW = sz[1];
         *   if (ImGui.beginChild("##L", leftW, 0.0, false)) { ... } ImGui.endChild();
         *   ImGui.sameLine();
         *   if (ImGui.beginChild("##R", rightW, 0.0, false)) { ... } ImGui.endChild();
         * ---------------------------------------------------------------- */

        /* ----------------------------------------------------------------
         * Phase 4: textures (Image), clipboard, fonts.
         * ---------------------------------------------------------------- */

        /* ImGui::Image overload provided by imgui-sfml takes a
         * const sf::Texture&. Pass the texture id from
         * __native__sfml_texture_load_from_file (or any other source). */
        MTypeValue* nImGuiImage(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_image")) {
                return g_host->makeVoid(ctx);
            }
            int64_t texId = g_host->getInt(args[0]);
            sf::Texture* tex = g_textures.find(texId);
            if (!tex) return g_host->makeVoid(ctx);
            sf::Vector2f size(static_cast<float>(g_host->getFloat(args[1])),
                              static_cast<float>(g_host->getFloat(args[2])));
            ImGui::Image(*tex, size);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSetClipboardText(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_set_clipboard_text")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::SetClipboardText(getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiGetClipboardText(void*, MTypeContext* ctx,
                                             const MTypeValue* const*, int)
        {
            const char* s = ImGui::GetClipboardText();
            if (!s) return g_host->makeString(ctx, "", 0);
            return g_host->makeString(ctx, s, std::strlen(s));
        }

        /* Fonts. AddFontFromFileTTF must be called BEFORE the first
         * ImGui::SFML::Update() that uses the font; we call
         * ImGui::SFML::UpdateFontTexture() right after adding so the
         * backend rebuilds its atlas. Returns the font handle (0 on fail). */
        MTypeValue* nImGuiAddFontFromFile(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_add_font_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            float size = static_cast<float>(g_host->getFloat(args[1]));
            ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, size);
            if (!font) {
                std::string m = std::string("__native__imgui_add_font_from_file: failed to load '")
                              + path + "'";
                g_host->raiseError(ctx, kEx, m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            (void)ImGui::SFML::UpdateFontTexture();
            return g_host->makeInt(ctx, g_imguiFonts.insert(font));
        }
        MTypeValue* nImGuiPushFont(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_push_font")) {
                return g_host->makeVoid(ctx);
            }
            ImFont* font = g_imguiFonts.find(g_host->getInt(args[0]));
            if (font) ImGui::PushFont(font);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPopFont(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int)
        {
            ImGui::PopFont();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSplitter(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__imgui_splitter")) {
                return g_host->makeNull(ctx);
            }
            bool vertical = g_host->getBool(args[0]) != 0;
            float thickness = static_cast<float>(g_host->getFloat(args[1]));
            float size1 = static_cast<float>(g_host->getFloat(args[2]));
            float size2 = static_cast<float>(g_host->getFloat(args[3]));
            float min1  = static_cast<float>(g_host->getFloat(args[4]));
            float min2  = static_cast<float>(g_host->getFloat(args[5]));

            ImVec2 btnSize = vertical ? ImVec2(thickness, ImGui::GetContentRegionAvail().y)
                                       : ImVec2(ImGui::GetContentRegionAvail().x, thickness);

            /* Style the invisible button to look like Separator. */
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImGui::GetStyleColorVec4(ImGuiCol_Separator));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImGui::GetStyleColorVec4(ImGuiCol_SeparatorHovered));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                ImGui::GetStyleColorVec4(ImGuiCol_SeparatorActive));
            ImGui::Button(vertical ? "##vsplitter" : "##hsplitter", btnSize);
            ImGui::PopStyleColor(3);

            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW
                                                : ImGuiMouseCursor_ResizeNS);
            }

            bool changed = false;
            if (ImGui::IsItemActive()) {
                float delta = vertical ? ImGui::GetIO().MouseDelta.x
                                        : ImGui::GetIO().MouseDelta.y;
                if (delta != 0.0f) {
                    float ns1 = std::max(size1 + delta, min1);
                    float ns2 = std::max(size2 - delta, min2);
                    /* Only commit if both clamps were honored. */
                    if (ns1 + ns2 == size1 + size2) {
                        size1 = ns1;
                        size2 = ns2;
                        changed = true;
                    }
                }
            }
            g_lastWidgetChanged = changed;

            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, size1));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, size2));
            return out;
        }

        /* ----------------------------------------------------------------
         * Phase 7-A: interactivity queries, ID stack, ProgressBar.
         *
         * All "is item / is window" queries take zero args and read state
         * for the *previously submitted* widget (item) or *current* window
         * — same lifetime rules as native ImGui. Mouse and key queries
         * accept their target as an int constant (button index or
         * ImGuiKey value); the key constants are exposed as a family of
         * tiny `__native__imgui_key_*_id` natives registered below.
         * ---------------------------------------------------------------- */

        MTypeValue* nImGuiIsItemHovered(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsItemHovered() ? 1 : 0);
        }
        MTypeValue* nImGuiIsItemActive(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsItemActive() ? 1 : 0);
        }
        MTypeValue* nImGuiIsItemFocused(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsItemFocused() ? 1 : 0);
        }
        MTypeValue* nImGuiIsItemClicked(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsItemClicked() ? 1 : 0);
        }
        MTypeValue* nImGuiIsItemEdited(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsItemEdited() ? 1 : 0);
        }
        MTypeValue* nImGuiIsWindowHovered(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsWindowHovered() ? 1 : 0);
        }
        MTypeValue* nImGuiIsWindowFocused(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::IsWindowFocused() ? 1 : 0);
        }

        MTypeValue* nImGuiGetWindowSize(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImVec2 s = ImGui::GetWindowSize();
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, s.x));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, s.y));
            return out;
        }
        MTypeValue* nImGuiGetWindowPos(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImVec2 p = ImGui::GetWindowPos();
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, p.x));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, p.y));
            return out;
        }
        MTypeValue* nImGuiGetMousePos(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImVec2 p = ImGui::GetMousePos();
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, p.x));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, p.y));
            return out;
        }

        MTypeValue* nImGuiIsMouseClicked(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_is_mouse_clicked")) {
                return g_host->makeBool(ctx, 0);
            }
            int btn = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, ImGui::IsMouseClicked(btn) ? 1 : 0);
        }
        MTypeValue* nImGuiIsMouseDown(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_is_mouse_down")) {
                return g_host->makeBool(ctx, 0);
            }
            int btn = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, ImGui::IsMouseDown(btn) ? 1 : 0);
        }
        MTypeValue* nImGuiIsKeyPressed(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_is_key_pressed")) {
                return g_host->makeBool(ctx, 0);
            }
            int k = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, ImGui::IsKeyPressed(static_cast<ImGuiKey>(k)) ? 1 : 0);
        }
        MTypeValue* nImGuiIsKeyDown(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_is_key_down")) {
                return g_host->makeBool(ctx, 0);
            }
            int k = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, ImGui::IsKeyDown(static_cast<ImGuiKey>(k)) ? 1 : 0);
        }

        /* ID stack — required for any list that draws widgets with
         * duplicate labels. PushID pairs with PopID. */
        MTypeValue* nImGuiPushIdStr(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_push_id_str")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::PushID(getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPushIdInt(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_push_id_int")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::PushID(static_cast<int>(g_host->getInt(args[0])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiPopId(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::PopID();
            return g_host->makeVoid(ctx);
        }

        /* ProgressBar. Pass width<0 / height<0 to take available width /
         * default height respectively (-1.0 here triggers ImGui's
         * "use default" path; native API uses -FLT_MIN, but -1 is below
         * any plausible pixel size and serves the same role). */
        MTypeValue* nImGuiProgressBar(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__imgui_progress_bar")) {
                return g_host->makeVoid(ctx);
            }
            float frac = static_cast<float>(g_host->getFloat(args[0]));
            float w    = static_cast<float>(g_host->getFloat(args[1]));
            float h    = static_cast<float>(g_host->getFloat(args[2]));
            const char* overlay = getStr(args[3]);
            ImGui::ProgressBar(frac, ImVec2(w, h), (overlay && overlay[0]) ? overlay : nullptr);
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 7-B: menus, tooltips, trees, text variants, numeric
         * inputs, layout precision.
         *
         * Main menu bars are top-level (no containing window required).
         * In-window menu bars need the window to carry the MenuBar flag
         * at Begin time — `beginMenuBarWindow` opens such a window.
         * ---------------------------------------------------------------- */

        /* Menus. */
        MTypeValue* nImGuiBeginMainMenuBar(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::BeginMainMenuBar() ? 1 : 0);
        }
        MTypeValue* nImGuiEndMainMenuBar(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndMainMenuBar();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBeginMenuBarWindow(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_menu_bar_window")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* name = getStr(args[0]);
            return g_host->makeBool(ctx,
                ImGui::Begin(name, nullptr, ImGuiWindowFlags_MenuBar) ? 1 : 0);
        }
        MTypeValue* nImGuiBeginMenuBar(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::BeginMenuBar() ? 1 : 0);
        }
        MTypeValue* nImGuiEndMenuBar(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndMenuBar();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBeginMenu(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin_menu")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::BeginMenu(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiEndMenu(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndMenu();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiMenuItem(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_menu_item")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* label = getStr(args[0]);
            const char* shortcut = getStr(args[1]);
            return g_host->makeBool(ctx,
                ImGui::MenuItem(label, (shortcut && shortcut[0]) ? shortcut : nullptr) ? 1 : 0);
        }
        /* Returns bool[2] = [clicked, newChecked]. */
        MTypeValue* nImGuiMenuItemCheck(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_menu_item_check")) {
                return g_host->makeNull(ctx);
            }
            const char* label = getStr(args[0]);
            const char* shortcut = getStr(args[1]);
            bool sel = g_host->getBool(args[2]) != 0;
            bool clicked = ImGui::MenuItem(label,
                                           (shortcut && shortcut[0]) ? shortcut : nullptr,
                                           &sel);
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_BOOL, 2);
            g_host->arraySet(out, 0, g_host->makeBool(ctx, clicked ? 1 : 0));
            g_host->arraySet(out, 1, g_host->makeBool(ctx, sel ? 1 : 0));
            return out;
        }

        /* Tooltips. */
        MTypeValue* nImGuiSetTooltip(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_set_tooltip")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::SetTooltip("%s", getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiBeginTooltip(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            return g_host->makeBool(ctx, ImGui::BeginTooltip() ? 1 : 0);
        }
        MTypeValue* nImGuiEndTooltip(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndTooltip();
            return g_host->makeVoid(ctx);
        }

        /* Text variants. */
        MTypeValue* nImGuiTextColored(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__imgui_text_colored")) {
                return g_host->makeVoid(ctx);
            }
            ImVec4 c(static_cast<float>(g_host->getFloat(args[0])),
                     static_cast<float>(g_host->getFloat(args[1])),
                     static_cast<float>(g_host->getFloat(args[2])),
                     static_cast<float>(g_host->getFloat(args[3])));
            ImGui::TextColored(c, "%s", getStr(args[4]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTextWrapped(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_text_wrapped")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::TextWrapped("%s", getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiTextDisabled(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_text_disabled")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::TextDisabled("%s", getStr(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiLabelText(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_label_text")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::LabelText(getStr(args[0]), "%s", getStr(args[1]));
            return g_host->makeVoid(ctx);
        }

        /* Trees. */
        MTypeValue* nImGuiTreeNode(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_tree_node")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::TreeNode(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiTreePop(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::TreePop();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiCollapsingHeader(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_collapsing_header")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui::CollapsingHeader(getStr(args[0])) ? 1 : 0);
        }
        MTypeValue* nImGuiSelectable(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_selectable")) {
                return g_host->makeBool(ctx, 0);
            }
            const char* label = getStr(args[0]);
            bool sel = g_host->getBool(args[1]) != 0;
            bool clicked = ImGui::Selectable(label, sel);
            g_lastWidgetChanged = clicked;
            return g_host->makeBool(ctx, clicked ? 1 : 0);
        }

        /* Numeric inputs. */
        MTypeValue* nImGuiInputFloat(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_input_float")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            const char* label = getStr(args[0]);
            float v = static_cast<float>(g_host->getFloat(args[1]));
            g_lastWidgetChanged = ImGui::InputFloat(label, &v);
            return g_host->makeFloat(ctx, static_cast<double>(v));
        }
        MTypeValue* nImGuiInputInt(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_input_int")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* label = getStr(args[0]);
            int v = static_cast<int>(g_host->getInt(args[1]));
            g_lastWidgetChanged = ImGui::InputInt(label, &v);
            return g_host->makeInt(ctx, static_cast<int64_t>(v));
        }
        MTypeValue* nImGuiDragFloat(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_drag_float")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            const char* label = getStr(args[0]);
            float v = static_cast<float>(g_host->getFloat(args[1]));
            float speed = static_cast<float>(g_host->getFloat(args[2]));
            g_lastWidgetChanged = ImGui::DragFloat(label, &v, speed);
            return g_host->makeFloat(ctx, static_cast<double>(v));
        }
        MTypeValue* nImGuiDragInt(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__imgui_drag_int")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* label = getStr(args[0]);
            int v = static_cast<int>(g_host->getInt(args[1]));
            float speed = static_cast<float>(g_host->getFloat(args[2]));
            g_lastWidgetChanged = ImGui::DragInt(label, &v, speed);
            return g_host->makeInt(ctx, static_cast<int64_t>(v));
        }
        MTypeValue* nImGuiInputTextMultiline(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__imgui_input_text_multiline")) {
                return g_host->makeString(ctx, "", 0);
            }
            const char* label = getStr(args[0]);
            size_t curLen = 0;
            const char* curStr = getStr(args[1], &curLen);
            int requestedCap = static_cast<int>(g_host->getInt(args[2]));
            float w = static_cast<float>(g_host->getFloat(args[3]));
            float h = static_cast<float>(g_host->getFloat(args[4]));

            constexpr size_t kMinCap = 256;
            constexpr size_t kMaxCap = 1 << 16;
            size_t cap = static_cast<size_t>(std::max(0, requestedCap));
            if (cap < kMinCap) cap = kMinCap;
            if (cap > kMaxCap) cap = kMaxCap;

            std::vector<char> buf(cap, 0);
            size_t copy = std::min(curLen, cap - 1);
            std::memcpy(buf.data(), curStr, copy);
            buf[copy] = '\0';

            g_lastWidgetChanged = ImGui::InputTextMultiline(label, buf.data(), cap, ImVec2(w, h));
            return g_host->makeString(ctx, buf.data(), std::strlen(buf.data()));
        }

        /* Layout precision. */
        MTypeValue* nImGuiBeginGroup(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::BeginGroup();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiEndGroup(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImGui::EndGroup();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiIndent(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_indent")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::Indent(static_cast<float>(g_host->getFloat(args[0])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiUnindent(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_unindent")) {
                return g_host->makeVoid(ctx);
            }
            ImGui::Unindent(static_cast<float>(g_host->getFloat(args[0])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiGetCursorPos(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            ImVec2 p = ImGui::GetCursorPos();
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
            g_host->arraySet(out, 0, g_host->makeFloat(ctx, p.x));
            g_host->arraySet(out, 1, g_host->makeFloat(ctx, p.y));
            return out;
        }
        MTypeValue* nImGuiSetCursorPos(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_set_cursor_pos")) {
                return g_host->makeVoid(ctx);
            }
            ImVec2 p(static_cast<float>(g_host->getFloat(args[0])),
                     static_cast<float>(g_host->getFloat(args[1])));
            ImGui::SetCursorPos(p);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImGuiDummy(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_dummy")) {
                return g_host->makeVoid(ctx);
            }
            ImVec2 s(static_cast<float>(g_host->getFloat(args[0])),
                     static_cast<float>(g_host->getFloat(args[1])));
            ImGui::Dummy(s);
            return g_host->makeVoid(ctx);
        }
    }

    void registerImGuiNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };
        reg("__native__imgui_init",                    &nImGuiInit);
        reg("__native__imgui_shutdown",                &nImGuiShutdown);
        reg("__native__imgui_process_sfml_event",      &nImGuiProcessSfmlEvent);
        reg("__native__imgui_update",                  &nImGuiUpdate);
        reg("__native__imgui_render",                  &nImGuiRender);
        reg("__native__imgui_begin",                   &nImGuiBegin);
        reg("__native__imgui_begin_closable",          &nImGuiBeginClosable);
        reg("__native__imgui_set_next_window_pos",     &nImGuiSetNextWindowPos);
        reg("__native__imgui_set_next_window_size",    &nImGuiSetNextWindowSize);
        reg("__native__imgui_begin_fixed",             &nImGuiBeginFixed);
        reg("__native__imgui_end",                     &nImGuiEnd);
        reg("__native__imgui_text",                    &nImGuiText);
        reg("__native__imgui_button",                  &nImGuiButton);

        /* Phase 2 — input widgets */
        reg("__native__imgui_widget_changed",          &nImGuiWidgetChanged);
        reg("__native__imgui_slider_float",            &nImGuiSliderFloat);
        reg("__native__imgui_slider_int",              &nImGuiSliderInt);
        reg("__native__imgui_checkbox",                &nImGuiCheckbox);
        reg("__native__imgui_combo",                   &nImGuiCombo);
        reg("__native__imgui_input_text",              &nImGuiInputText);
        reg("__native__imgui_color_edit3",             &nImGuiColorEdit3);

        /* Phase 2 — layout helpers */
        reg("__native__imgui_same_line",               &nImGuiSameLine);
        reg("__native__imgui_separator",               &nImGuiSeparator);
        reg("__native__imgui_spacing",                 &nImGuiSpacing);
        reg("__native__imgui_bullet_text",             &nImGuiBulletText);

        /* Phase 3 — Tables */
        reg("__native__imgui_begin_table",             &nImGuiBeginTable);
        reg("__native__imgui_end_table",               &nImGuiEndTable);
        reg("__native__imgui_table_setup_column",      &nImGuiTableSetupColumn);
        reg("__native__imgui_table_headers_row",       &nImGuiTableHeadersRow);
        reg("__native__imgui_table_next_row",          &nImGuiTableNextRow);
        reg("__native__imgui_table_next_column",       &nImGuiTableNextColumn);
        reg("__native__imgui_table_set_column_index",  &nImGuiTableSetColumnIndex);

        /* Phase 3 — Child windows */
        reg("__native__imgui_begin_child",             &nImGuiBeginChild);
        reg("__native__imgui_end_child",               &nImGuiEndChild);

        /* Phase 3 — Tab bars */
        reg("__native__imgui_begin_tab_bar",           &nImGuiBeginTabBar);
        reg("__native__imgui_end_tab_bar",             &nImGuiEndTabBar);
        reg("__native__imgui_begin_tab_item",          &nImGuiBeginTabItem);
        reg("__native__imgui_end_tab_item",            &nImGuiEndTabItem);

        /* Phase 3 — Docking */
        reg("__native__imgui_enable_docking",          &nImGuiEnableDocking);
        reg("__native__imgui_dock_space_over_viewport", &nImGuiDockSpaceOverViewport);
        reg("__native__imgui_dock_space",              &nImGuiDockSpace);

        /* Phase 3 — Splitter */
        reg("__native__imgui_splitter",                &nImGuiSplitter);

        /* Phase 4 — image, clipboard, fonts */
        reg("__native__imgui_image",                   &nImGuiImage);
        reg("__native__imgui_set_clipboard_text",      &nImGuiSetClipboardText);
        reg("__native__imgui_get_clipboard_text",      &nImGuiGetClipboardText);
        reg("__native__imgui_add_font_from_file",      &nImGuiAddFontFromFile);
        reg("__native__imgui_push_font",               &nImGuiPushFont);
        reg("__native__imgui_pop_font",                &nImGuiPopFont);

        /* Phase 6 — popups */
        reg("__native__imgui_open_popup",                  &nImGuiOpenPopup);
        reg("__native__imgui_begin_popup",                 &nImGuiBeginPopup);
        reg("__native__imgui_begin_popup_modal",           &nImGuiBeginPopupModal);
        reg("__native__imgui_begin_popup_context_item",    &nImGuiBeginPopupContextItem);
        reg("__native__imgui_begin_popup_context_window",  &nImGuiBeginPopupContextWindow);
        reg("__native__imgui_end_popup",                   &nImGuiEndPopup);
        reg("__native__imgui_close_current_popup",         &nImGuiCloseCurrentPopup);

        /* Phase 6 — styles */
        reg("__native__imgui_style_dark",                  &nImGuiStyleDark);
        reg("__native__imgui_style_light",                 &nImGuiStyleLight);
        reg("__native__imgui_style_classic",               &nImGuiStyleClassic);
        reg("__native__imgui_push_style_color",            &nImGuiPushStyleColor);
        reg("__native__imgui_pop_style_color",             &nImGuiPopStyleColor);
        reg("__native__imgui_push_style_var_float",        &nImGuiPushStyleVarFloat);
        reg("__native__imgui_push_style_var_vec2",         &nImGuiPushStyleVarVec2);
        reg("__native__imgui_pop_style_var",               &nImGuiPopStyleVar);

        /* Phase 7-A — interactivity queries */
        reg("__native__imgui_is_item_hovered",   &nImGuiIsItemHovered);
        reg("__native__imgui_is_item_active",    &nImGuiIsItemActive);
        reg("__native__imgui_is_item_focused",   &nImGuiIsItemFocused);
        reg("__native__imgui_is_item_clicked",   &nImGuiIsItemClicked);
        reg("__native__imgui_is_item_edited",    &nImGuiIsItemEdited);
        reg("__native__imgui_is_window_hovered", &nImGuiIsWindowHovered);
        reg("__native__imgui_is_window_focused", &nImGuiIsWindowFocused);
        reg("__native__imgui_get_window_size",   &nImGuiGetWindowSize);
        reg("__native__imgui_get_window_pos",    &nImGuiGetWindowPos);
        reg("__native__imgui_get_mouse_pos",     &nImGuiGetMousePos);
        reg("__native__imgui_is_mouse_clicked",  &nImGuiIsMouseClicked);
        reg("__native__imgui_is_mouse_down",     &nImGuiIsMouseDown);
        reg("__native__imgui_is_key_pressed",    &nImGuiIsKeyPressed);
        reg("__native__imgui_is_key_down",       &nImGuiIsKeyDown);

        /* Phase 7-A — ID stack */
        reg("__native__imgui_push_id_str", &nImGuiPushIdStr);
        reg("__native__imgui_push_id_int", &nImGuiPushIdInt);
        reg("__native__imgui_pop_id",      &nImGuiPopId);

        /* Phase 7-A — progress bar */
        reg("__native__imgui_progress_bar", &nImGuiProgressBar);

        /* Phase 7-A — ImGuiKey constant getters. Each is a tiny
         * non-capturing lambda that returns the ImGuiKey int. Names
         * follow __native__imgui_key_<lower>_id. */
        #define REG_KEY(NAME_LOWER, IMKEY) \
            reg("__native__imgui_key_" NAME_LOWER "_id", \
                +[](void*, MTypeContext* c, const MTypeValue* const*, int) -> MTypeValue* { \
                    return g_host->makeInt(c, static_cast<int64_t>(IMKEY)); })
        REG_KEY("escape",     ImGuiKey_Escape);
        REG_KEY("space",      ImGuiKey_Space);
        REG_KEY("enter",      ImGuiKey_Enter);
        REG_KEY("tab",        ImGuiKey_Tab);
        REG_KEY("backspace",  ImGuiKey_Backspace);
        REG_KEY("delete",     ImGuiKey_Delete);
        REG_KEY("insert",     ImGuiKey_Insert);
        REG_KEY("home",       ImGuiKey_Home);
        REG_KEY("end",        ImGuiKey_End);
        REG_KEY("page_up",    ImGuiKey_PageUp);
        REG_KEY("page_down",  ImGuiKey_PageDown);
        REG_KEY("left",       ImGuiKey_LeftArrow);
        REG_KEY("right",      ImGuiKey_RightArrow);
        REG_KEY("up",         ImGuiKey_UpArrow);
        REG_KEY("down",       ImGuiKey_DownArrow);
        REG_KEY("left_ctrl",  ImGuiKey_LeftCtrl);
        REG_KEY("left_shift", ImGuiKey_LeftShift);
        REG_KEY("left_alt",   ImGuiKey_LeftAlt);
        REG_KEY("left_super", ImGuiKey_LeftSuper);
        REG_KEY("right_ctrl", ImGuiKey_RightCtrl);
        REG_KEY("right_shift",ImGuiKey_RightShift);
        REG_KEY("right_alt",  ImGuiKey_RightAlt);
        REG_KEY("right_super",ImGuiKey_RightSuper);
        REG_KEY("a", ImGuiKey_A); REG_KEY("b", ImGuiKey_B); REG_KEY("c", ImGuiKey_C);
        REG_KEY("d", ImGuiKey_D); REG_KEY("e", ImGuiKey_E); REG_KEY("f", ImGuiKey_F);
        REG_KEY("g", ImGuiKey_G); REG_KEY("h", ImGuiKey_H); REG_KEY("i", ImGuiKey_I);
        REG_KEY("j", ImGuiKey_J); REG_KEY("k", ImGuiKey_K); REG_KEY("l", ImGuiKey_L);
        REG_KEY("m", ImGuiKey_M); REG_KEY("n", ImGuiKey_N); REG_KEY("o", ImGuiKey_O);
        REG_KEY("p", ImGuiKey_P); REG_KEY("q", ImGuiKey_Q); REG_KEY("r", ImGuiKey_R);
        REG_KEY("s", ImGuiKey_S); REG_KEY("t", ImGuiKey_T); REG_KEY("u", ImGuiKey_U);
        REG_KEY("v", ImGuiKey_V); REG_KEY("w", ImGuiKey_W); REG_KEY("x", ImGuiKey_X);
        REG_KEY("y", ImGuiKey_Y); REG_KEY("z", ImGuiKey_Z);
        REG_KEY("f1",  ImGuiKey_F1);  REG_KEY("f2",  ImGuiKey_F2);
        REG_KEY("f3",  ImGuiKey_F3);  REG_KEY("f4",  ImGuiKey_F4);
        REG_KEY("f5",  ImGuiKey_F5);  REG_KEY("f6",  ImGuiKey_F6);
        REG_KEY("f7",  ImGuiKey_F7);  REG_KEY("f8",  ImGuiKey_F8);
        REG_KEY("f9",  ImGuiKey_F9);  REG_KEY("f10", ImGuiKey_F10);
        REG_KEY("f11", ImGuiKey_F11); REG_KEY("f12", ImGuiKey_F12);
        #undef REG_KEY

        /* Phase 7-B — menus */
        reg("__native__imgui_begin_main_menu_bar",   &nImGuiBeginMainMenuBar);
        reg("__native__imgui_end_main_menu_bar",     &nImGuiEndMainMenuBar);
        reg("__native__imgui_begin_menu_bar_window", &nImGuiBeginMenuBarWindow);
        reg("__native__imgui_begin_menu_bar",        &nImGuiBeginMenuBar);
        reg("__native__imgui_end_menu_bar",          &nImGuiEndMenuBar);
        reg("__native__imgui_begin_menu",            &nImGuiBeginMenu);
        reg("__native__imgui_end_menu",              &nImGuiEndMenu);
        reg("__native__imgui_menu_item",             &nImGuiMenuItem);
        reg("__native__imgui_menu_item_check",       &nImGuiMenuItemCheck);

        /* Phase 7-B — tooltips */
        reg("__native__imgui_set_tooltip",   &nImGuiSetTooltip);
        reg("__native__imgui_begin_tooltip", &nImGuiBeginTooltip);
        reg("__native__imgui_end_tooltip",   &nImGuiEndTooltip);

        /* Phase 7-B — text variants */
        reg("__native__imgui_text_colored",  &nImGuiTextColored);
        reg("__native__imgui_text_wrapped",  &nImGuiTextWrapped);
        reg("__native__imgui_text_disabled", &nImGuiTextDisabled);
        reg("__native__imgui_label_text",    &nImGuiLabelText);

        /* Phase 7-B — trees */
        reg("__native__imgui_tree_node",          &nImGuiTreeNode);
        reg("__native__imgui_tree_pop",           &nImGuiTreePop);
        reg("__native__imgui_collapsing_header",  &nImGuiCollapsingHeader);
        reg("__native__imgui_selectable",         &nImGuiSelectable);

        /* Phase 7-B — numeric inputs */
        reg("__native__imgui_input_float",          &nImGuiInputFloat);
        reg("__native__imgui_input_int",            &nImGuiInputInt);
        reg("__native__imgui_drag_float",           &nImGuiDragFloat);
        reg("__native__imgui_drag_int",             &nImGuiDragInt);
        reg("__native__imgui_input_text_multiline", &nImGuiInputTextMultiline);

        /* Phase 7-B — layout precision */
        reg("__native__imgui_begin_group",    &nImGuiBeginGroup);
        reg("__native__imgui_end_group",      &nImGuiEndGroup);
        reg("__native__imgui_indent",         &nImGuiIndent);
        reg("__native__imgui_unindent",       &nImGuiUnindent);
        reg("__native__imgui_get_cursor_pos", &nImGuiGetCursorPos);
        reg("__native__imgui_set_cursor_pos", &nImGuiSetCursorPos);
        reg("__native__imgui_dummy",          &nImGuiDummy);
    }
}
