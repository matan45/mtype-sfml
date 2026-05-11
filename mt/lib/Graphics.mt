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

// ----------------------------------------------------------------------
// Phase 2 — VertexArray (procedural geometry) + View (camera).
// ----------------------------------------------------------------------

// sf::PrimitiveType values from SFML 3's enum class.
class Primitive {
    public static function points():        int { return 0; }
    public static function lines():         int { return 1; }
    public static function lineStrip():     int { return 2; }
    public static function triangles():     int { return 3; }
    public static function triangleStrip(): int { return 4; }
    public static function triangleFan():   int { return 5; }
}

class VertexArray {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_vertex_array_destroy(this.handle); }

    public function size(): int { return __native__sfml_vertex_array_size(this.handle); }
    public function resize(int count): void {
        __native__sfml_vertex_array_resize(this.handle, count);
    }
    public function setPrimitiveType(int primitive): void {
        __native__sfml_vertex_array_set_primitive_type(this.handle, primitive);
    }
    // Write one vertex: (position x,y) + (color r,g,b,a) + (texCoord u,v).
    public function setVertex(int index,
                                float x, float y,
                                int r, int g, int b, int a,
                                float u, float v): void {
        __native__sfml_vertex_array_set_vertex(this.handle, index, x, y, r, g, b, a, u, v);
    }
}

class VertexArrays {
    // Create with a sf::PrimitiveType (use Primitive::xxx()) and a
    // pre-allocated vertex count. Resize later if needed.
    public static function create(int primitive, int initialSize): VertexArray {
        return new VertexArray(__native__sfml_vertex_array_create(primitive, initialSize));
    }
}

class View {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_view_destroy(this.handle); }

    public function setCenter(float x, float y): void {
        __native__sfml_view_set_center(this.handle, x, y);
    }
    public function setSize(float w, float h): void {
        __native__sfml_view_set_size(this.handle, w, h);
    }
    public function setRotation(float degrees): void {
        __native__sfml_view_set_rotation(this.handle, degrees);
    }
    public function move(float dx, float dy): void {
        __native__sfml_view_move(this.handle, dx, dy);
    }
    // Multiply view size by `factor`. < 1 zooms in, > 1 zooms out.
    public function zoom(float factor): void {
        __native__sfml_view_zoom(this.handle, factor);
    }
    public function rotate(float degrees): void {
        __native__sfml_view_rotate(this.handle, degrees);
    }
}

class Views {
    // Create a view with given center and size in world units. The view
    // maps that world rect onto the entire window.
    public static function create(float centerX, float centerY,
                                    float sizeW,   float sizeH): View {
        return new View(__native__sfml_view_create(centerX, centerY, sizeW, sizeH));
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
    public static function vertexArray(RenderWindow w, VertexArray va): void {
        __native__sfml_window_draw_vertex_array(w.handle, va.handle);
    }
    public static function vertexArrayTextured(RenderWindow w, VertexArray va,
                                                  Texture tex): void {
        __native__sfml_window_draw_vertex_array_textured(w.handle, va.handle, tex.handle);
    }
}

// Camera helpers on the window itself — pair set/reset around a draw to
// scope the transform to a subset of the frame.
class Camera {
    public static function setView(RenderWindow w, View v): void {
        __native__sfml_window_set_view(w.handle, v.handle);
    }
    public static function resetView(RenderWindow w): void {
        __native__sfml_window_reset_view(w.handle);
    }
}

// ----------------------------------------------------------------------
// Phase 3 — RenderTexture (offscreen draw target).
//
// Mirror RenderWindow's draw surface, but renders into an off-screen
// texture you can sample later (post-processing, framebuffer effects,
// minimaps). Lifetime: any borrowed Texture from getTexture() must be
// destroyed before its source RenderTexture.
// ----------------------------------------------------------------------

class RenderTexture {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_render_texture_destroy(this.handle); }

    public function size(): int[] { return __native__sfml_render_texture_size(this.handle); }
    public function resize(int w, int h): bool {
        return __native__sfml_render_texture_resize(this.handle, w, h);
    }

    public function clear(int r, int g, int b, int a): void {
        __native__sfml_render_texture_clear(this.handle, r, g, b, a);
    }
    // Must be called after drawing into the RT, before sampling its
    // texture or drawing into it again.
    public function display(): void { __native__sfml_render_texture_display(this.handle); }

    public function setView(View v): void {
        __native__sfml_render_texture_set_view(this.handle, v.handle);
    }
    public function resetView(): void { __native__sfml_render_texture_reset_view(this.handle); }

    // Borrowed Texture handle into the RT's framebuffer. The returned
    // Texture's .destroy() releases the handle without freeing the
    // underlying pixels (those belong to the RenderTexture).
    public function getTexture(): Texture {
        return new Texture(__native__sfml_render_texture_get_texture(this.handle));
    }
}

class RenderTextures {
    public static function create(int width, int height): RenderTexture {
        return new RenderTexture(__native__sfml_render_texture_create(width, height));
    }
}

// Drawing INTO a RenderTexture. Mirrors Draw::* but with an RT target.
class DrawTo {
    public static function sprite(RenderTexture rt, Sprite s): void {
        __native__sfml_render_texture_draw_sprite(rt.handle, s.handle);
    }
    public static function rect(RenderTexture rt, RectangleShape r): void {
        __native__sfml_render_texture_draw_rect(rt.handle, r.handle);
    }
    public static function circle(RenderTexture rt, CircleShape c): void {
        __native__sfml_render_texture_draw_circle(rt.handle, c.handle);
    }
    public static function text(RenderTexture rt, Text t): void {
        __native__sfml_render_texture_draw_text(rt.handle, t.handle);
    }
    public static function vertexArray(RenderTexture rt, VertexArray va): void {
        __native__sfml_render_texture_draw_vertex_array(rt.handle, va.handle);
    }
}

// ----------------------------------------------------------------------
// Phase 4 — Shader (GLSL programs).
//
// Load a fragment / vertex GLSL shader, set uniforms, then draw with
// the shader bound. Use Shader::CurrentTexture sentinel for the
// drawable's own texture (the common case for post-processing).
// ----------------------------------------------------------------------

// sf::Shader::Type values.
class ShaderType {
    public static function vertex():   int { return 0; }
    public static function fragment(): int { return 1; }
    public static function geometry(): int { return 2; }
}

class Shader {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_shader_destroy(this.handle); }

    public function loadFromFile(string path, int type): bool {
        return __native__sfml_shader_load_from_file(this.handle, path, type);
    }
    public function loadVertFragFromFile(string vertPath, string fragPath): bool {
        return __native__sfml_shader_load_vert_frag_from_file(this.handle, vertPath, fragPath);
    }
    public function loadFragFromMemory(string source): bool {
        return __native__sfml_shader_load_frag_from_memory(this.handle, source);
    }

    public function setFloat(string name, float value): void {
        __native__sfml_shader_set_uniform_float(this.handle, name, value);
    }
    public function setVec2(string name, float x, float y): void {
        __native__sfml_shader_set_uniform_vec2(this.handle, name, x, y);
    }
    public function setVec3(string name, float x, float y, float z): void {
        __native__sfml_shader_set_uniform_vec3(this.handle, name, x, y, z);
    }
    public function setVec4(string name, float x, float y, float z, float w): void {
        __native__sfml_shader_set_uniform_vec4(this.handle, name, x, y, z, w);
    }
    public function setInt(string name, int value): void {
        __native__sfml_shader_set_uniform_int(this.handle, name, value);
    }
    public function setTexture(string name, Texture tex): void {
        __native__sfml_shader_set_uniform_texture(this.handle, name, tex.handle);
    }
    // Bind the drawable's own texture for `name`. The standard pattern
    // for post-processing fragment shaders.
    public function setCurrentTexture(string name): void {
        __native__sfml_shader_set_uniform_current_texture(this.handle, name);
    }
}

class Shaders {
    public static function create(): Shader {
        return new Shader(__native__sfml_shader_create());
    }
}

// Shader-aware draws. The shader is applied to the sprite/VA's pixels.
class DrawShader {
    public static function sprite(RenderWindow w, Sprite s, Shader sh): void {
        __native__sfml_window_draw_sprite_shader(w.handle, s.handle, sh.handle);
    }
    public static function vertexArray(RenderWindow w, VertexArray va,
                                          Texture tex, Shader sh): void {
        __native__sfml_window_draw_vertex_array_shader(w.handle, va.handle,
                                                        tex.handle, sh.handle);
    }
}
