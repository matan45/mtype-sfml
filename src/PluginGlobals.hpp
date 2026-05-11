#pragma once
/*
 * Plugin-global state shared across the binding TUs:
 *  - host vtable pointer (set in mtype_plugin_register)
 *  - one HandleRegistry per native SFML type we expose
 *
 * Definitions live in PluginEntry.cpp. Forward-declares only — keeps the
 * SFML headers out of the host-side ABI surface.
 */

#include "PluginHostApi.h"
#include "HandleRegistry.hpp"

namespace sf
{
    class RenderWindow;
    class Texture;
    class Sprite;
    class Font;
    class Text;
    class RectangleShape;
    class CircleShape;
}

namespace mtypesfml
{
    extern const MTypePluginHost* g_host;

    extern HandleRegistry<sf::RenderWindow>   g_windows;
    extern HandleRegistry<sf::Texture>        g_textures;
    extern HandleRegistry<sf::Sprite>         g_sprites;
    extern HandleRegistry<sf::Font>           g_fonts;
    extern HandleRegistry<sf::Text>           g_texts;
    extern HandleRegistry<sf::RectangleShape> g_rectShapes;
    extern HandleRegistry<sf::CircleShape>    g_circleShapes;

    void registerWindowNatives(MTypeContext* ctx);
    void registerGraphicsNatives(MTypeContext* ctx);
}
