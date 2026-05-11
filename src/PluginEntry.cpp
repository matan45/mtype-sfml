#include "PluginGlobals.hpp"

#include <SFML/Graphics.hpp>

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
    HandleRegistry<sf::VertexArray>    g_vertexArrays;
    HandleRegistry<sf::View>           g_views;
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

    mtypesfml::registerWindowNatives(registrationCtx);
    mtypesfml::registerGraphicsNatives(registrationCtx);
    return 0;
}
