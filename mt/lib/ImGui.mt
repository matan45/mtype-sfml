// mType wrapper around the __native__imgui_* natives exposed by
// mtype-sfml (via imgui-sfml). ImGui's context + platform/renderer
// backends are owned by ImGui::SFML — no separate ImGuiContext handle
// on the mType side. Loop pattern:
//
//   ImGui::init(window);
//   while (window.isOpen()) {
//       int ev = window.pollEvent();
//       while (ev != 0) {
//           ImGui::processSfmlEvent(window);
//           ...
//           ev = window.pollEvent();
//       }
//       ImGui::update(window, dtMs);
//       ...widget calls...
//       window.clear(...);
//       ...world draws...
//       ImGui::render(window);
//       window.display();
//   }
//   ImGui::shutdown();

import * from "Graphics.mt";

class ImGui {
    public static function init(RenderWindow window): bool {
        return __native__imgui_init(window.handle);
    }
    public static function shutdown(): void {
        __native__imgui_shutdown();
    }

    // Forward the most-recent SFML event into ImGui's input pipeline.
    public static function processSfmlEvent(RenderWindow window): void {
        __native__imgui_process_sfml_event(window.handle);
    }

    // Combines NewFrame + ImGui::SFML clock advance. dtMs is milliseconds
    // since the last update (use Timing if you add it, or hardcode 16.0).
    public static function update(RenderWindow window, float dtMs): void {
        __native__imgui_update(window.handle, dtMs);
    }

    public static function render(RenderWindow window): void {
        __native__imgui_render(window.handle);
    }

    // Begin a top-level window. Always pair with end().
    public static function begin(string title): bool {
        return __native__imgui_begin(title);
    }

    public static function setNextWindowPos(float x, float y): void {
        __native__imgui_set_next_window_pos(x, y);
    }

    public static function setNextWindowSize(float width, float height): void {
        __native__imgui_set_next_window_size(width, height);
    }

    // Fixed overlay window: no title bar, no resizing/moving/collapsing,
    // and no persisted imgui.ini placement.
    public static function beginFixed(string title): bool {
        return __native__imgui_begin_fixed(title);
    }

    public static function end(): void {
        __native__imgui_end();
    }

    // Top-level window WITH the X close button. Returns bool[2]:
    //   ret[0] = shouldDraw  (true if Begin's body should run; false when
    //                         the window is collapsed/clipped — but still
    //                         call end() either way)
    //   ret[1] = newOpen     (false on the frame the user clicks X)
    //
    // Pattern:
    //   if (myWindowOpen) {
    //     bool[] s = ImGui::beginClosable("Title", myWindowOpen);
    //     if (s[0]) { ... widgets ... }
    //     ImGui::end();
    //     myWindowOpen = s[1];
    //   }
    public static function beginClosable(string title, bool currentOpen): bool[] {
        return __native__imgui_begin_closable(title, currentOpen);
    }

    public static function text(string label): void {
        __native__imgui_text(label);
    }

    // Draw a labeled button. Returns true on the frame the user clicks it.
    public static function button(string label): bool {
        return __native__imgui_button(label);
    }

    // ------------------------------------------------------------------
    // Phase 2 — input widgets.
    //
    // Widgets that mutate a value follow the same pattern: pass the
    // current value, the call returns the (possibly modified) new value.
    // To detect "did the user touch this widget THIS frame" (and not
    // just "current != previous"), call ImGui.widgetChanged() after the
    // widget call — it's true on the frame the widget mutated its value.
    // ------------------------------------------------------------------

    public static function widgetChanged(): bool {
        return __native__imgui_widget_changed();
    }

    public static function sliderFloat(string label, float current, float min, float max): float {
        return __native__imgui_slider_float(label, current, min, max);
    }

    public static function sliderInt(string label, int current, int min, int max): int {
        return __native__imgui_slider_int(label, current, min, max);
    }

    public static function checkbox(string label, bool current): bool {
        return __native__imgui_checkbox(label, current);
    }

    // Combo dropdown. items is a string[] of options.
    // Returns the new selected index (== currentIdx if unchanged).
    public static function combo(string label, int currentIdx, string[] items): int {
        return __native__imgui_combo(label, currentIdx, items);
    }

    // Single-line text edit. maxCapacity is the buffer size the plugin
    // allocates (clamped to [256, 65536] internally). Returns the
    // current contents — equal to `current` on frames the user didn't
    // edit.
    public static function inputText(string label, string current, int maxCapacity): string {
        return __native__imgui_input_text(label, current, maxCapacity);
    }

    // RGB color edit. Each component is in [0,1]. Returns a new
    // float[3] — index 0=R, 1=G, 2=B.
    public static function colorEdit3(string label, float r, float g, float b): float[] {
        return __native__imgui_color_edit3(label, r, g, b);
    }

    // ------------------------------------------------------------------
    // Phase 2 — layout helpers.
    // ------------------------------------------------------------------

    public static function sameLine():  void { __native__imgui_same_line(); }
    public static function separator(): void { __native__imgui_separator(); }
    public static function spacing():   void { __native__imgui_spacing(); }
    public static function bulletText(string s): void { __native__imgui_bullet_text(s); }

    // ------------------------------------------------------------------
    // Phase 3 — layout: tables, child windows, tabs, docking, splitter.
    // ------------------------------------------------------------------

    // Tables. Pattern:
    //   if (ImGui::beginTable("id", 3)) {
    //     ImGui::tableSetupColumn("a"); ImGui::tableSetupColumn("b"); ImGui::tableSetupColumn("c");
    //     ImGui::tableHeadersRow();
    //     ImGui::tableNextRow(); ImGui::tableNextColumn(); ImGui::text("...");
    //     ImGui::endTable();
    //   }
    public static function beginTable(string id, int columns): bool {
        return __native__imgui_begin_table(id, columns);
    }
    public static function endTable(): void { __native__imgui_end_table(); }
    public static function tableSetupColumn(string label): void {
        __native__imgui_table_setup_column(label);
    }
    public static function tableHeadersRow(): void { __native__imgui_table_headers_row(); }
    public static function tableNextRow():    void { __native__imgui_table_next_row(); }
    public static function tableNextColumn(): bool { return __native__imgui_table_next_column(); }
    public static function tableSetColumnIndex(int idx): bool {
        return __native__imgui_table_set_column_index(idx);
    }

    // Child windows. Always call endChild even if beginChild returned false.
    // Pass 0.0 for w/h to take the available region in that axis.
    public static function beginChild(string id, float width, float height, bool border): bool {
        return __native__imgui_begin_child(id, width, height, border);
    }
    public static function endChild(): void { __native__imgui_end_child(); }

    // Tab bars.
    public static function beginTabBar(string id): bool {
        return __native__imgui_begin_tab_bar(id);
    }
    public static function endTabBar(): void { __native__imgui_end_tab_bar(); }
    public static function beginTabItem(string label): bool {
        return __native__imgui_begin_tab_item(label);
    }
    public static function endTabItem(): void { __native__imgui_end_tab_item(); }

    // Docking. Call enableDocking() once at startup BEFORE the first
    // newFrame(). Then either dockSpaceOverViewport() to make the platform
    // window a dockspace, or dockSpace(id, w, h) inside a regular window.
    public static function enableDocking(): void { __native__imgui_enable_docking(); }
    public static function dockSpaceOverViewport(): void { __native__imgui_dock_space_over_viewport(); }
    public static function dockSpace(string id, float width, float height): void {
        __native__imgui_dock_space(id, width, height);
    }

    // Splitter. Pass current size1/size2 (e.g. left-pane width, right-pane
    // width for a vertical splitter), plus minimum sizes. Returns float[2]
    // = [newSize1, newSize2]. If neither pane can shrink below its min,
    // sizes stay put.
    public static function splitter(bool vertical, float thickness,
                                     float size1, float size2,
                                     float min1, float min2): float[] {
        return __native__imgui_splitter(vertical, thickness, size1, size2, min1, min2);
    }

    // ------------------------------------------------------------------
    // Phase 4 — image, clipboard, fonts.
    // ------------------------------------------------------------------

    // Draw a Texture as an Image widget at the given size in pixels.
    public static function image(Texture texture, float width, float height): void {
        __native__imgui_image(texture.handle, width, height);
    }

    public static function setClipboardText(string text): void {
        __native__imgui_set_clipboard_text(text);
    }
    public static function getClipboardText(): string {
        return __native__imgui_get_clipboard_text();
    }

    // Add a TTF font. Must be called BEFORE the first newFrame() of any
    // frame the font is going to be used in (the SDL3 renderer backend
    // rebuilds its font texture lazily on next frame). Returns a Font
    // wrapper; pass it to pushFont/popFont to scope a section's text to
    // it.
    public static function addFontFromFile(string path, float sizePixels): ImGuiFont {
        return new ImGuiFont(__native__imgui_add_font_from_file(path, sizePixels));
    }
    public static function pushFont(ImGuiFont font): void {
        __native__imgui_push_font(font.handle);
    }
    public static function popFont(): void { __native__imgui_pop_font(); }

    // ------------------------------------------------------------------
    // Phase 6 — popups.
    //
    // Two-step trigger pattern:
    //   if (ImGui::button("Open")) { ImGui::openPopup("##my_popup"); }
    //   if (ImGui::beginPopup("##my_popup")) {
    //       ImGui::text("hello");
    //       if (ImGui::button("Close")) { ImGui::closeCurrentPopup(); }
    //       ImGui::endPopup();
    //   }
    //
    // Modal popups support an X close button — same bool[2] convention as
    // beginClosable: ret[0] = shouldDraw, ret[1] = newOpen.
    //
    // Context popups auto-trigger on right-click (no openPopup needed).
    // ------------------------------------------------------------------

    public static function openPopup(string id): void { __native__imgui_open_popup(id); }
    public static function beginPopup(string id): bool {
        return __native__imgui_begin_popup(id);
    }
    public static function beginPopupModal(string title, bool currentOpen): bool[] {
        return __native__imgui_begin_popup_modal(title, currentOpen);
    }
    public static function beginPopupContextItem(string id): bool {
        return __native__imgui_begin_popup_context_item(id);
    }
    public static function beginPopupContextWindow(string id): bool {
        return __native__imgui_begin_popup_context_window(id);
    }
    public static function endPopup(): void { __native__imgui_end_popup(); }
    public static function closeCurrentPopup(): void { __native__imgui_close_current_popup(); }

    // ------------------------------------------------------------------
    // Phase 6 — styles.
    //
    // Theme switches (dark / light / classic) reset every color at once.
    // Per-section overrides use push/pop with names like "Button" or
    // "WindowBg" — keyed by name so .mt code doesn't have to track ImGui's
    // enum re-numberings between versions. See PLUGIN_NOTES.md for the
    // full name list, or check src/ImGuiBindings.cpp's resolveColorIdx.
    //
    // Each push must be balanced by a matching pop with the right count.
    // ------------------------------------------------------------------

    public static function styleDark():    void { __native__imgui_style_dark(); }
    public static function styleLight():   void { __native__imgui_style_light(); }
    public static function styleClassic(): void { __native__imgui_style_classic(); }

    public static function pushStyleColor(string name, float r, float g, float b, float a): void {
        __native__imgui_push_style_color(name, r, g, b, a);
    }
    public static function popStyleColor(int count): void {
        __native__imgui_pop_style_color(count);
    }
    public static function pushStyleVarFloat(string name, float value): void {
        __native__imgui_push_style_var_float(name, value);
    }
    public static function pushStyleVarVec2(string name, float x, float y): void {
        __native__imgui_push_style_var_vec2(name, x, y);
    }
    public static function popStyleVar(int count): void {
        __native__imgui_pop_style_var(count);
    }

    // ------------------------------------------------------------------
    // Phase 7-A — interactivity queries.
    //
    // Item-* queries read state for the previously-submitted widget.
    // Window-* read state for the current window. Mouse buttons:
    // 0=left, 1=right, 2=middle. Key constants: use the ImGuiKey class.
    // ------------------------------------------------------------------

    public static function isItemHovered():   bool { return __native__imgui_is_item_hovered(); }
    public static function isItemActive():    bool { return __native__imgui_is_item_active(); }
    public static function isItemFocused():   bool { return __native__imgui_is_item_focused(); }
    public static function isItemClicked():   bool { return __native__imgui_is_item_clicked(); }
    public static function isItemEdited():    bool { return __native__imgui_is_item_edited(); }
    public static function isWindowHovered(): bool { return __native__imgui_is_window_hovered(); }
    public static function isWindowFocused(): bool { return __native__imgui_is_window_focused(); }

    public static function getWindowSize(): float[] { return __native__imgui_get_window_size(); }
    public static function getWindowPos():  float[] { return __native__imgui_get_window_pos(); }
    public static function getMousePos():   float[] { return __native__imgui_get_mouse_pos(); }

    public static function isMouseClicked(int button): bool {
        return __native__imgui_is_mouse_clicked(button);
    }
    public static function isMouseDown(int button): bool {
        return __native__imgui_is_mouse_down(button);
    }
    public static function isKeyPressed(int keyId): bool {
        return __native__imgui_is_key_pressed(keyId);
    }
    public static function isKeyDown(int keyId): bool {
        return __native__imgui_is_key_down(keyId);
    }

    // ID stack — every list rendering widgets with the same label must
    // wrap each item in pushID(uniqueValue) / popID(). Strings or ints
    // both work; both feed into ImGui's same internal hash.
    public static function pushID(string id):   void { __native__imgui_push_id_str(id); }
    public static function pushIDInt(int id):   void { __native__imgui_push_id_int(id); }
    public static function popID(): void { __native__imgui_pop_id(); }

    // Progress bar. fraction in [0,1]. Pass width<0 or height<0 to
    // auto-size on that axis. overlay="" suppresses the centered label.
    public static function progressBar(float fraction, float width, float height,
                                        string overlay): void {
        __native__imgui_progress_bar(fraction, width, height, overlay);
    }

    // ------------------------------------------------------------------
    // Phase 7-B — menus, tooltips, trees, text variants, numeric inputs,
    // layout precision.
    //
    // Main menu bars are top-level (no Begin needed). In-window menu
    // bars require the window to carry the MenuBar flag — open such a
    // window with beginMenuBarWindow() instead of begin().
    // ------------------------------------------------------------------

    // --- menus ---
    public static function beginMainMenuBar(): bool {
        return __native__imgui_begin_main_menu_bar();
    }
    public static function endMainMenuBar(): void { __native__imgui_end_main_menu_bar(); }

    public static function beginMenuBarWindow(string title): bool {
        return __native__imgui_begin_menu_bar_window(title);
    }
    public static function beginMenuBar(): bool { return __native__imgui_begin_menu_bar(); }
    public static function endMenuBar():   void { __native__imgui_end_menu_bar(); }

    public static function beginMenu(string label): bool {
        return __native__imgui_begin_menu(label);
    }
    public static function endMenu(): void { __native__imgui_end_menu(); }

    // Returns true on the frame the user clicks. Pass "" as shortcut to
    // hide it.
    public static function menuItem(string label, string shortcut): bool {
        return __native__imgui_menu_item(label, shortcut);
    }

    // Returns bool[2] = [clickedThisFrame, newChecked]. Same shape as
    // beginClosable / beginPopupModal.
    public static function menuItemCheck(string label, string shortcut, bool current): bool[] {
        return __native__imgui_menu_item_check(label, shortcut, current);
    }

    // --- tooltips ---
    // setTooltip is the one-shot helper: call right after the widget you
    // want the tooltip to attach to. Pair with isItemHovered() to gate it.
    public static function setTooltip(string text): void {
        __native__imgui_set_tooltip(text);
    }
    // beginTooltip / endTooltip for multi-line / multi-widget tooltips.
    public static function beginTooltip(): bool { return __native__imgui_begin_tooltip(); }
    public static function endTooltip():   void { __native__imgui_end_tooltip(); }

    // --- text variants ---
    public static function textColored(float r, float g, float b, float a, string text): void {
        __native__imgui_text_colored(r, g, b, a, text);
    }
    public static function textWrapped(string text):  void { __native__imgui_text_wrapped(text); }
    public static function textDisabled(string text): void { __native__imgui_text_disabled(text); }
    public static function labelText(string label, string text): void {
        __native__imgui_label_text(label, text);
    }

    // --- trees ---
    // treeNode returns true while the node is expanded — when true, draw
    // children, then call treePop(). collapsingHeader is similar but
    // self-renders its caret; no Pop needed.
    public static function treeNode(string label): bool {
        return __native__imgui_tree_node(label);
    }
    public static function treePop(): void { __native__imgui_tree_pop(); }
    public static function collapsingHeader(string label): bool {
        return __native__imgui_collapsing_header(label);
    }
    // Selectable returns true on the frame it was clicked. Pass the
    // current selection state in `isSelected` to draw the highlight.
    public static function selectable(string label, bool isSelected): bool {
        return __native__imgui_selectable(label, isSelected);
    }

    // --- numeric inputs ---
    public static function inputFloat(string label, float current): float {
        return __native__imgui_input_float(label, current);
    }
    public static function inputInt(string label, int current): int {
        return __native__imgui_input_int(label, current);
    }
    public static function dragFloat(string label, float current, float speed): float {
        return __native__imgui_drag_float(label, current, speed);
    }
    public static function dragInt(string label, int current, float speed): int {
        return __native__imgui_drag_int(label, current, speed);
    }
    public static function inputTextMultiline(string label, string current, int maxCapacity,
                                                float width, float height): string {
        return __native__imgui_input_text_multiline(label, current, maxCapacity, width, height);
    }

    // --- layout precision ---
    public static function beginGroup(): void { __native__imgui_begin_group(); }
    public static function endGroup():   void { __native__imgui_end_group(); }
    public static function indent(float width):   void { __native__imgui_indent(width); }
    public static function unindent(float width): void { __native__imgui_unindent(width); }
    public static function getCursorPos(): float[] { return __native__imgui_get_cursor_pos(); }
    public static function setCursorPos(float x, float y): void {
        __native__imgui_set_cursor_pos(x, y);
    }
    // Reserves a rectangle of empty space to occupy layout for the next
    // widget. Useful for fine-tuning vertical alignment.
    public static function dummy(float width, float height): void {
        __native__imgui_dummy(width, height);
    }
}

// Renamed from `Font` to `ImGuiFont` so it doesn't collide with the
// sf::Font wrapper in Graphics.mt (mType disallows duplicate class
// names — both libs would otherwise contribute a `Font` to scope).
class ImGuiFont {
    public int handle;
    public constructor(int h) { this.handle = h; }
}

// ----------------------------------------------------------------------
// Phase 7-A — ImGuiKey constant getters.
//
// ImGui has its own key enumeration distinct from SDL scancodes. Pass
// the int returned by these methods to ImGui::isKeyPressed / isKeyDown.
// Resolved once on the plugin side, so the call cost is one int read.
// ----------------------------------------------------------------------

class ImGuiKey {
    public static function escape():     int { return __native__imgui_key_escape_id(); }
    public static function space():      int { return __native__imgui_key_space_id(); }
    public static function enter():      int { return __native__imgui_key_enter_id(); }
    public static function tab():        int { return __native__imgui_key_tab_id(); }
    public static function backspace():  int { return __native__imgui_key_backspace_id(); }
    public static function delete_():    int { return __native__imgui_key_delete_id(); }
    public static function insert():     int { return __native__imgui_key_insert_id(); }
    public static function home():       int { return __native__imgui_key_home_id(); }
    public static function end_():       int { return __native__imgui_key_end_id(); }
    public static function pageUp():     int { return __native__imgui_key_page_up_id(); }
    public static function pageDown():   int { return __native__imgui_key_page_down_id(); }

    public static function left():       int { return __native__imgui_key_left_id(); }
    public static function right():      int { return __native__imgui_key_right_id(); }
    public static function up():         int { return __native__imgui_key_up_id(); }
    public static function down():       int { return __native__imgui_key_down_id(); }

    public static function leftCtrl():   int { return __native__imgui_key_left_ctrl_id(); }
    public static function leftShift():  int { return __native__imgui_key_left_shift_id(); }
    public static function leftAlt():    int { return __native__imgui_key_left_alt_id(); }
    public static function leftSuper():  int { return __native__imgui_key_left_super_id(); }
    public static function rightCtrl():  int { return __native__imgui_key_right_ctrl_id(); }
    public static function rightShift(): int { return __native__imgui_key_right_shift_id(); }
    public static function rightAlt():   int { return __native__imgui_key_right_alt_id(); }
    public static function rightSuper(): int { return __native__imgui_key_right_super_id(); }

    public static function a(): int { return __native__imgui_key_a_id(); }
    public static function b(): int { return __native__imgui_key_b_id(); }
    public static function c(): int { return __native__imgui_key_c_id(); }
    public static function d(): int { return __native__imgui_key_d_id(); }
    public static function e(): int { return __native__imgui_key_e_id(); }
    public static function f(): int { return __native__imgui_key_f_id(); }
    public static function g(): int { return __native__imgui_key_g_id(); }
    public static function h(): int { return __native__imgui_key_h_id(); }
    public static function i(): int { return __native__imgui_key_i_id(); }
    public static function j(): int { return __native__imgui_key_j_id(); }
    public static function k(): int { return __native__imgui_key_k_id(); }
    public static function l(): int { return __native__imgui_key_l_id(); }
    public static function m(): int { return __native__imgui_key_m_id(); }
    public static function n(): int { return __native__imgui_key_n_id(); }
    public static function o(): int { return __native__imgui_key_o_id(); }
    public static function p(): int { return __native__imgui_key_p_id(); }
    public static function q(): int { return __native__imgui_key_q_id(); }
    public static function r(): int { return __native__imgui_key_r_id(); }
    public static function s(): int { return __native__imgui_key_s_id(); }
    public static function t(): int { return __native__imgui_key_t_id(); }
    public static function u(): int { return __native__imgui_key_u_id(); }
    public static function v(): int { return __native__imgui_key_v_id(); }
    public static function w(): int { return __native__imgui_key_w_id(); }
    public static function x(): int { return __native__imgui_key_x_id(); }
    public static function y(): int { return __native__imgui_key_y_id(); }
    public static function z(): int { return __native__imgui_key_z_id(); }

    public static function f1():  int { return __native__imgui_key_f1_id(); }
    public static function f2():  int { return __native__imgui_key_f2_id(); }
    public static function f3():  int { return __native__imgui_key_f3_id(); }
    public static function f4():  int { return __native__imgui_key_f4_id(); }
    public static function f5():  int { return __native__imgui_key_f5_id(); }
    public static function f6():  int { return __native__imgui_key_f6_id(); }
    public static function f7():  int { return __native__imgui_key_f7_id(); }
    public static function f8():  int { return __native__imgui_key_f8_id(); }
    public static function f9():  int { return __native__imgui_key_f9_id(); }
    public static function f10(): int { return __native__imgui_key_f10_id(); }
    public static function f11(): int { return __native__imgui_key_f11_id(); }
    public static function f12(): int { return __native__imgui_key_f12_id(); }
}
