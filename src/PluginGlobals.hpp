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
    class ConvexShape;
    class VertexArray;
    class View;
    class RenderTexture;
    class Shader;
    class Image;
    class Clock;
    class Cursor;
    class SoundBuffer;
    class Sound;
    class Music;
}

struct ImFont;

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
    extern HandleRegistry<sf::ConvexShape>    g_convexShapes;
    extern HandleRegistry<sf::VertexArray>    g_vertexArrays;
    extern HandleRegistry<sf::View>           g_views;
    extern HandleRegistry<sf::RenderTexture>  g_renderTextures;
    extern HandleRegistry<sf::Shader>         g_shaders;
    extern HandleRegistry<sf::Image>          g_images;
    extern HandleRegistry<sf::Clock>          g_clocks;
    extern HandleRegistry<sf::Cursor>         g_cursors;
    extern HandleRegistry<sf::SoundBuffer>    g_soundBuffers;
    extern HandleRegistry<sf::Sound>          g_sounds;
    extern HandleRegistry<sf::Music>          g_music;
    extern HandleRegistry<ImFont>             g_imguiFonts;

    void registerSystemNatives(MTypeContext* ctx);
    void registerWindowNatives(MTypeContext* ctx);
    void registerGraphicsNatives(MTypeContext* ctx);
    void registerAudioNatives(MTypeContext* ctx);
    void registerImGuiNatives(MTypeContext* ctx);

    /* Hand WindowBindings' last-polled sf::Event to ImGui::SFML.
     * Implemented in WindowBindings.cpp where g_lastEvent is in scope. */
    void imguiProcessLastEvent(sf::RenderWindow& win);
}
