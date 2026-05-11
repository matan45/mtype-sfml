// mType wrapper around the SFML 3 window / event natives exposed by
// mtype-sfml. RenderWindow and event handling go through here; graphics
// primitives (Sprite / Shape / Text / Font) live in Graphics.mt.

class RenderWindow {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void { __native__sfml_destroy_window(this.handle); }

    public function isOpen(): bool   { return __native__sfml_window_is_open(this.handle); }
    public function close():  void   { __native__sfml_window_close(this.handle); }
    public function display(): void  { __native__sfml_window_display(this.handle); }
    public function size():   int[]  { return __native__sfml_window_size(this.handle); }

    public function clear(int r, int g, int b, int a): void {
        __native__sfml_window_clear(this.handle, r, g, b, a);
    }

    // Returns the event tag of the next pending event (compare against
    // Sfml::*EventId() constants), or 0 if no event is queued.
    public function pollEvent(): int {
        return __native__sfml_poll_event(this.handle);
    }

    // ---- settings ----
    public function setTitle(string title): void {
        __native__sfml_window_set_title(this.handle, title);
    }
    public function setPosition(int x, int y): void {
        __native__sfml_window_set_position(this.handle, x, y);
    }
    public function setSize(int w, int h): void {
        __native__sfml_window_set_size(this.handle, w, h);
    }
    public function setVsync(bool enabled): void {
        __native__sfml_window_set_vsync(this.handle, enabled);
    }
    public function setFramerateLimit(int fps): void {
        __native__sfml_window_set_framerate_limit(this.handle, fps);
    }
    public function setKeyRepeatEnabled(bool enabled): void {
        __native__sfml_window_set_key_repeat(this.handle, enabled);
    }
    public function setMouseCursorVisible(bool visible): void {
        __native__sfml_window_set_mouse_cursor_visible(this.handle, visible);
    }
    public function setMouseCursorGrabbed(bool grabbed): void {
        __native__sfml_window_set_mouse_cursor_grabbed(this.handle, grabbed);
    }
    public function hasFocus(): bool { return __native__sfml_window_has_focus(this.handle); }
    public function requestFocus(): void { __native__sfml_window_request_focus(this.handle); }

    public function setMousePosition(int x, int y): void {
        __native__sfml_window_set_mouse_position(this.handle, x, y);
    }
    // [x, y] in window-local pixels.
    public function mousePosition(): int[] {
        return __native__sfml_mouse_position_window(this.handle);
    }

    public function setMouseCursor(Cursor cursor): void {
        __native__sfml_window_set_mouse_cursor(this.handle, cursor.handle);
    }
}

class Sfml {
    public static function createWindow(string title, int width, int height): RenderWindow {
        return new RenderWindow(__native__sfml_create_window(title, width, height));
    }

    // Event-tag constants. Compare against the int returned by
    // RenderWindow.pollEvent().
    public static function closedEventId():              int { return __native__sfml_evt_closed_id(); }
    public static function resizedEventId():             int { return __native__sfml_evt_resized_id(); }
    public static function keyPressedEventId():          int { return __native__sfml_evt_key_pressed_id(); }
    public static function keyReleasedEventId():         int { return __native__sfml_evt_key_released_id(); }
    public static function mouseMovedEventId():          int { return __native__sfml_evt_mouse_moved_id(); }
    public static function mouseButtonPressedEventId():  int { return __native__sfml_evt_mouse_button_pressed_id(); }
    public static function mouseButtonReleasedEventId(): int { return __native__sfml_evt_mouse_button_released_id(); }
    public static function mouseWheelScrolledEventId():  int { return __native__sfml_evt_mouse_wheel_scrolled_id(); }
    public static function textEnteredEventId():         int { return __native__sfml_evt_text_entered_id(); }
    public static function focusGainedEventId():         int { return __native__sfml_evt_focus_gained_id(); }
    public static function focusLostEventId():           int { return __native__sfml_evt_focus_lost_id(); }
}

// Read accessors for the most-recently-polled event. Each returns 0
// when the last event isn't of the matching type — gate on Sfml's
// event-id constants first.
class Event {
    public static function mouseX():        int   { return __native__sfml_evt_mouse_x(); }
    public static function mouseY():        int   { return __native__sfml_evt_mouse_y(); }
    public static function mouseButton():   int   { return __native__sfml_evt_mouse_button(); }
    public static function wheelDelta():    float { return __native__sfml_evt_wheel_delta(); }
    public static function key():           int   { return __native__sfml_evt_key(); }
    public static function resizeWidth():   int   { return __native__sfml_evt_resize_width(); }
    public static function resizeHeight():  int   { return __native__sfml_evt_resize_height(); }
    public static function textUnicode():   int   { return __native__sfml_evt_text_unicode(); }
}

// Realtime input — read the current state without going through events.
class Keyboard {
    public static function isKeyPressed(int key): bool {
        return __native__sfml_key_is_down(key);
    }
}

class Mouse {
    public static function isButtonPressed(int button): bool {
        return __native__sfml_mouse_button_is_down(button);
    }
}

// Common sf::Keyboard::Key values from SFML 3's scoped enum. Add more
// as needed (the SFML enum has ~100 entries).
class Key {
    public static function a(): int { return 0; }
    public static function b(): int { return 1; }
    public static function c(): int { return 2; }
    public static function d(): int { return 3; }
    public static function e(): int { return 4; }
    public static function f(): int { return 5; }
    public static function g(): int { return 6; }
    public static function h(): int { return 7; }
    public static function i(): int { return 8; }
    public static function j(): int { return 9; }
    public static function k(): int { return 10; }
    public static function l(): int { return 11; }
    public static function m(): int { return 12; }
    public static function n(): int { return 13; }
    public static function o(): int { return 14; }
    public static function p(): int { return 15; }
    public static function q(): int { return 16; }
    public static function r(): int { return 17; }
    public static function s(): int { return 18; }
    public static function t(): int { return 19; }
    public static function u(): int { return 20; }
    public static function v(): int { return 21; }
    public static function w(): int { return 22; }
    public static function x(): int { return 23; }
    public static function y(): int { return 24; }
    public static function z(): int { return 25; }

    public static function escape():    int { return 36; }
    public static function leftCtrl():  int { return 37; }
    public static function leftShift(): int { return 38; }
    public static function leftAlt():   int { return 39; }
    public static function space():     int { return 57; }
    public static function enter():     int { return 58; }
    public static function backspace(): int { return 59; }
    public static function tab():       int { return 60; }

    public static function left():  int { return 71; }
    public static function right(): int { return 72; }
    public static function up():    int { return 73; }
    public static function down():  int { return 74; }

    public static function f1():  int { return 85; }
    public static function f2():  int { return 86; }
    public static function f3():  int { return 87; }
    public static function f4():  int { return 88; }
    public static function f5():  int { return 89; }
    public static function f6():  int { return 90; }
    public static function f7():  int { return 91; }
    public static function f8():  int { return 92; }
    public static function f9():  int { return 93; }
    public static function f10(): int { return 94; }
    public static function f11(): int { return 95; }
    public static function f12(): int { return 96; }
}

// sf::Mouse::Button.
class MouseButton {
    public static function left():   int { return 0; }
    public static function right():  int { return 1; }
    public static function middle(): int { return 2; }
    public static function xButton1(): int { return 3; }
    public static function xButton2(): int { return 4; }
}

// sf::Cursor::Type values. Use with Cursors::createFromSystem.
class CursorType {
    public static function arrow():           int { return 0; }
    public static function arrowWait():       int { return 1; }
    public static function wait():            int { return 2; }
    public static function text():            int { return 3; }
    public static function hand():            int { return 4; }
    public static function sizeHorizontal():  int { return 5; }
    public static function sizeVertical():    int { return 6; }
    public static function sizeTopLeftBottomRight():     int { return 7; }
    public static function sizeBottomLeftTopRight():     int { return 8; }
    public static function sizeLeft():        int { return 9; }
    public static function sizeRight():       int { return 10; }
    public static function sizeTop():         int { return 11; }
    public static function sizeBottom():      int { return 12; }
    public static function sizeTopLeft():     int { return 13; }
    public static function sizeBottomRight(): int { return 14; }
    public static function sizeBottomLeft():  int { return 15; }
    public static function sizeTopRight():    int { return 16; }
    public static function sizeAll():         int { return 17; }
    public static function cross():           int { return 18; }
    public static function help():            int { return 19; }
    public static function notAllowed():      int { return 20; }
}

class Cursor {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_cursor_destroy(this.handle); }
}

class Cursors {
    // type: one of CursorType::xxx().
    public static function createFromSystem(int type): Cursor {
        return new Cursor(__native__sfml_cursor_create_from_system(type));
    }
}

// sf::Joystick::Axis values (sf::Joystick::Axis enum order).
class JoystickAxis {
    public static function x():    int { return 0; }
    public static function y():    int { return 1; }
    public static function z():    int { return 2; }
    public static function r():    int { return 3; }
    public static function u():    int { return 4; }
    public static function v():    int { return 5; }
    public static function povX(): int { return 6; }
    public static function povY(): int { return 7; }
}

// Joystick state — SFML polls the OS automatically each frame in
// pollEvent(); call update() if you're sampling outside that loop.
class Joystick {
    public static function update(): void { __native__sfml_joystick_update(); }
    public static function isConnected(int id): bool {
        return __native__sfml_joystick_is_connected(id);
    }
    public static function buttonCount(int id): int {
        return __native__sfml_joystick_button_count(id);
    }
    public static function hasAxis(int id, int axis): bool {
        return __native__sfml_joystick_axis_is_available(id, axis);
    }
    public static function isButtonPressed(int id, int button): bool {
        return __native__sfml_joystick_is_button_pressed(id, button);
    }
    // Returns [-100.0, 100.0].
    public static function axisPosition(int id, int axis): float {
        return __native__sfml_joystick_axis_position(id, axis);
    }
}
