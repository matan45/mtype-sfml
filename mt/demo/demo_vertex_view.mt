// Phase 2 SFML demo: procedural geometry (VertexArray) + camera (View).
//
// A 64-segment triangle-strip ribbon with rainbow colors fills the
// world. The camera (View) pans across it: WASD moves the camera,
// Q/E zoom out/in, R rotates. Esc quits.
//
// The geometry is built once; the view transform alone moves the
// "camera" over the same vertices each frame — that's the SFML
// custom-rendering split: world geometry vs. on-screen viewport.

import * from "../lib/Sfml.mt";
import * from "../lib/Graphics.mt";

__plugin_load("mt/mtype_sfml.dll");

RenderWindow window = Sfml::createWindow("VertexArray + View", 900, 600);

// World is 4000 x 600. Camera shows 900 x 600 of it at a time.
float worldW = 4000.0;
float worldH = 600.0;

// Build a triangle strip: pairs of vertices (top, bottom) across the
// world, with hue cycling along x.
int segments = 64;
int vertexCount = (segments + 1) * 2;
VertexArray ribbon = VertexArrays::create(Primitive::triangleStrip(), vertexCount);

int i = 0;
while (i <= segments) {
    float t = ((float)i) / ((float)segments);
    float x = t * worldW;
    // Hue ramps across the ribbon. Convert hue -> RGB in 3 bands.
    float h = t * 6.0;
    int r = 0; int g = 0; int b = 0;
    if (h < 1.0) {
        r = 255;
        g = (int)(h * 255.0);
        b = 0;
    } else if (h < 2.0) {
        r = (int)((2.0 - h) * 255.0);
        g = 255;
        b = 0;
    } else if (h < 3.0) {
        r = 0;
        g = 255;
        b = (int)((h - 2.0) * 255.0);
    } else if (h < 4.0) {
        r = 0;
        g = (int)((4.0 - h) * 255.0);
        b = 255;
    } else if (h < 5.0) {
        r = (int)((h - 4.0) * 255.0);
        g = 0;
        b = 255;
    } else {
        r = 255;
        g = 0;
        b = (int)((6.0 - h) * 255.0);
    }
    // Top vertex (y near 100), bottom vertex (y near 500) — strip pairs them.
    ribbon.setVertex(i * 2,     x, 100.0, r, g, b, 255, 0.0, 0.0);
    ribbon.setVertex(i * 2 + 1, x, 500.0, r, g, b, 255, 0.0, 0.0);
    i = i + 1;
}

// Camera: starts centered on the world's left, showing a 900x600 chunk.
float camX = 450.0;
float camY = 300.0;
float camZoom = 1.0;     // 1.0 = exactly 900x600 visible; >1 zoomed out
float camRot = 0.0;
View cam = Views::create(camX, camY, 900.0, 600.0);

int evClosed     = Sfml::closedEventId();
int evKeyPressed = Sfml::keyPressedEventId();
int kEsc = Key::escape();
int kW = Key::w();
int kA = Key::a();
int kS = Key::s();
int kD = Key::d();
int kQ = Key::q();
int kE = Key::e();
int kR = Key::r();

float panSpeed  = 400.0;   // world units/sec
float zoomRate  = 0.6;     // factor/sec
float rotRate   = 60.0;    // deg/sec
float dt = 1.0 / 60.0;

while (window.isOpen()) {
    int ev = window.pollEvent();
    while (ev != 0) {
        if (ev == evClosed) {
            window.close();
        } else if (ev == evKeyPressed) {
            if (Event::key() == kEsc) { window.close(); }
        }
        ev = window.pollEvent();
    }

    if (Keyboard::isKeyPressed(kW)) { camY = camY - panSpeed * dt; }
    if (Keyboard::isKeyPressed(kS)) { camY = camY + panSpeed * dt; }
    if (Keyboard::isKeyPressed(kA)) { camX = camX - panSpeed * dt; }
    if (Keyboard::isKeyPressed(kD)) { camX = camX + panSpeed * dt; }
    if (Keyboard::isKeyPressed(kQ)) { camZoom = camZoom + zoomRate * dt; }
    if (Keyboard::isKeyPressed(kE)) { camZoom = camZoom - zoomRate * dt; }
    if (camZoom < 0.2) { camZoom = 0.2; }
    if (camZoom > 4.0) { camZoom = 4.0; }
    if (Keyboard::isKeyPressed(kR)) { camRot = camRot + rotRate * dt; }

    cam.setCenter(camX, camY);
    cam.setSize(900.0 * camZoom, 600.0 * camZoom);
    cam.setRotation(camRot);

    window.clear(15, 17, 22, 255);

    // Switch to the camera for the world-space draw.
    Camera::setView(window, cam);
    Draw::vertexArray(window, ribbon);

    // Switch back to the default view for any HUD-style drawing (none
    // in this demo, but this is the standard pattern).
    Camera::resetView(window);

    window.display();
}

cam.destroy();
ribbon.destroy();
window.destroy();
__plugin_unload("mt/mtype_sfml.dll");
