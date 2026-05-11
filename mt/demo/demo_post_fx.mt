// Phase 3 + 4 demo: offscreen rendering (RenderTexture) + GLSL shader
// post-processing.
//
// Pipeline per frame:
//   1. Build a rainbow triangle-strip ribbon (VertexArray).
//   2. Render it into a RenderTexture (offscreen).
//   3. Wrap the RT's framebuffer texture in a Sprite.
//   4. Draw that sprite to the window through a fragment shader that
//      adds wavy distortion + scanlines, driven by a time uniform.
//
// Controls:
//   Esc — quit
//   Space — toggle shader on/off
//   Up / Down — increase/decrease wave amplitude

import * from "../lib/Sfml.mt";
import * from "../lib/Graphics.mt";

__plugin_load("mt/mtype_sfml.dll");

int W = 900;
int H = 600;
RenderWindow window = Sfml::createWindow("Phase 3+4: RenderTexture + Shader", W, H);

// ---- offscreen target ----
RenderTexture rt = RenderTextures::create(W, H);

// ---- geometry (same ribbon idea as the vertex-view demo) ----
int segments = 64;
int vertexCount = (segments + 1) * 2;
VertexArray ribbon = VertexArrays::create(Primitive::triangleStrip(), vertexCount);

int i = 0;
while (i <= segments) {
    float t = ((float)i) / ((float)segments);
    float x = t * ((float)W);
    float h = t * 6.0;
    int r = 0; int g = 0; int b = 0;
    if (h < 1.0)       { r = 255;                          g = (int)(h * 255.0);          b = 0; }
    else if (h < 2.0)  { r = (int)((2.0 - h) * 255.0);     g = 255;                       b = 0; }
    else if (h < 3.0)  { r = 0;                            g = 255;                       b = (int)((h - 2.0) * 255.0); }
    else if (h < 4.0)  { r = 0;                            g = (int)((4.0 - h) * 255.0);  b = 255; }
    else if (h < 5.0)  { r = (int)((h - 4.0) * 255.0);     g = 0;                         b = 255; }
    else               { r = 255;                          g = 0;                         b = (int)((6.0 - h) * 255.0); }
    ribbon.setVertex(i * 2,     x, 100.0, r, g, b, 255, 0.0, 0.0);
    ribbon.setVertex(i * 2 + 1, x, 500.0, r, g, b, 255, 0.0, 0.0);
    i = i + 1;
}

// ---- fragment shader (loaded from a mType string, no file shipped) ----
string fragSrc =
    "uniform sampler2D texture;\n"
    + "uniform float time;\n"
    + "uniform float amplitude;\n"
    + "uniform vec2 resolution;\n"
    + "void main() {\n"
    + "    vec2 uv = gl_TexCoord[0].xy;\n"
    + "    uv.x += sin(uv.y * 20.0 + time * 2.0) * amplitude;\n"
    + "    vec4 c = texture2D(texture, uv);\n"
    + "    float scan = 0.85 + 0.15 * sin(uv.y * resolution.y * 1.5);\n"
    + "    gl_FragColor = vec4(c.rgb * scan, c.a) * gl_Color;\n"
    + "}\n";

Shader fx = Shaders::create();
bool ok = fx.loadFragFromMemory(fragSrc);
if (!ok) {
    print("Shader failed to compile — running without effect.");
}
fx.setCurrentTexture("texture");
fx.setVec2("resolution", (float)W, (float)H);

// Sprite that wraps the RT's framebuffer texture.
Texture rtTex = rt.getTexture();
Sprite rtSprite = Sprites::create(rtTex);

// ---- state ----
bool shaderOn = true;
float amplitude = 0.01;
float t = 0.0;
float dt = 1.0 / 60.0;

int evClosed     = Sfml::closedEventId();
int evKeyPressed = Sfml::keyPressedEventId();
int kEsc   = Key::escape();
int kSpace = Key::space();
int kUp    = Key::up();
int kDown  = Key::down();

while (window.isOpen()) {
    int ev = window.pollEvent();
    while (ev != 0) {
        if (ev == evClosed) {
            window.close();
        } else if (ev == evKeyPressed) {
            int k = Event::key();
            if (k == kEsc)   { window.close(); }
            if (k == kSpace) { shaderOn = !shaderOn; }
            if (k == kUp)    { amplitude = amplitude + 0.005; }
            if (k == kDown)  { amplitude = amplitude - 0.005; if (amplitude < 0.0) { amplitude = 0.0; } }
        }
        ev = window.pollEvent();
    }

    t = t + dt;
    fx.setFloat("time", t);
    fx.setFloat("amplitude", amplitude);

    // 1. Draw ribbon into the offscreen RT.
    rt.clear(15, 17, 22, 255);
    DrawTo::vertexArray(rt, ribbon);
    rt.display();

    // 2. Composite RT to window, optionally through the shader.
    window.clear(0, 0, 0, 255);
    if (shaderOn && ok) {
        DrawShader::sprite(window, rtSprite, fx);
    } else {
        Draw::sprite(window, rtSprite);
    }
    window.display();
}

rtSprite.destroy();
rtTex.destroy();   // borrowed handle — no-op on the underlying pixels
fx.destroy();
ribbon.destroy();
rt.destroy();
window.destroy();
__plugin_unload("mt/mtype_sfml.dll");
