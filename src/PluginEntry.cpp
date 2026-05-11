#include "PluginGlobals.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>

namespace mtypesfml
{
    const MTypePluginHost* g_host = nullptr;

    HandleRegistry<sf::RenderWindow>   g_windows;
    HandleRegistry<sf::Texture>        g_textures;
    HandleRegistry<sf::Sprite>         g_sprites;
    HandleRegistry<sf::Font>           g_fonts;
    HandleRegistry<sf::Text>           g_texts;
    HandleRegistry<sf::RectangleShape> g_rectShapes;
    HandleRegistry<sf::CircleShape>    g_circleShapes;
    HandleRegistry<sf::ConvexShape>    g_convexShapes;
    HandleRegistry<sf::VertexArray>    g_vertexArrays;
    HandleRegistry<sf::View>           g_views;
    HandleRegistry<sf::RenderTexture>  g_renderTextures;
    HandleRegistry<sf::Shader>         g_shaders;
    HandleRegistry<sf::Image>          g_images;
    HandleRegistry<sf::Clock>          g_clocks;
    HandleRegistry<sf::Cursor>         g_cursors;
    HandleRegistry<sf::SoundBuffer>    g_soundBuffers;
    HandleRegistry<sf::Sound>          g_sounds;
    HandleRegistry<sf::Music>          g_music;
    HandleRegistry<ImFont>             g_imguiFonts;
}

extern "C" MTYPE_PLUGIN_EXPORT
int mtype_plugin_register(uint32_t hostAbiVersion,
                          const MTypePluginHost* host,
                          MTypeContext* registrationCtx)
{
    if (hostAbiVersion != MTYPE_PLUGIN_ABI_VERSION) {
        return 1;
    }
    mtypesfml::g_host = host;

    mtypesfml::registerSystemNatives(registrationCtx);
    mtypesfml::registerWindowNatives(registrationCtx);
    mtypesfml::registerGraphicsNatives(registrationCtx);
    mtypesfml::registerAudioNatives(registrationCtx);
    mtypesfml::registerImGuiNatives(registrationCtx);
    return 0;
}
