// SFML 3 demo: window + events + shapes + realtime input.
//
// No texture/font loaded by default (no asset shipped) — uses
// RectangleShape + CircleShape and the realtime Keyboard API.
// Esc quits. WASD moves the player rectangle.

import * from "../lib/Sfml.mt";
import * from "../lib/Graphics.mt";
import * from "../lib/System.mt";

__plugin_load("mt/mtype_sfml.dll");

RenderWindow window = Sfml::createWindow("SFML 3 from mType", 900, 600);

// Player rectangle.
RectangleShape player = Rectangles::create(48.0, 48.0);
player.setFillColor(220, 80, 60, 255);
player.setOutlineColor(255, 255, 255, 255);
player.setOutlineThickness(2.0);
float px = 400.0;
float py = 300.0;

// Decorative orbiting circle.
CircleShape orb = Circles::create(12.0);
orb.setFillColor(120, 180, 255, 255);

// Pre-cache event-tag constants + scancodes once.
int evClosed     = Sfml::closedEventId();
int evKeyPressed = Sfml::keyPressedEventId();
int keyEsc = Key::escape();
int keyW   = Key::w();
int keyA   = Key::a();
int keyS   = Key::s();
int keyD   = Key::d();

float speed = 200.0;     // px/sec
float t = 0.0;            // accumulator for orb orbit
Clock frame = Clocks::create();

while (window.isOpen()) {
    int ev = window.pollEvent();
    while (ev != 0) {
        if (ev == evClosed) {
            window.close();
        } else if (ev == evKeyPressed) {
            if (Event::key() == keyEsc) {
                window.close();
            }
        }
        ev = window.pollEvent();
    }

    float dt = frame.restartSeconds();
    if (Keyboard::isKeyPressed(keyW)) { py = py - speed * dt; }
    if (Keyboard::isKeyPressed(keyS)) { py = py + speed * dt; }
    if (Keyboard::isKeyPressed(keyA)) { px = px - speed * dt; }
    if (Keyboard::isKeyPressed(keyD)) { px = px + speed * dt; }

    // Orb orbits the player.
    t = t + dt;
    float ox = px + 80.0 + 60.0 * t;
    float oy = py + 60.0;
    orb.setPosition(ox, oy);
    player.setPosition(px, py);

    window.clear(20, 22, 32, 255);
    Draw::rect(window, player);
    Draw::circle(window, orb);
    window.display();
}

frame.destroy();
orb.destroy();
player.destroy();
window.destroy();
__plugin_unload("mt/mtype_sfml.dll");
