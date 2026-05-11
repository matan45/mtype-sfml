// SFML + ImGui together. The rainbow ribbon (SFML VertexArray) lives in
// the same window as an ImGui controls panel that drives its parameters
// live. Both libraries draw to the SAME RenderWindow — ImGui::SFML wires
// ImGui's draw lists into SFML's OpenGL context.

import * from "../lib/Sfml.mt";
import * from "../lib/Graphics.mt";
import * from "../lib/ImGui.mt";

__plugin_load("mt/mtype_sfml.dll");

int W = 1100;
int H = 700;
RenderWindow window = Sfml::createWindow("SFML + ImGui", W, H);

ImGui::init(window);

// Ribbon — same triangle-strip idea as demo_vertex_view.
int segments = 64;
int vertexCount = (segments + 1) * 2;
VertexArray ribbon = VertexArrays::create(Primitive::triangleStrip(), vertexCount);

float ribbonY      = 350.0;
float ribbonHeight = 200.0;
float speed        = 1.0;
float t            = 0.0;
float dtMs         = 16.0;

// Pre-cache event ids.
int evClosed     = Sfml::closedEventId();
int evKeyPressed = Sfml::keyPressedEventId();
int kEsc = Key::escape();

while (window.isOpen()) {
    int ev = window.pollEvent();
    while (ev != 0) {
        ImGui::processSfmlEvent(window);
        if (ev == evClosed) { window.close(); }
        else if (ev == evKeyPressed && Event::key() == kEsc) { window.close(); }
        ev = window.pollEvent();
    }

    ImGui::update(window, dtMs);
    t = t + dtMs / 1000.0 * speed;

    // ImGui controls panel — adjust the ribbon parameters live.
    if (ImGui::begin("Ribbon controls")) {
        ImGui::text("World + UI in one window via imgui-sfml.");
        ImGui::separator();
        ribbonY      = ImGui::sliderFloat("center Y", ribbonY,      50.0, 650.0);
        ribbonHeight = ImGui::sliderFloat("height",   ribbonHeight, 10.0, 500.0);
        speed        = ImGui::sliderFloat("speed",    speed,        0.0,  4.0);
        ImGui::separator();
        ImGui::text("Esc or close window to quit.");
    }
    ImGui::end();

    // Rebuild ribbon for this frame (rainbow hue cycles with t).
    int i = 0;
    while (i <= segments) {
        float u = ((float)i) / ((float)segments);
        float x = u * ((float)W);
        float h = (u + t * 0.1) * 6.0;
        while (h >= 6.0) { h = h - 6.0; }
        int r = 0; int g = 0; int b = 0;
        if (h < 1.0)       { r = 255;                          g = (int)(h * 255.0);          b = 0; }
        else if (h < 2.0)  { r = (int)((2.0 - h) * 255.0);     g = 255;                       b = 0; }
        else if (h < 3.0)  { r = 0;                            g = 255;                       b = (int)((h - 2.0) * 255.0); }
        else if (h < 4.0)  { r = 0;                            g = (int)((4.0 - h) * 255.0);  b = 255; }
        else if (h < 5.0)  { r = (int)((h - 4.0) * 255.0);     g = 0;                         b = 255; }
        else               { r = 255;                          g = 0;                         b = (int)((6.0 - h) * 255.0); }
        float yTop    = ribbonY - ribbonHeight * 0.5;
        float yBottom = ribbonY + ribbonHeight * 0.5;
        ribbon.setVertex(i * 2,     x, yTop,    r, g, b, 255, 0.0, 0.0);
        ribbon.setVertex(i * 2 + 1, x, yBottom, r, g, b, 255, 0.0, 0.0);
        i = i + 1;
    }

    // World first…
    window.clear(15, 17, 22, 255);
    Draw::vertexArray(window, ribbon);
    // …then ImGui composites its widgets on top of the same window.
    ImGui::render(window);
    window.display();
}

ImGui::shutdown();
ribbon.destroy();
window.destroy();
__plugin_unload("mt/mtype_sfml.dll");
