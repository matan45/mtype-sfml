/*
 * SFML 3 graphics bindings — textures, sprites, shapes, fonts, text.
 *
 * Lifetime note: sf::Sprite and sf::Text hold non-owning references to
 * the Texture / Font they were constructed from. Scripts must keep the
 * Texture/Font alive (i.e. don't .destroy() them) until every Sprite /
 * Text built on top is also gone.
 */

#include "PluginGlobals.hpp"
#include "BindingHelpers.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/System/Angle.hpp>

#include <string>

namespace mtypesfml
{
    namespace
    {
        constexpr const char* kEx = "SfmlError";

        inline bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            return detail::requireArgs(ctx, argc, expected, name, kEx);
        }
        inline const char* getStr(const MTypeValue* v, size_t* outLen = nullptr)
        {
            return detail::getStr(v, outLen);
        }

        inline sf::Color colorFromArgs(MTypeContext*,
                                         const MTypeValue* const* args, int off)
        {
            return sf::Color(
                static_cast<std::uint8_t>(g_host->getInt(args[off])),
                static_cast<std::uint8_t>(g_host->getInt(args[off + 1])),
                static_cast<std::uint8_t>(g_host->getInt(args[off + 2])),
                static_cast<std::uint8_t>(g_host->getInt(args[off + 3])));
        }

        /* ---- Texture ---- */

        MTypeValue* nTextureLoadFromFile(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_texture_load_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            auto* tex = new sf::Texture();
            if (!tex->loadFromFile(path)) {
                delete tex;
                std::string m = std::string("__native__sfml_texture_load_from_file: failed for '")
                              + path + "'";
                g_host->raiseError(ctx, kEx, m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_textures.insert(tex));
        }
        MTypeValue* nTextureDestroy(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_texture_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_textures.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextureSize(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_texture_size")) {
                return g_host->makeNull(ctx);
            }
            sf::Texture* t = g_textures.find(g_host->getInt(args[0]));
            unsigned w = 0, h = 0;
            if (t) { auto s = t->getSize(); w = s.x; h = s.y; }
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_INT, 2);
            g_host->arraySet(out, 0, g_host->makeInt(ctx, w));
            g_host->arraySet(out, 1, g_host->makeInt(ctx, h));
            return out;
        }

        /* ---- Sprite (always bound to a Texture at creation) ---- */

        MTypeValue* nSpriteCreate(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sprite_create")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Texture* tex = g_textures.find(g_host->getInt(args[0]));
            if (!tex) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_sprite_create: invalid texture id");
                return g_host->makeInt(ctx, 0);
            }
            auto* spr = new sf::Sprite(*tex);
            return g_host->makeInt(ctx, g_sprites.insert(spr));
        }
        MTypeValue* nSpriteDestroy(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sprite_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_sprites.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSpriteSetPosition(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_sprite_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return g_host->makeVoid(ctx);
            s->setPosition({static_cast<float>(g_host->getFloat(args[1])),
                             static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSpriteSetScale(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_sprite_set_scale")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return g_host->makeVoid(ctx);
            s->setScale({static_cast<float>(g_host->getFloat(args[1])),
                         static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSpriteSetRotation(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_sprite_set_rotation")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return g_host->makeVoid(ctx);
            s->setRotation(sf::degrees(static_cast<float>(g_host->getFloat(args[1]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSpriteSetOrigin(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_sprite_set_origin")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return g_host->makeVoid(ctx);
            s->setOrigin({static_cast<float>(g_host->getFloat(args[1])),
                           static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSpriteSetColor(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_sprite_set_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return g_host->makeVoid(ctx);
            s->setColor(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawSprite(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_draw_sprite")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[1]));
            if (w && s) w->draw(*s);
            return g_host->makeVoid(ctx);
        }

        /* ---- RectangleShape ---- */

        MTypeValue* nRectCreate(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_rect_create")) {
                return g_host->makeInt(ctx, 0);
            }
            float w = static_cast<float>(g_host->getFloat(args[0]));
            float h = static_cast<float>(g_host->getFloat(args[1]));
            auto* r = new sf::RectangleShape({w, h});
            return g_host->makeInt(ctx, g_rectShapes.insert(r));
        }
        MTypeValue* nRectDestroy(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_rect_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_rectShapes.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRectSetPosition(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_rect_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            r->setPosition({static_cast<float>(g_host->getFloat(args[1])),
                             static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRectSetSize(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_rect_set_size")) {
                return g_host->makeVoid(ctx);
            }
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            r->setSize({static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRectSetFillColor(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_rect_set_fill_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            r->setFillColor(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRectSetOutlineColor(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_rect_set_outline_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            r->setOutlineColor(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRectSetOutlineThickness(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_rect_set_outline_thickness")) {
                return g_host->makeVoid(ctx);
            }
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            r->setOutlineThickness(static_cast<float>(g_host->getFloat(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawRect(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_draw_rect")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[1]));
            if (w && r) w->draw(*r);
            return g_host->makeVoid(ctx);
        }

        /* ---- CircleShape ---- */

        MTypeValue* nCircleCreate(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_circle_create")) {
                return g_host->makeInt(ctx, 0);
            }
            float radius = static_cast<float>(g_host->getFloat(args[0]));
            auto* c = new sf::CircleShape(radius);
            return g_host->makeInt(ctx, g_circleShapes.insert(c));
        }
        MTypeValue* nCircleDestroy(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_circle_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_circleShapes.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nCircleSetPosition(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_circle_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeVoid(ctx);
            c->setPosition({static_cast<float>(g_host->getFloat(args[1])),
                             static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nCircleSetRadius(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_circle_set_radius")) {
                return g_host->makeVoid(ctx);
            }
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeVoid(ctx);
            c->setRadius(static_cast<float>(g_host->getFloat(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nCircleSetFillColor(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_circle_set_fill_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeVoid(ctx);
            c->setFillColor(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nCircleSetOutlineColor(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_circle_set_outline_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeVoid(ctx);
            c->setOutlineColor(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nCircleSetOutlineThickness(void*, MTypeContext* ctx,
                                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_circle_set_outline_thickness")) {
                return g_host->makeVoid(ctx);
            }
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeVoid(ctx);
            c->setOutlineThickness(static_cast<float>(g_host->getFloat(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawCircle(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_draw_circle")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[1]));
            if (w && c) w->draw(*c);
            return g_host->makeVoid(ctx);
        }

        /* ---- Font + Text ---- */

        MTypeValue* nFontOpenFromFile(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_font_open_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            auto* font = new sf::Font();
            if (!font->openFromFile(path)) {
                delete font;
                std::string m = std::string("__native__sfml_font_open_from_file: failed for '")
                              + path + "'";
                g_host->raiseError(ctx, kEx, m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_fonts.insert(font));
        }
        MTypeValue* nFontDestroy(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_font_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_fonts.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nTextCreate(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_text_create")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Font* font = g_fonts.find(g_host->getInt(args[0]));
            if (!font) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_text_create: invalid font id");
                return g_host->makeInt(ctx, 0);
            }
            std::string s = getStr(args[1]);
            unsigned size = static_cast<unsigned>(g_host->getInt(args[2]));
            auto* t = new sf::Text(*font, s, size);
            return g_host->makeInt(ctx, g_texts.insert(t));
        }
        MTypeValue* nTextDestroy(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_text_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_texts.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetString(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_text_set_string")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (!t) return g_host->makeVoid(ctx);
            t->setString(std::string(getStr(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetPosition(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_text_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (!t) return g_host->makeVoid(ctx);
            t->setPosition({static_cast<float>(g_host->getFloat(args[1])),
                             static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetCharacterSize(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_text_set_character_size")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (!t) return g_host->makeVoid(ctx);
            t->setCharacterSize(static_cast<unsigned>(g_host->getInt(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetFillColor(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_text_set_fill_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (!t) return g_host->makeVoid(ctx);
            t->setFillColor(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawText(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_draw_text")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::Text* t = g_texts.find(g_host->getInt(args[1]));
            if (w && t) w->draw(*t);
            return g_host->makeVoid(ctx);
        }
    }

    void registerGraphicsNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };

        /* Texture */
        reg("__native__sfml_texture_load_from_file", &nTextureLoadFromFile);
        reg("__native__sfml_texture_destroy",        &nTextureDestroy);
        reg("__native__sfml_texture_size",           &nTextureSize);

        /* Sprite */
        reg("__native__sfml_sprite_create",          &nSpriteCreate);
        reg("__native__sfml_sprite_destroy",         &nSpriteDestroy);
        reg("__native__sfml_sprite_set_position",    &nSpriteSetPosition);
        reg("__native__sfml_sprite_set_scale",       &nSpriteSetScale);
        reg("__native__sfml_sprite_set_rotation",    &nSpriteSetRotation);
        reg("__native__sfml_sprite_set_origin",      &nSpriteSetOrigin);
        reg("__native__sfml_sprite_set_color",       &nSpriteSetColor);
        reg("__native__sfml_window_draw_sprite",     &nWindowDrawSprite);

        /* Rectangle */
        reg("__native__sfml_rect_create",                  &nRectCreate);
        reg("__native__sfml_rect_destroy",                 &nRectDestroy);
        reg("__native__sfml_rect_set_position",            &nRectSetPosition);
        reg("__native__sfml_rect_set_size",                &nRectSetSize);
        reg("__native__sfml_rect_set_fill_color",          &nRectSetFillColor);
        reg("__native__sfml_rect_set_outline_color",       &nRectSetOutlineColor);
        reg("__native__sfml_rect_set_outline_thickness",   &nRectSetOutlineThickness);
        reg("__native__sfml_window_draw_rect",             &nWindowDrawRect);

        /* Circle */
        reg("__native__sfml_circle_create",                  &nCircleCreate);
        reg("__native__sfml_circle_destroy",                 &nCircleDestroy);
        reg("__native__sfml_circle_set_position",            &nCircleSetPosition);
        reg("__native__sfml_circle_set_radius",              &nCircleSetRadius);
        reg("__native__sfml_circle_set_fill_color",          &nCircleSetFillColor);
        reg("__native__sfml_circle_set_outline_color",       &nCircleSetOutlineColor);
        reg("__native__sfml_circle_set_outline_thickness",   &nCircleSetOutlineThickness);
        reg("__native__sfml_window_draw_circle",             &nWindowDrawCircle);

        /* Font + Text */
        reg("__native__sfml_font_open_from_file",  &nFontOpenFromFile);
        reg("__native__sfml_font_destroy",         &nFontDestroy);

        reg("__native__sfml_text_create",              &nTextCreate);
        reg("__native__sfml_text_destroy",             &nTextDestroy);
        reg("__native__sfml_text_set_string",          &nTextSetString);
        reg("__native__sfml_text_set_position",        &nTextSetPosition);
        reg("__native__sfml_text_set_character_size",  &nTextSetCharacterSize);
        reg("__native__sfml_text_set_fill_color",      &nTextSetFillColor);
        reg("__native__sfml_window_draw_text",         &nWindowDrawText);
    }
}
