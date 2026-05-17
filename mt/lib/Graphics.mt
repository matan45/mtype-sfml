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

    // Re-upload pixels from a CPU-side Image. Image and Texture must
    // already have matching dimensions.
    public function updateFromImage(Image img): void {
        __native__sfml_texture_update_from_image(this.handle, img.handle);
    }
    // Snapshot the GPU texture back into a CPU-side Image. Caller owns
    // the returned Image and must .destroy() it.
    public function copyToImage(): Image {
        return new Image(__native__sfml_texture_copy_to_image(this.handle));
    }
}

class Textures {
    // Load PNG / JPG / BMP / TGA into a GPU-resident texture.
    public static function load(string path): Texture {
        return new Texture(__native__sfml_texture_load_from_file(path));
    }
    // Upload an Image into a fresh Texture.
    public static function fromImage(Image img): Texture {
        return new Texture(__native__sfml_texture_load_from_image(img.handle));
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
    // [x, y, w, h] axis-aligned bounding box in world space.
    public function globalBounds(): float[] {
        return __native__sfml_sprite_global_bounds(this.handle);
    }
    public function localBounds(): float[] {
        return __native__sfml_sprite_local_bounds(this.handle);
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

    // ---- transform parity (Sprite-style transforms on RectangleShape) ----
    public function setScale(float sx, float sy): void {
        __native__sfml_rect_set_scale(this.handle, sx, sy);
    }
    public function setRotation(float degrees): void {
        __native__sfml_rect_set_rotation(this.handle, degrees);
    }
    public function setOrigin(float ox, float oy): void {
        __native__sfml_rect_set_origin(this.handle, ox, oy);
    }
    public function move(float dx, float dy): void {
        __native__sfml_rect_move(this.handle, dx, dy);
    }
    public function rotate(float degrees): void {
        __native__sfml_rect_rotate(this.handle, degrees);
    }
    public function scale(float sx, float sy): void {
        __native__sfml_rect_scale(this.handle, sx, sy);
    }
    public function globalBounds(): float[] { return __native__sfml_rect_global_bounds(this.handle); }
    public function localBounds():  float[] { return __native__sfml_rect_local_bounds(this.handle); }
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

    // ---- transform parity ----
    public function setScale(float sx, float sy): void {
        __native__sfml_circle_set_scale(this.handle, sx, sy);
    }
    public function setRotation(float degrees): void {
        __native__sfml_circle_set_rotation(this.handle, degrees);
    }
    public function setOrigin(float ox, float oy): void {
        __native__sfml_circle_set_origin(this.handle, ox, oy);
    }
    public function move(float dx, float dy): void {
        __native__sfml_circle_move(this.handle, dx, dy);
    }
    public function rotate(float degrees): void {
        __native__sfml_circle_rotate(this.handle, degrees);
    }
    public function scale(float sx, float sy): void {
        __native__sfml_circle_scale(this.handle, sx, sy);
    }
    public function globalBounds(): float[] { return __native__sfml_circle_global_bounds(this.handle); }
    public function localBounds():  float[] { return __native__sfml_circle_local_bounds(this.handle); }
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

    // ---- advanced styling ----
    public function setOutlineColor(int r, int g, int b, int a): void {
        __native__sfml_text_set_outline_color(this.handle, r, g, b, a);
    }
    public function setOutlineThickness(float t): void {
        __native__sfml_text_set_outline_thickness(this.handle, t);
    }
    // style is a bitmask of TextStyle constants (Regular=0, Bold=1, Italic=2,
    // Underlined=4, StrikeThrough=8 — OR them together).
    public function setStyle(int style): void {
        __native__sfml_text_set_style(this.handle, style);
    }
    // factor: 1.0 = default. Bigger values widen letter / line gaps.
    public function setLetterSpacing(float factor): void {
        __native__sfml_text_set_letter_spacing(this.handle, factor);
    }
    public function setLineSpacing(float factor): void {
        __native__sfml_text_set_line_spacing(this.handle, factor);
    }

    // ---- transform parity ----
    public function setScale(float sx, float sy): void {
        __native__sfml_text_set_scale(this.handle, sx, sy);
    }
    public function setRotation(float degrees): void {
        __native__sfml_text_set_rotation(this.handle, degrees);
    }
    public function setOrigin(float ox, float oy): void {
        __native__sfml_text_set_origin(this.handle, ox, oy);
    }
    public function move(float dx, float dy): void {
        __native__sfml_text_move(this.handle, dx, dy);
    }
    public function rotate(float degrees): void {
        __native__sfml_text_rotate(this.handle, degrees);
    }
    public function scale(float sx, float sy): void {
        __native__sfml_text_scale(this.handle, sx, sy);
    }
    public function globalBounds(): float[] { return __native__sfml_text_global_bounds(this.handle); }
    public function localBounds():  float[] { return __native__sfml_text_local_bounds(this.handle); }
}

// sf::Text::Style bitmask values.
class TextStyle {
    public static function regular():       int { return 0; }
    public static function bold():          int { return 1; }
    public static function italic():        int { return 2; }
    public static function underlined():    int { return 4; }
    public static function strikeThrough(): int { return 8; }
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
    public static function convex(RenderWindow w, ConvexShape c): void {
        __native__sfml_window_draw_convex(w.handle, c.handle);
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
	
	// Resync SFML's internal cache of GL state. Call once after any draw
    // through a custom Shader so subsequent default draws (ImGui, plain
    // shapes) don't inherit stale shader / texture / blend bindings.
    public static function resetGLStates(RenderWindow w): void {
      __native__sfml_window_reset_gl_states(w.handle);
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
    public static function convex(RenderTexture rt, ConvexShape c): void {
        __native__sfml_render_texture_draw_convex(rt.handle, c.handle);
    }
}

// ----------------------------------------------------------------------
// Phase 4 — Shader (GLSL programs).
//
// Load a fragment / vertex GLSL shader, set uniforms, then draw with
// the shader bound. Use Shader::CurrentTexture sentinel for the
// drawable's own texture (the common case for post-processing).
// ----------------------------------------------------------------------

// sf::Shader::Type values (SFML 3 enum class order: Vertex, Geometry, Fragment).
class ShaderType {
    public static final int VERTEX = 0;
    public static final int FRAGMENT = 1;
    public static final int GEOMETRY = 2;
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

    // ---- Phase 4: advanced uniforms ----
    public function loadVertFragFromMemory(string vertSrc, string fragSrc): bool {
        return __native__sfml_shader_load_vert_frag_from_memory(this.handle, vertSrc, fragSrc);
    }
    public function setBool(string name, bool value): void {
        __native__sfml_shader_set_uniform_bool(this.handle, name, value);
    }
    public function setIvec2(string name, int x, int y): void {
        __native__sfml_shader_set_uniform_ivec2(this.handle, name, x, y);
    }
    public function setIvec3(string name, int x, int y, int z): void {
        __native__sfml_shader_set_uniform_ivec3(this.handle, name, x, y, z);
    }
    public function setIvec4(string name, int x, int y, int z, int w): void {
        __native__sfml_shader_set_uniform_ivec4(this.handle, name, x, y, z, w);
    }
    // Normalizes 0-255 sf::Color to a vec4 in [0, 1].
    public function setColor(string name, int r, int g, int b, int a): void {
        __native__sfml_shader_set_uniform_color(this.handle, name, r, g, b, a);
    }
    // Column-major 9 floats.
    public function setMat3(string name, float[] m): void {
        __native__sfml_shader_set_uniform_mat3(this.handle, name, m);
    }
    // Column-major 16 floats.
    public function setMat4(string name, float[] m): void {
        __native__sfml_shader_set_uniform_mat4(this.handle, name, m);
    }
    public function setFloatArray(string name, float[] values): void {
        __native__sfml_shader_set_uniform_float_array(this.handle, name, values);
    }
    // Flat-packed: every 4 consecutive floats form one vec4.
    public function setVec4Array(string name, float[] values): void {
        __native__sfml_shader_set_uniform_vec4_array(this.handle, name, values);
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

// ----------------------------------------------------------------------
// ConvexShape — arbitrary convex polygon (sibling of RectangleShape /
// CircleShape).
// ----------------------------------------------------------------------

class ConvexShape {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_convex_destroy(this.handle); }

    public function setPointCount(int n): void {
        __native__sfml_convex_set_point_count(this.handle, n);
    }
    public function setPoint(int index, float x, float y): void {
        __native__sfml_convex_set_point(this.handle, index, x, y);
    }

    public function setPosition(float x, float y): void {
        __native__sfml_convex_set_position(this.handle, x, y);
    }
    public function setFillColor(int r, int g, int b, int a): void {
        __native__sfml_convex_set_fill_color(this.handle, r, g, b, a);
    }
    public function setOutlineColor(int r, int g, int b, int a): void {
        __native__sfml_convex_set_outline_color(this.handle, r, g, b, a);
    }
    public function setOutlineThickness(float t): void {
        __native__sfml_convex_set_outline_thickness(this.handle, t);
    }

    // ---- transform parity ----
    public function setScale(float sx, float sy): void {
        __native__sfml_convex_set_scale(this.handle, sx, sy);
    }
    public function setRotation(float degrees): void {
        __native__sfml_convex_set_rotation(this.handle, degrees);
    }
    public function setOrigin(float ox, float oy): void {
        __native__sfml_convex_set_origin(this.handle, ox, oy);
    }
    public function move(float dx, float dy): void {
        __native__sfml_convex_move(this.handle, dx, dy);
    }
    public function rotate(float degrees): void {
        __native__sfml_convex_rotate(this.handle, degrees);
    }
    public function scale(float sx, float sy): void {
        __native__sfml_convex_scale(this.handle, sx, sy);
    }
    public function globalBounds(): float[] { return __native__sfml_convex_global_bounds(this.handle); }
    public function localBounds():  float[] { return __native__sfml_convex_local_bounds(this.handle); }
}

class ConvexShapes {
    public static function create(int pointCount): ConvexShape {
        return new ConvexShape(__native__sfml_convex_create(pointCount));
    }
}

// ----------------------------------------------------------------------
// Image — CPU-side pixel buffer. Counterpart to Texture (GPU). Use for
// loading/saving PNG/JPG, procedural pixel art, screenshots.
// ----------------------------------------------------------------------

class Image {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_image_destroy(this.handle); }

    // [width, height] in pixels.
    public function size(): int[] { return __native__sfml_image_size(this.handle); }

    public function saveToFile(string path): bool {
        return __native__sfml_image_save_to_file(this.handle, path);
    }

    // Returns [r, g, b, a] each in 0..255. Out-of-range coords return all zeros.
    public function getPixel(int x, int y): int[] {
        return __native__sfml_image_get_pixel(this.handle, x, y);
    }
    public function setPixel(int x, int y, int r, int g, int b, int a): void {
        __native__sfml_image_set_pixel(this.handle, x, y, r, g, b, a);
    }

    // Replaces every pixel whose (r,g,b,a) matches `match` with an alpha
    // of `alpha` (other channels left unchanged). Common idiom: pass
    // (255,0,255,255) and alpha=0 to mask out magenta keys.
    public function createMaskFromColor(int r, int g, int b, int a, int alpha): void {
        __native__sfml_image_create_mask_from_color(this.handle, r, g, b, a, alpha);
    }
}

class Images {
    public static function create(int width, int height,
                                    int r, int g, int b, int a): Image {
        return new Image(__native__sfml_image_create(width, height, r, g, b, a));
    }
    public static function load(string path): Image {
        return new Image(__native__sfml_image_load_from_file(path));
    }
}
