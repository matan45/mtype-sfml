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
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/System/Angle.hpp>

#include <vector>

#include <string>
#include <unordered_set>

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

        /* Texture handles that point at a sf::Texture owned by some other
         * SFML object (currently: sf::RenderTexture::getTexture). Marked
         * here so nTextureDestroy doesn't `delete` a foreign pointer.
         * Lifetime warning: the user must destroy any borrowed Texture
         * before destroying the RenderTexture it came from. */
        std::unordered_set<std::int64_t> g_borrowedTextureIds;

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
            std::int64_t id = g_host->getInt(args[0]);
            sf::Texture* tex = g_textures.erase(id);
            /* If this handle points at a texture owned elsewhere (e.g.
             * borrowed from a RenderTexture), drop it from the registry
             * but don't free the pointer. */
            if (g_borrowedTextureIds.erase(id) == 0) {
                delete tex;
            }
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

        /* ---- VertexArray (procedural geometry) ---- */

        /* Create with a primitive type (sf::PrimitiveType — 0=Points,
         * 1=Lines, 2=LineStrip, 3=Triangles, 4=TriangleStrip,
         * 5=TriangleFan) and an initial vertex count. */
        MTypeValue* nVertexArrayCreate(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_vertex_array_create")) {
                return g_host->makeInt(ctx, 0);
            }
            int primitive  = static_cast<int>(g_host->getInt(args[0]));
            size_t initial = static_cast<size_t>(g_host->getInt(args[1]));
            auto* va = new sf::VertexArray(static_cast<sf::PrimitiveType>(primitive), initial);
            return g_host->makeInt(ctx, g_vertexArrays.insert(va));
        }
        MTypeValue* nVertexArrayDestroy(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_vertex_array_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_vertexArrays.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nVertexArrayResize(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_vertex_array_resize")) {
                return g_host->makeVoid(ctx);
            }
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[0]));
            if (!va) return g_host->makeVoid(ctx);
            va->resize(static_cast<size_t>(g_host->getInt(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nVertexArraySize(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_vertex_array_size")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[0]));
            return g_host->makeInt(ctx, va ? static_cast<int64_t>(va->getVertexCount()) : 0);
        }
        MTypeValue* nVertexArraySetPrimitive(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_vertex_array_set_primitive_type")) {
                return g_host->makeVoid(ctx);
            }
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[0]));
            if (!va) return g_host->makeVoid(ctx);
            va->setPrimitiveType(static_cast<sf::PrimitiveType>(g_host->getInt(args[1])));
            return g_host->makeVoid(ctx);
        }
        /* Set one vertex: index, position x/y, color r/g/b/a, texCoords u/v. */
        MTypeValue* nVertexArraySetVertex(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 10, "__native__sfml_vertex_array_set_vertex")) {
                return g_host->makeVoid(ctx);
            }
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[0]));
            if (!va) return g_host->makeVoid(ctx);
            size_t i = static_cast<size_t>(g_host->getInt(args[1]));
            if (i >= va->getVertexCount()) return g_host->makeVoid(ctx);
            auto& v = (*va)[i];
            v.position  = {static_cast<float>(g_host->getFloat(args[2])),
                            static_cast<float>(g_host->getFloat(args[3]))};
            v.color     = sf::Color(
                static_cast<std::uint8_t>(g_host->getInt(args[4])),
                static_cast<std::uint8_t>(g_host->getInt(args[5])),
                static_cast<std::uint8_t>(g_host->getInt(args[6])),
                static_cast<std::uint8_t>(g_host->getInt(args[7])));
            v.texCoords = {static_cast<float>(g_host->getFloat(args[8])),
                            static_cast<float>(g_host->getFloat(args[9]))};
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawVertexArray(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_draw_vertex_array")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[1]));
            if (w && va) w->draw(*va);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawVertexArrayTextured(void*, MTypeContext* ctx,
                                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_window_draw_vertex_array_textured")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[1]));
            sf::Texture* tex    = g_textures.find(g_host->getInt(args[2]));
            if (w && va) {
                sf::RenderStates states;
                states.texture = tex;
                w->draw(*va, states);
            }
            return g_host->makeVoid(ctx);
        }

        /* ---- View (camera / transform) ---- */

        MTypeValue* nViewCreate(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sfml_view_create")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Vector2f center{static_cast<float>(g_host->getFloat(args[0])),
                                 static_cast<float>(g_host->getFloat(args[1]))};
            sf::Vector2f size  {static_cast<float>(g_host->getFloat(args[2])),
                                 static_cast<float>(g_host->getFloat(args[3]))};
            auto* v = new sf::View(center, size);
            return g_host->makeInt(ctx, g_views.insert(v));
        }
        MTypeValue* nViewDestroy(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_view_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_views.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nViewSetCenter(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_view_set_center")) {
                return g_host->makeVoid(ctx);
            }
            sf::View* v = g_views.find(g_host->getInt(args[0]));
            if (!v) return g_host->makeVoid(ctx);
            v->setCenter({static_cast<float>(g_host->getFloat(args[1])),
                          static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nViewSetSize(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_view_set_size")) {
                return g_host->makeVoid(ctx);
            }
            sf::View* v = g_views.find(g_host->getInt(args[0]));
            if (!v) return g_host->makeVoid(ctx);
            v->setSize({static_cast<float>(g_host->getFloat(args[1])),
                        static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nViewSetRotation(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_view_set_rotation")) {
                return g_host->makeVoid(ctx);
            }
            sf::View* v = g_views.find(g_host->getInt(args[0]));
            if (!v) return g_host->makeVoid(ctx);
            v->setRotation(sf::degrees(static_cast<float>(g_host->getFloat(args[1]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nViewMove(void*, MTypeContext* ctx,
                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_view_move")) {
                return g_host->makeVoid(ctx);
            }
            sf::View* v = g_views.find(g_host->getInt(args[0]));
            if (!v) return g_host->makeVoid(ctx);
            v->move({static_cast<float>(g_host->getFloat(args[1])),
                     static_cast<float>(g_host->getFloat(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nViewZoom(void*, MTypeContext* ctx,
                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_view_zoom")) {
                return g_host->makeVoid(ctx);
            }
            sf::View* v = g_views.find(g_host->getInt(args[0]));
            if (!v) return g_host->makeVoid(ctx);
            v->zoom(static_cast<float>(g_host->getFloat(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nViewRotate(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_view_rotate")) {
                return g_host->makeVoid(ctx);
            }
            sf::View* v = g_views.find(g_host->getInt(args[0]));
            if (!v) return g_host->makeVoid(ctx);
            v->rotate(sf::degrees(static_cast<float>(g_host->getFloat(args[1]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetView(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_view")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::View* v         = g_views.find(g_host->getInt(args[1]));
            if (w && v) w->setView(*v);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowResetView(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_reset_view")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setView(w->getDefaultView());
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 3 — RenderTexture (offscreen target).
         *
         * Same surface as RenderWindow's draw side: clear, draw_*,
         * display, set_view, reset_view. Plus get_texture which returns
         * a *borrowed* Texture handle pointing into the RT's internal
         * framebuffer texture.
         * ---------------------------------------------------------------- */

        MTypeValue* nRenderTextureCreate(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_create")) {
                return g_host->makeInt(ctx, 0);
            }
            unsigned w = static_cast<unsigned>(g_host->getInt(args[0]));
            unsigned h = static_cast<unsigned>(g_host->getInt(args[1]));
            sf::RenderTexture* rt = nullptr;
            try {
                rt = new sf::RenderTexture({w, h});
            } catch (const std::exception& e) {
                std::string m = std::string("__native__sfml_render_texture_create: ") + e.what();
                g_host->raiseError(ctx, kEx, m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_renderTextures.insert(rt));
        }
        MTypeValue* nRenderTextureDestroy(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_render_texture_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_renderTextures.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureResize(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_render_texture_resize")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            if (!rt) return g_host->makeBool(ctx, 0);
            unsigned w = static_cast<unsigned>(g_host->getInt(args[1]));
            unsigned h = static_cast<unsigned>(g_host->getInt(args[2]));
            return g_host->makeBool(ctx, rt->resize({w, h}) ? 1 : 0);
        }
        MTypeValue* nRenderTextureSize(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_render_texture_size")) {
                return g_host->makeNull(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            unsigned w = 0, h = 0;
            if (rt) { auto s = rt->getSize(); w = s.x; h = s.y; }
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_INT, 2);
            g_host->arraySet(out, 0, g_host->makeInt(ctx, w));
            g_host->arraySet(out, 1, g_host->makeInt(ctx, h));
            return out;
        }
        MTypeValue* nRenderTextureClear(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_render_texture_clear")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            if (!rt) return g_host->makeVoid(ctx);
            rt->clear(colorFromArgs(ctx, args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureDisplay(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_render_texture_display")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            if (rt) rt->display();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureSetView(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_set_view")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::View* v           = g_views.find(g_host->getInt(args[1]));
            if (rt && v) rt->setView(*v);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureResetView(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_render_texture_reset_view")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            if (rt) rt->setView(rt->getDefaultView());
            return g_host->makeVoid(ctx);
        }

        /* Get a borrowed Texture handle backed by the RT's framebuffer.
         * The handle is registered in g_textures but flagged as borrowed
         * — Texture.destroy() will release the handle without freeing
         * the underlying pointer. Don't destroy the RT before destroying
         * any borrowed textures from it. */
        MTypeValue* nRenderTextureGetTexture(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_render_texture_get_texture")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            if (!rt) return g_host->makeInt(ctx, 0);
            sf::Texture* tex = const_cast<sf::Texture*>(&rt->getTexture());
            std::int64_t id = g_textures.insert(tex);
            g_borrowedTextureIds.insert(id);
            return g_host->makeInt(ctx, id);
        }

        /* RenderTexture draw natives — same drawables as RenderWindow.
         * Implementation is a one-line per drawable. */
        MTypeValue* nRenderTextureDrawSprite(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_draw_sprite")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[1]));
            if (rt && s) rt->draw(*s);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureDrawRect(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_draw_rect")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::RectangleShape* r = g_rectShapes.find(g_host->getInt(args[1]));
            if (rt && r) rt->draw(*r);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureDrawCircle(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_draw_circle")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::CircleShape* c = g_circleShapes.find(g_host->getInt(args[1]));
            if (rt && c) rt->draw(*c);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureDrawText(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_draw_text")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::Text* t = g_texts.find(g_host->getInt(args[1]));
            if (rt && t) rt->draw(*t);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureDrawVertexArray(void*, MTypeContext* ctx,
                                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_draw_vertex_array")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[1]));
            if (rt && va) rt->draw(*va);
            return g_host->makeVoid(ctx);
        }

        /* ----------------------------------------------------------------
         * Phase 4 — Shader.
         *
         * GLSL fragment / vertex programs. Loaded from disk (loadFromFile)
         * or from a mType string (loadFragmentFromMemory). Uniforms set
         * by name with typed setters. Pass via shader-aware draw natives
         * to apply during rendering.
         * ---------------------------------------------------------------- */

        MTypeValue* nShaderCreate(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sfml_shader_create")) {
                return g_host->makeInt(ctx, 0);
            }
            auto* sh = new sf::Shader();
            return g_host->makeInt(ctx, g_shaders.insert(sh));
        }
        MTypeValue* nShaderDestroy(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_shader_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_shaders.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        /* type: 0=Vertex, 1=Fragment, 2=Geometry — matches sf::Shader::Type. */
        MTypeValue* nShaderLoadFromFile(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_load_from_file")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeBool(ctx, 0);
            const char* path = getStr(args[1]);
            int type = static_cast<int>(g_host->getInt(args[2]));
            return g_host->makeBool(ctx,
                sh->loadFromFile(path, static_cast<sf::Shader::Type>(type)) ? 1 : 0);
        }
        MTypeValue* nShaderLoadVertFragFromFile(void*, MTypeContext* ctx,
                                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_load_vert_frag_from_file")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeBool(ctx, 0);
            const char* vp = getStr(args[1]);
            const char* fp = getStr(args[2]);
            return g_host->makeBool(ctx, sh->loadFromFile(vp, fp) ? 1 : 0);
        }
        /* Load a fragment shader from an in-memory GLSL source string. */
        MTypeValue* nShaderLoadFragFromMemory(void*, MTypeContext* ctx,
                                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_shader_load_frag_from_memory")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeBool(ctx, 0);
            return g_host->makeBool(ctx,
                sh->loadFromMemory(getStr(args[1]), sf::Shader::Type::Fragment) ? 1 : 0);
        }
        MTypeValue* nShaderSetUniformFloat(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_float")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), static_cast<float>(g_host->getFloat(args[2])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformVec2(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sfml_shader_set_uniform_vec2")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Glsl::Vec2(
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformVec3(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_shader_set_uniform_vec3")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Glsl::Vec3(
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformVec4(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__sfml_shader_set_uniform_vec4")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Glsl::Vec4(
                static_cast<float>(g_host->getFloat(args[2])),
                static_cast<float>(g_host->getFloat(args[3])),
                static_cast<float>(g_host->getFloat(args[4])),
                static_cast<float>(g_host->getFloat(args[5]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformInt(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_int")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), static_cast<int>(g_host->getInt(args[2])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformTexture(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_texture")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            sf::Texture* tex = g_textures.find(g_host->getInt(args[2]));
            if (!sh || !tex) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), *tex);
            return g_host->makeVoid(ctx);
        }
        /* CurrentTexture sentinel — binds the drawable's own texture
         * unit as the named sampler. Most common uniform shape for
         * post-processing shaders. */
        MTypeValue* nShaderSetUniformCurrentTexture(void*, MTypeContext* ctx,
                                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_shader_set_uniform_current_texture")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Shader::CurrentTexture);
            return g_host->makeVoid(ctx);
        }

        /* Shader-aware draws. Pass nullptr shader to draw without one. */
        MTypeValue* nWindowDrawSpriteShader(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_window_draw_sprite_shader")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[1]));
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[2]));
            if (w && s) {
                sf::RenderStates states;
                states.shader = sh;
                w->draw(*s, states);
            }
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawVertexArrayShader(void*, MTypeContext* ctx,
                                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sfml_window_draw_vertex_array_shader")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::VertexArray* va = g_vertexArrays.find(g_host->getInt(args[1]));
            sf::Texture* tex    = g_textures.find(g_host->getInt(args[2]));
            sf::Shader* sh      = g_shaders.find(g_host->getInt(args[3]));
            if (w && va) {
                sf::RenderStates states;
                states.texture = tex;
                states.shader  = sh;
                w->draw(*va, states);
            }
            return g_host->makeVoid(ctx);
        }

        /* ====================================================================
         * Phase 4 additions: transform parity, ConvexShape, Image, advanced
         * Text styling, advanced shader uniforms.
         * ==================================================================== */

        /* Helper to flatten a sf::FloatRect to a [x, y, w, h] float[4]. */
        inline MTypeValue* makeRect(MTypeContext* ctx, const sf::FloatRect& r) {
            return detail::makeFloatQuad(ctx, r.position.x, r.position.y,
                                              r.size.x,     r.size.y);
        }

        /* ---- Sprite bounds (parity with rect/circle/text) ---- */

        MTypeValue* nSpriteGlobalBounds(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sprite_global_bounds")) {
                return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            return makeRect(ctx, s->getGlobalBounds());
        }
        MTypeValue* nSpriteLocalBounds(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sprite_local_bounds")) {
                return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            }
            sf::Sprite* s = g_sprites.find(g_host->getInt(args[0]));
            if (!s) return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            return makeRect(ctx, s->getLocalBounds());
        }

        /* ---- Generic shape-transform shim ----
         * Rect/Circle/ConvexShape all inherit from sf::Shape; Text from
         * sf::Transformable. The pattern below assumes only a Transformable&
         * is needed, so we templatize over the registry. */
        template <typename Reg>
        MTypeValue* setScaleImpl(MTypeContext* ctx, Reg& reg,
                                   const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 3, op)) return g_host->makeVoid(ctx);
            auto* p = reg.find(g_host->getInt(args[0]));
            if (p) p->setScale({detail::getF(args[1]), detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
        template <typename Reg>
        MTypeValue* setRotationImpl(MTypeContext* ctx, Reg& reg,
                                       const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 2, op)) return g_host->makeVoid(ctx);
            auto* p = reg.find(g_host->getInt(args[0]));
            if (p) p->setRotation(sf::degrees(detail::getF(args[1])));
            return g_host->makeVoid(ctx);
        }
        template <typename Reg>
        MTypeValue* setOriginImpl(MTypeContext* ctx, Reg& reg,
                                     const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 3, op)) return g_host->makeVoid(ctx);
            auto* p = reg.find(g_host->getInt(args[0]));
            if (p) p->setOrigin({detail::getF(args[1]), detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
        template <typename Reg>
        MTypeValue* moveImpl(MTypeContext* ctx, Reg& reg,
                                const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 3, op)) return g_host->makeVoid(ctx);
            auto* p = reg.find(g_host->getInt(args[0]));
            if (p) p->move({detail::getF(args[1]), detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
        template <typename Reg>
        MTypeValue* rotateImpl(MTypeContext* ctx, Reg& reg,
                                  const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 2, op)) return g_host->makeVoid(ctx);
            auto* p = reg.find(g_host->getInt(args[0]));
            if (p) p->rotate(sf::degrees(detail::getF(args[1])));
            return g_host->makeVoid(ctx);
        }
        template <typename Reg>
        MTypeValue* scaleImpl(MTypeContext* ctx, Reg& reg,
                                 const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 3, op)) return g_host->makeVoid(ctx);
            auto* p = reg.find(g_host->getInt(args[0]));
            if (p) p->scale({detail::getF(args[1]), detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
        template <typename Reg>
        MTypeValue* globalBoundsImpl(MTypeContext* ctx, Reg& reg,
                                        const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 1, op)) {
                return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            }
            auto* p = reg.find(g_host->getInt(args[0]));
            if (!p) return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            return makeRect(ctx, p->getGlobalBounds());
        }
        template <typename Reg>
        MTypeValue* localBoundsImpl(MTypeContext* ctx, Reg& reg,
                                       const MTypeValue* const* args, int argc, const char* op)
        {
            if (!requireArgs(ctx, argc, 1, op)) {
                return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            }
            auto* p = reg.find(g_host->getInt(args[0]));
            if (!p) return detail::makeFloatQuad(ctx, 0, 0, 0, 0);
            return makeRect(ctx, p->getLocalBounds());
        }

        /* Generate `n<Type>SetScale` etc. — each is a one-line forward to the
         * templated impl above, so the registration table stays homogeneous. */
        #define SHAPE_TRANSFORM(N, Reg, NameLower) \
            MTypeValue* n##N##SetScale(void*, MTypeContext* ctx, \
                                          const MTypeValue* const* args, int argc) \
            { return setScaleImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_set_scale"); } \
            MTypeValue* n##N##SetRotation(void*, MTypeContext* ctx, \
                                             const MTypeValue* const* args, int argc) \
            { return setRotationImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_set_rotation"); } \
            MTypeValue* n##N##SetOrigin(void*, MTypeContext* ctx, \
                                           const MTypeValue* const* args, int argc) \
            { return setOriginImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_set_origin"); } \
            MTypeValue* n##N##Move(void*, MTypeContext* ctx, \
                                       const MTypeValue* const* args, int argc) \
            { return moveImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_move"); } \
            MTypeValue* n##N##Rotate(void*, MTypeContext* ctx, \
                                         const MTypeValue* const* args, int argc) \
            { return rotateImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_rotate"); } \
            MTypeValue* n##N##Scale(void*, MTypeContext* ctx, \
                                        const MTypeValue* const* args, int argc) \
            { return scaleImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_scale"); } \
            MTypeValue* n##N##GlobalBounds(void*, MTypeContext* ctx, \
                                               const MTypeValue* const* args, int argc) \
            { return globalBoundsImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_global_bounds"); } \
            MTypeValue* n##N##LocalBounds(void*, MTypeContext* ctx, \
                                              const MTypeValue* const* args, int argc) \
            { return localBoundsImpl(ctx, Reg, args, argc, "__native__sfml_" NameLower "_local_bounds"); }

        SHAPE_TRANSFORM(Rect,   g_rectShapes,   "rect")
        SHAPE_TRANSFORM(Circle, g_circleShapes, "circle")
        SHAPE_TRANSFORM(Convex, g_convexShapes, "convex")
        SHAPE_TRANSFORM(Text,   g_texts,        "text")

        #undef SHAPE_TRANSFORM

        /* ---- ConvexShape (polygonal shape, sibling of Rect/Circle) ---- */

        MTypeValue* nConvexCreate(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_convex_create")) {
                return g_host->makeInt(ctx, 0);
            }
            std::size_t n = static_cast<std::size_t>(g_host->getInt(args[0]));
            auto* c = new sf::ConvexShape(n);
            return g_host->makeInt(ctx, g_convexShapes.insert(c));
        }
        MTypeValue* nConvexDestroy(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_convex_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_convexShapes.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nConvexSetPointCount(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_convex_set_point_count")) {
                return g_host->makeVoid(ctx);
            }
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[0]));
            if (c) c->setPointCount(static_cast<std::size_t>(g_host->getInt(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nConvexSetPoint(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sfml_convex_set_point")) {
                return g_host->makeVoid(ctx);
            }
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[0]));
            if (c) c->setPoint(static_cast<std::size_t>(g_host->getInt(args[1])),
                                {detail::getF(args[2]), detail::getF(args[3])});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nConvexSetPosition(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_convex_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[0]));
            if (c) c->setPosition({detail::getF(args[1]), detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nConvexSetFillColor(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_convex_set_fill_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[0]));
            if (c) c->setFillColor(detail::getColor(args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nConvexSetOutlineColor(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_convex_set_outline_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[0]));
            if (c) c->setOutlineColor(detail::getColor(args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nConvexSetOutlineThickness(void*, MTypeContext* ctx,
                                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_convex_set_outline_thickness")) {
                return g_host->makeVoid(ctx);
            }
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[0]));
            if (c) c->setOutlineThickness(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowDrawConvex(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_draw_convex")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[1]));
            if (w && c) w->draw(*c);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nRenderTextureDrawConvex(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_render_texture_draw_convex")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderTexture* rt = g_renderTextures.find(g_host->getInt(args[0]));
            sf::ConvexShape* c = g_convexShapes.find(g_host->getInt(args[1]));
            if (rt && c) rt->draw(*c);
            return g_host->makeVoid(ctx);
        }

        /* ---- Image (pixel data; CPU-side counterpart to Texture) ---- */

        MTypeValue* nImageCreate(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__sfml_image_create")) {
                return g_host->makeInt(ctx, 0);
            }
            unsigned w = detail::getU(args[0]);
            unsigned h = detail::getU(args[1]);
            sf::Color c = detail::getColor(args, 2);
            auto* img = new sf::Image({w, h}, c);
            return g_host->makeInt(ctx, g_images.insert(img));
        }
        MTypeValue* nImageLoadFromFile(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_image_load_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            auto* img = new sf::Image();
            if (!img->loadFromFile(path)) {
                delete img;
                std::string m = std::string("__native__sfml_image_load_from_file: failed for '")
                              + path + "'";
                g_host->raiseError(ctx, kEx, m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_images.insert(img));
        }
        MTypeValue* nImageDestroy(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_image_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_images.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImageSaveToFile(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_image_save_to_file")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::Image* img = g_images.find(g_host->getInt(args[0]));
            if (!img) return g_host->makeBool(ctx, 0);
            return g_host->makeBool(ctx, img->saveToFile(getStr(args[1])) ? 1 : 0);
        }
        MTypeValue* nImageSize(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_image_size")) {
                return detail::makeIntPair(ctx, 0, 0);
            }
            sf::Image* img = g_images.find(g_host->getInt(args[0]));
            if (!img) return detail::makeIntPair(ctx, 0, 0);
            auto s = img->getSize();
            return detail::makeIntPair(ctx, s.x, s.y);
        }
        MTypeValue* nImageGetPixel(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_image_get_pixel")) {
                return detail::makeIntQuad(ctx, 0, 0, 0, 0);
            }
            sf::Image* img = g_images.find(g_host->getInt(args[0]));
            if (!img) return detail::makeIntQuad(ctx, 0, 0, 0, 0);
            unsigned x = detail::getU(args[1]);
            unsigned y = detail::getU(args[2]);
            auto sz = img->getSize();
            if (x >= sz.x || y >= sz.y) return detail::makeIntQuad(ctx, 0, 0, 0, 0);
            sf::Color c = img->getPixel({x, y});
            return detail::makeIntQuad(ctx, c.r, c.g, c.b, c.a);
        }
        MTypeValue* nImageSetPixel(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 7, "__native__sfml_image_set_pixel")) {
                return g_host->makeVoid(ctx);
            }
            sf::Image* img = g_images.find(g_host->getInt(args[0]));
            if (!img) return g_host->makeVoid(ctx);
            unsigned x = detail::getU(args[1]);
            unsigned y = detail::getU(args[2]);
            auto sz = img->getSize();
            if (x >= sz.x || y >= sz.y) return g_host->makeVoid(ctx);
            img->setPixel({x, y}, detail::getColor(args, 3));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nImageCreateMaskFromColor(void*, MTypeContext* ctx,
                                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__sfml_image_create_mask_from_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::Image* img = g_images.find(g_host->getInt(args[0]));
            if (!img) return g_host->makeVoid(ctx);
            sf::Color match = detail::getColor(args, 1);
            std::uint8_t alpha = detail::getU8(args[5]);
            img->createMaskFromColor(match, alpha);
            return g_host->makeVoid(ctx);
        }

        /* Texture <-> Image conversions. */
        MTypeValue* nTextureLoadFromImage(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_texture_load_from_image")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Image* img = g_images.find(g_host->getInt(args[0]));
            if (!img) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_texture_load_from_image: invalid image id");
                return g_host->makeInt(ctx, 0);
            }
            auto* tex = new sf::Texture();
            if (!tex->loadFromImage(*img)) {
                delete tex;
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_texture_load_from_image: loadFromImage failed");
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_textures.insert(tex));
        }
        MTypeValue* nTextureCopyToImage(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_texture_copy_to_image")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Texture* tex = g_textures.find(g_host->getInt(args[0]));
            if (!tex) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_texture_copy_to_image: invalid texture id");
                return g_host->makeInt(ctx, 0);
            }
            auto* img = new sf::Image(tex->copyToImage());
            return g_host->makeInt(ctx, g_images.insert(img));
        }
        MTypeValue* nTextureUpdateFromImage(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_texture_update_from_image")) {
                return g_host->makeVoid(ctx);
            }
            sf::Texture* tex = g_textures.find(g_host->getInt(args[0]));
            sf::Image*   img = g_images.find(g_host->getInt(args[1]));
            if (tex && img) tex->update(*img);
            return g_host->makeVoid(ctx);
        }

        /* ---- Text advanced styling ---- */

        MTypeValue* nTextSetOutlineColor(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_text_set_outline_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (t) t->setOutlineColor(detail::getColor(args, 1));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetOutlineThickness(void*, MTypeContext* ctx,
                                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_text_set_outline_thickness")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (t) t->setOutlineThickness(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetStyle(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_text_set_style")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (t) t->setStyle(static_cast<std::uint32_t>(g_host->getInt(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetLetterSpacing(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_text_set_letter_spacing")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (t) t->setLetterSpacing(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nTextSetLineSpacing(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_text_set_line_spacing")) {
                return g_host->makeVoid(ctx);
            }
            sf::Text* t = g_texts.find(g_host->getInt(args[0]));
            if (t) t->setLineSpacing(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }

        /* ---- Advanced shader uniforms ---- */

        MTypeValue* nShaderLoadVertFragFromMemory(void*, MTypeContext* ctx,
                                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_load_vert_frag_from_memory")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeBool(ctx, 0);
            return g_host->makeBool(ctx,
                sh->loadFromMemory(getStr(args[1]), getStr(args[2])) ? 1 : 0);
        }
        MTypeValue* nShaderSetUniformBool(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_bool")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (sh) sh->setUniform(getStr(args[1]), detail::getB(args[2]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformIvec2(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sfml_shader_set_uniform_ivec2")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Glsl::Ivec2(
                static_cast<int>(g_host->getInt(args[2])),
                static_cast<int>(g_host->getInt(args[3]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformIvec3(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_shader_set_uniform_ivec3")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Glsl::Ivec3(
                static_cast<int>(g_host->getInt(args[2])),
                static_cast<int>(g_host->getInt(args[3])),
                static_cast<int>(g_host->getInt(args[4]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformIvec4(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__sfml_shader_set_uniform_ivec4")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            sh->setUniform(getStr(args[1]), sf::Glsl::Ivec4(
                static_cast<int>(g_host->getInt(args[2])),
                static_cast<int>(g_host->getInt(args[3])),
                static_cast<int>(g_host->getInt(args[4])),
                static_cast<int>(g_host->getInt(args[5]))));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformColor(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 6, "__native__sfml_shader_set_uniform_color")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            /* sf::Glsl::Vec4 is constructible from sf::Color (normalizes 0-255 -> 0-1). */
            sh->setUniform(getStr(args[1]), sf::Glsl::Vec4(detail::getColor(args, 2)));
            return g_host->makeVoid(ctx);
        }
        /* Read an mType float[] into a flat std::vector<float>. Used by mat/
         * array uniform setters. Returns empty vector on type mismatch. */
        inline std::vector<float> readFloatArray(const MTypeValue* arr)
        {
            std::vector<float> out;
            if (g_host->getTag(arr) != MT_TAG_ARRAY) return out;
            std::size_t n = g_host->arrayLen(arr);
            out.reserve(n);
            for (std::size_t i = 0; i < n; ++i) {
                const MTypeValue* v = g_host->arrayGet(nullptr, arr, i);
                out.push_back(static_cast<float>(g_host->getFloat(v)));
            }
            return out;
        }
        MTypeValue* nShaderSetUniformMat3(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_mat3")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            auto vals = readFloatArray(args[2]);
            if (vals.size() != 9) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_shader_set_uniform_mat3: array must have length 9");
                return g_host->makeVoid(ctx);
            }
            sh->setUniform(getStr(args[1]), sf::Glsl::Mat3(vals.data()));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformMat4(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_mat4")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            auto vals = readFloatArray(args[2]);
            if (vals.size() != 16) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_shader_set_uniform_mat4: array must have length 16");
                return g_host->makeVoid(ctx);
            }
            sh->setUniform(getStr(args[1]), sf::Glsl::Mat4(vals.data()));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformFloatArray(void*, MTypeContext* ctx,
                                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_float_array")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            auto vals = readFloatArray(args[2]);
            sh->setUniformArray(getStr(args[1]), vals.data(), vals.size());
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nShaderSetUniformVec4Array(void*, MTypeContext* ctx,
                                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_shader_set_uniform_vec4_array")) {
                return g_host->makeVoid(ctx);
            }
            sf::Shader* sh = g_shaders.find(g_host->getInt(args[0]));
            if (!sh) return g_host->makeVoid(ctx);
            auto vals = readFloatArray(args[2]);
            if (vals.size() % 4 != 0) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_shader_set_uniform_vec4_array: array length must be multiple of 4");
                return g_host->makeVoid(ctx);
            }
            std::vector<sf::Glsl::Vec4> packed;
            packed.reserve(vals.size() / 4);
            for (std::size_t i = 0; i < vals.size(); i += 4) {
                packed.emplace_back(vals[i], vals[i+1], vals[i+2], vals[i+3]);
            }
            sh->setUniformArray(getStr(args[1]), packed.data(), packed.size());
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

        /* VertexArray */
        reg("__native__sfml_vertex_array_create",              &nVertexArrayCreate);
        reg("__native__sfml_vertex_array_destroy",             &nVertexArrayDestroy);
        reg("__native__sfml_vertex_array_resize",              &nVertexArrayResize);
        reg("__native__sfml_vertex_array_size",                &nVertexArraySize);
        reg("__native__sfml_vertex_array_set_primitive_type",  &nVertexArraySetPrimitive);
        reg("__native__sfml_vertex_array_set_vertex",          &nVertexArraySetVertex);
        reg("__native__sfml_window_draw_vertex_array",         &nWindowDrawVertexArray);
        reg("__native__sfml_window_draw_vertex_array_textured",&nWindowDrawVertexArrayTextured);

        /* View */
        reg("__native__sfml_view_create",        &nViewCreate);
        reg("__native__sfml_view_destroy",       &nViewDestroy);
        reg("__native__sfml_view_set_center",    &nViewSetCenter);
        reg("__native__sfml_view_set_size",      &nViewSetSize);
        reg("__native__sfml_view_set_rotation",  &nViewSetRotation);
        reg("__native__sfml_view_move",          &nViewMove);
        reg("__native__sfml_view_zoom",          &nViewZoom);
        reg("__native__sfml_view_rotate",        &nViewRotate);
        reg("__native__sfml_window_set_view",    &nWindowSetView);
        reg("__native__sfml_window_reset_view",  &nWindowResetView);

        /* RenderTexture */
        reg("__native__sfml_render_texture_create",                &nRenderTextureCreate);
        reg("__native__sfml_render_texture_destroy",               &nRenderTextureDestroy);
        reg("__native__sfml_render_texture_resize",                &nRenderTextureResize);
        reg("__native__sfml_render_texture_size",                  &nRenderTextureSize);
        reg("__native__sfml_render_texture_clear",                 &nRenderTextureClear);
        reg("__native__sfml_render_texture_display",               &nRenderTextureDisplay);
        reg("__native__sfml_render_texture_set_view",              &nRenderTextureSetView);
        reg("__native__sfml_render_texture_reset_view",            &nRenderTextureResetView);
        reg("__native__sfml_render_texture_get_texture",           &nRenderTextureGetTexture);
        reg("__native__sfml_render_texture_draw_sprite",           &nRenderTextureDrawSprite);
        reg("__native__sfml_render_texture_draw_rect",             &nRenderTextureDrawRect);
        reg("__native__sfml_render_texture_draw_circle",           &nRenderTextureDrawCircle);
        reg("__native__sfml_render_texture_draw_text",             &nRenderTextureDrawText);
        reg("__native__sfml_render_texture_draw_vertex_array",     &nRenderTextureDrawVertexArray);

        /* Shader */
        reg("__native__sfml_shader_create",                       &nShaderCreate);
        reg("__native__sfml_shader_destroy",                      &nShaderDestroy);
        reg("__native__sfml_shader_load_from_file",               &nShaderLoadFromFile);
        reg("__native__sfml_shader_load_vert_frag_from_file",     &nShaderLoadVertFragFromFile);
        reg("__native__sfml_shader_load_frag_from_memory",        &nShaderLoadFragFromMemory);
        reg("__native__sfml_shader_set_uniform_float",            &nShaderSetUniformFloat);
        reg("__native__sfml_shader_set_uniform_vec2",             &nShaderSetUniformVec2);
        reg("__native__sfml_shader_set_uniform_vec3",             &nShaderSetUniformVec3);
        reg("__native__sfml_shader_set_uniform_vec4",             &nShaderSetUniformVec4);
        reg("__native__sfml_shader_set_uniform_int",              &nShaderSetUniformInt);
        reg("__native__sfml_shader_set_uniform_texture",          &nShaderSetUniformTexture);
        reg("__native__sfml_shader_set_uniform_current_texture",  &nShaderSetUniformCurrentTexture);
        reg("__native__sfml_window_draw_sprite_shader",           &nWindowDrawSpriteShader);
        reg("__native__sfml_window_draw_vertex_array_shader",     &nWindowDrawVertexArrayShader);

        /* ---- Phase 4 additions ---- */

        /* Sprite bounds (the other shapes get theirs from the macro block below). */
        reg("__native__sfml_sprite_global_bounds",     &nSpriteGlobalBounds);
        reg("__native__sfml_sprite_local_bounds",      &nSpriteLocalBounds);

        /* Transform parity — rect/circle/convex/text get the full 8-func set. */
        #define REG_SHAPE_TRANSFORM(NameLower, N) \
            reg("__native__sfml_" NameLower "_set_scale",     &n##N##SetScale); \
            reg("__native__sfml_" NameLower "_set_rotation",  &n##N##SetRotation); \
            reg("__native__sfml_" NameLower "_set_origin",    &n##N##SetOrigin); \
            reg("__native__sfml_" NameLower "_move",          &n##N##Move); \
            reg("__native__sfml_" NameLower "_rotate",        &n##N##Rotate); \
            reg("__native__sfml_" NameLower "_scale",         &n##N##Scale); \
            reg("__native__sfml_" NameLower "_global_bounds", &n##N##GlobalBounds); \
            reg("__native__sfml_" NameLower "_local_bounds",  &n##N##LocalBounds);

        REG_SHAPE_TRANSFORM("rect",   Rect)
        REG_SHAPE_TRANSFORM("circle", Circle)
        REG_SHAPE_TRANSFORM("convex", Convex)
        REG_SHAPE_TRANSFORM("text",   Text)
        #undef REG_SHAPE_TRANSFORM

        /* ConvexShape lifecycle / point setters / color / draw. */
        reg("__native__sfml_convex_create",                  &nConvexCreate);
        reg("__native__sfml_convex_destroy",                 &nConvexDestroy);
        reg("__native__sfml_convex_set_point_count",         &nConvexSetPointCount);
        reg("__native__sfml_convex_set_point",               &nConvexSetPoint);
        reg("__native__sfml_convex_set_position",            &nConvexSetPosition);
        reg("__native__sfml_convex_set_fill_color",          &nConvexSetFillColor);
        reg("__native__sfml_convex_set_outline_color",       &nConvexSetOutlineColor);
        reg("__native__sfml_convex_set_outline_thickness",   &nConvexSetOutlineThickness);
        reg("__native__sfml_window_draw_convex",             &nWindowDrawConvex);
        reg("__native__sfml_render_texture_draw_convex",     &nRenderTextureDrawConvex);

        /* Image. */
        reg("__native__sfml_image_create",                   &nImageCreate);
        reg("__native__sfml_image_load_from_file",           &nImageLoadFromFile);
        reg("__native__sfml_image_destroy",                  &nImageDestroy);
        reg("__native__sfml_image_save_to_file",             &nImageSaveToFile);
        reg("__native__sfml_image_size",                     &nImageSize);
        reg("__native__sfml_image_get_pixel",                &nImageGetPixel);
        reg("__native__sfml_image_set_pixel",                &nImageSetPixel);
        reg("__native__sfml_image_create_mask_from_color",   &nImageCreateMaskFromColor);
        reg("__native__sfml_texture_load_from_image",        &nTextureLoadFromImage);
        reg("__native__sfml_texture_copy_to_image",          &nTextureCopyToImage);
        reg("__native__sfml_texture_update_from_image",      &nTextureUpdateFromImage);

        /* Text advanced styling. */
        reg("__native__sfml_text_set_outline_color",         &nTextSetOutlineColor);
        reg("__native__sfml_text_set_outline_thickness",     &nTextSetOutlineThickness);
        reg("__native__sfml_text_set_style",                 &nTextSetStyle);
        reg("__native__sfml_text_set_letter_spacing",        &nTextSetLetterSpacing);
        reg("__native__sfml_text_set_line_spacing",          &nTextSetLineSpacing);

        /* Advanced shader uniforms. */
        reg("__native__sfml_shader_load_vert_frag_from_memory", &nShaderLoadVertFragFromMemory);
        reg("__native__sfml_shader_set_uniform_bool",           &nShaderSetUniformBool);
        reg("__native__sfml_shader_set_uniform_ivec2",          &nShaderSetUniformIvec2);
        reg("__native__sfml_shader_set_uniform_ivec3",          &nShaderSetUniformIvec3);
        reg("__native__sfml_shader_set_uniform_ivec4",          &nShaderSetUniformIvec4);
        reg("__native__sfml_shader_set_uniform_color",          &nShaderSetUniformColor);
        reg("__native__sfml_shader_set_uniform_mat3",           &nShaderSetUniformMat3);
        reg("__native__sfml_shader_set_uniform_mat4",           &nShaderSetUniformMat4);
        reg("__native__sfml_shader_set_uniform_float_array",    &nShaderSetUniformFloatArray);
        reg("__native__sfml_shader_set_uniform_vec4_array",     &nShaderSetUniformVec4Array);
    }
}
