// mType wrappers around the SFML 3 graphics natives. Textures, sprites,
// shapes, fonts, and text. Each class wraps an int handle minted by the
// plugin's HandleRegistry.
//
// Lifetime: Sprite holds a non-owning reference to Texture; Text holds
// a non-owning reference to Font. Keep the underlying Texture / Font
// alive at least as long as anything built on top.

import * from "Sfml.mt";

class Texture {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_texture_destroy(this.handle); }
    public function size(): int[]    { return __native__sfml_texture_size(this.handle); }
}

class Textures {
    // Load PNG / JPG / BMP / TGA into a GPU-resident texture.
    public static function load(string path): Texture {
        return new Texture(__native__sfml_texture_load_from_file(path));
    }
}

class Sprite {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_sprite_destroy(this.handle); }

    public function setPosition(float x, float y): void {
        __native__sfml_sprite_set_position(this.handle, x, y);
    }
    public function setScale(float sx, float sy): void {
        __native__sfml_sprite_set_scale(this.handle, sx, sy);
    }
    // Rotation in degrees, clockwise.
    public function setRotation(float degrees): void {
        __native__sfml_sprite_set_rotation(this.handle, degrees);
    }
    public function setOrigin(float ox, float oy): void {
        __native__sfml_sprite_set_origin(this.handle, ox, oy);
    }
    public function setColor(int r, int g, int b, int a): void {
        __native__sfml_sprite_set_color(this.handle, r, g, b, a);
    }
}

class Sprites {
    public static function create(Texture tex): Sprite {
        return new Sprite(__native__sfml_sprite_create(tex.handle));
    }
}

class RectangleShape {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_rect_destroy(this.handle); }

    public function setPosition(float x, float y): void {
        __native__sfml_rect_set_position(this.handle, x, y);
    }
    public function setSize(float w, float h): void {
        __native__sfml_rect_set_size(this.handle, w, h);
    }
    public function setFillColor(int r, int g, int b, int a): void {
        __native__sfml_rect_set_fill_color(this.handle, r, g, b, a);
    }
    public function setOutlineColor(int r, int g, int b, int a): void {
        __native__sfml_rect_set_outline_color(this.handle, r, g, b, a);
    }
    public function setOutlineThickness(float t): void {
        __native__sfml_rect_set_outline_thickness(this.handle, t);
    }
}

class Rectangles {
    public static function create(float width, float height): RectangleShape {
        return new RectangleShape(__native__sfml_rect_create(width, height));
    }
}

class CircleShape {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_circle_destroy(this.handle); }

    public function setPosition(float x, float y): void {
        __native__sfml_circle_set_position(this.handle, x, y);
    }
    public function setRadius(float r): void {
        __native__sfml_circle_set_radius(this.handle, r);
    }
    public function setFillColor(int r, int g, int b, int a): void {
        __native__sfml_circle_set_fill_color(this.handle, r, g, b, a);
    }
    public function setOutlineColor(int r, int g, int b, int a): void {
        __native__sfml_circle_set_outline_color(this.handle, r, g, b, a);
    }
    public function setOutlineThickness(float t): void {
        __native__sfml_circle_set_outline_thickness(this.handle, t);
    }
}

class Circles {
    public static function create(float radius): CircleShape {
        return new CircleShape(__native__sfml_circle_create(radius));
    }
}

class Font {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_font_destroy(this.handle); }
}

class Fonts {
    public static function load(string path): Font {
        return new Font(__native__sfml_font_open_from_file(path));
    }
}

class Text {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_text_destroy(this.handle); }

    public function setString(string s): void {
        __native__sfml_text_set_string(this.handle, s);
    }
    public function setPosition(float x, float y): void {
        __native__sfml_text_set_position(this.handle, x, y);
    }
    public function setCharacterSize(int pixels): void {
        __native__sfml_text_set_character_size(this.handle, pixels);
    }
    public function setFillColor(int r, int g, int b, int a): void {
        __native__sfml_text_set_fill_color(this.handle, r, g, b, a);
    }
}

class Texts {
    public static function create(Font font, string s, int characterSize): Text {
        return new Text(__native__sfml_text_create(font.handle, s, characterSize));
    }
}

// Drawing primitives — passed the window's handle implicitly via this
// helper class to keep the surface flat. Pattern:
//   Draw::sprite(window, mySprite);
//   Draw::rect(window, myRect);
class Draw {
    public static function sprite(RenderWindow w, Sprite s): void {
        __native__sfml_window_draw_sprite(w.handle, s.handle);
    }
    public static function rect(RenderWindow w, RectangleShape r): void {
        __native__sfml_window_draw_rect(w.handle, r.handle);
    }
    public static function circle(RenderWindow w, CircleShape c): void {
        __native__sfml_window_draw_circle(w.handle, c.handle);
    }
    public static function text(RenderWindow w, Text t): void {
        __native__sfml_window_draw_text(w.handle, t.handle);
    }
}
