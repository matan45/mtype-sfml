/*
 * SFML 3 window + event bindings for mType.
 *
 * Native names follow the mType "__module_op" convention.
 *
 * SFML 3 changed the event model: pollEvent() returns std::optional<Event>
 * and sf::Event itself wraps a std::variant<Closed, Resized, KeyPressed, ...>.
 * We translate each variant subtype to a stable integer "event tag" that
 * mType code compares against — same pattern as the SDL plugin's
 * sdl_event_*_id constants.
 */

#include "PluginGlobals.hpp"
#include "BindingHelpers.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/System/String.hpp>

#include <imgui-SFML.h>

#include <optional>
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

        /* Event tags — stable integers the mType side compares against.
         * Don't reorder: scripts may have constants baked in. */
        enum EventTag : int {
            EVT_NONE                  = 0,
            EVT_CLOSED                = 1,
            EVT_RESIZED               = 2,
            EVT_KEY_PRESSED           = 3,
            EVT_KEY_RELEASED          = 4,
            EVT_MOUSE_MOVED           = 5,
            EVT_MOUSE_BUTTON_PRESSED  = 6,
            EVT_MOUSE_BUTTON_RELEASED = 7,
            EVT_MOUSE_WHEEL_SCROLLED  = 8,
            EVT_TEXT_ENTERED          = 9,
            EVT_FOCUS_GAINED          = 10,
            EVT_FOCUS_LOST            = 11,
        };

        /* Most-recently-polled event + its tag. Copies are cheap because
         * sf::Event is a small variant. */
        std::optional<sf::Event> g_lastEvent;
        EventTag                 g_lastTag = EVT_NONE;

        EventTag classify(const sf::Event& e)
        {
            if (e.is<sf::Event::Closed>())              return EVT_CLOSED;
            if (e.is<sf::Event::Resized>())             return EVT_RESIZED;
            if (e.is<sf::Event::KeyPressed>())          return EVT_KEY_PRESSED;
            if (e.is<sf::Event::KeyReleased>())         return EVT_KEY_RELEASED;
            if (e.is<sf::Event::MouseMoved>())          return EVT_MOUSE_MOVED;
            if (e.is<sf::Event::MouseButtonPressed>())  return EVT_MOUSE_BUTTON_PRESSED;
            if (e.is<sf::Event::MouseButtonReleased>()) return EVT_MOUSE_BUTTON_RELEASED;
            if (e.is<sf::Event::MouseWheelScrolled>())  return EVT_MOUSE_WHEEL_SCROLLED;
            if (e.is<sf::Event::TextEntered>())         return EVT_TEXT_ENTERED;
            if (e.is<sf::Event::FocusGained>())         return EVT_FOCUS_GAINED;
            if (e.is<sf::Event::FocusLost>())           return EVT_FOCUS_LOST;
            return EVT_NONE;
        }

        /* ---- RenderWindow lifecycle ---- */

        MTypeValue* nCreateWindow(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_create_window")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* title = getStr(args[0]);
            unsigned w = static_cast<unsigned>(g_host->getInt(args[1]));
            unsigned h = static_cast<unsigned>(g_host->getInt(args[2]));
            auto* win = new sf::RenderWindow(sf::VideoMode({w, h}), title);
            return g_host->makeInt(ctx, g_windows.insert(win));
        }
        MTypeValue* nDestroyWindow(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_destroy_window")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.erase(g_host->getInt(args[0]));
            delete w;
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nIsOpen(void*, MTypeContext* ctx,
                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_is_open")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, (w && w->isOpen()) ? 1 : 0);
        }
        MTypeValue* nClose(void*, MTypeContext* ctx,
                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_close")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->close();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nClear(void*, MTypeContext* ctx,
                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sfml_window_clear")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return g_host->makeVoid(ctx);
            sf::Color c(
                static_cast<std::uint8_t>(g_host->getInt(args[1])),
                static_cast<std::uint8_t>(g_host->getInt(args[2])),
                static_cast<std::uint8_t>(g_host->getInt(args[3])),
                static_cast<std::uint8_t>(g_host->getInt(args[4])));
            w->clear(c);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nDisplay(void*, MTypeContext* ctx,
                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_display")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->display();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSize(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_size")) {
                return g_host->makeNull(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            unsigned ww = 0, hh = 0;
            if (w) {
                auto s = w->getSize();
                ww = s.x; hh = s.y;
            }
            MTypeValue* out = g_host->makeArray(ctx, MT_TAG_INT, 2);
            g_host->arraySet(out, 0, g_host->makeInt(ctx, ww));
            g_host->arraySet(out, 1, g_host->makeInt(ctx, hh));
            return out;
        }

        /* ---- pollEvent + tag/payload accessors ---- */

        MTypeValue* nPollEvent(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_poll_event")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) {
                g_lastEvent.reset();
                g_lastTag = EVT_NONE;
                return g_host->makeInt(ctx, 0);
            }
            auto ev = w->pollEvent();
            if (!ev) {
                g_lastEvent.reset();
                g_lastTag = EVT_NONE;
                return g_host->makeInt(ctx, 0);
            }
            g_lastEvent = *ev;
            g_lastTag = classify(*ev);
            return g_host->makeInt(ctx, static_cast<int64_t>(g_lastTag));
        }

        /* Event-tag-id constants. The values are the stable EventTag enum
         * — mType compares the int returned by pollEvent against these. */
        #define DEF_EVT_ID(NAME, TAG) \
            MTypeValue* nEvt_##NAME(void*, MTypeContext* ctx, \
                                     const MTypeValue* const*, int) { \
                return g_host->makeInt(ctx, static_cast<int64_t>(TAG)); \
            }
        DEF_EVT_ID(Closed,               EVT_CLOSED)
        DEF_EVT_ID(Resized,              EVT_RESIZED)
        DEF_EVT_ID(KeyPressed,           EVT_KEY_PRESSED)
        DEF_EVT_ID(KeyReleased,          EVT_KEY_RELEASED)
        DEF_EVT_ID(MouseMoved,           EVT_MOUSE_MOVED)
        DEF_EVT_ID(MouseButtonPressed,   EVT_MOUSE_BUTTON_PRESSED)
        DEF_EVT_ID(MouseButtonReleased,  EVT_MOUSE_BUTTON_RELEASED)
        DEF_EVT_ID(MouseWheelScrolled,   EVT_MOUSE_WHEEL_SCROLLED)
        DEF_EVT_ID(TextEntered,          EVT_TEXT_ENTERED)
        DEF_EVT_ID(FocusGained,          EVT_FOCUS_GAINED)
        DEF_EVT_ID(FocusLost,            EVT_FOCUS_LOST)
        #undef DEF_EVT_ID

        /* Payload accessors. Return 0 / "" when the last event isn't of
         * the matching type — caller should gate on the event-tag first. */

        MTypeValue* nEvtMouseX(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (!g_lastEvent) return g_host->makeInt(ctx, 0);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseMoved>())          return g_host->makeInt(ctx, e->position.x);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseButtonPressed>())  return g_host->makeInt(ctx, e->position.x);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseButtonReleased>()) return g_host->makeInt(ctx, e->position.x);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseWheelScrolled>())  return g_host->makeInt(ctx, e->position.x);
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nEvtMouseY(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (!g_lastEvent) return g_host->makeInt(ctx, 0);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseMoved>())          return g_host->makeInt(ctx, e->position.y);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseButtonPressed>())  return g_host->makeInt(ctx, e->position.y);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseButtonReleased>()) return g_host->makeInt(ctx, e->position.y);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseWheelScrolled>())  return g_host->makeInt(ctx, e->position.y);
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nEvtMouseButton(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (!g_lastEvent) return g_host->makeInt(ctx, 0);
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseButtonPressed>())
                return g_host->makeInt(ctx, static_cast<int64_t>(e->button));
            if (auto* e = g_lastEvent->getIf<sf::Event::MouseButtonReleased>())
                return g_host->makeInt(ctx, static_cast<int64_t>(e->button));
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nEvtWheelDelta(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (g_lastEvent) {
                if (auto* e = g_lastEvent->getIf<sf::Event::MouseWheelScrolled>())
                    return g_host->makeFloat(ctx, e->delta);
            }
            return g_host->makeFloat(ctx, 0.0);
        }
        MTypeValue* nEvtKey(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (!g_lastEvent) return g_host->makeInt(ctx, 0);
            if (auto* e = g_lastEvent->getIf<sf::Event::KeyPressed>())
                return g_host->makeInt(ctx, static_cast<int64_t>(e->code));
            if (auto* e = g_lastEvent->getIf<sf::Event::KeyReleased>())
                return g_host->makeInt(ctx, static_cast<int64_t>(e->code));
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nEvtResizeWidth(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (g_lastEvent) {
                if (auto* e = g_lastEvent->getIf<sf::Event::Resized>())
                    return g_host->makeInt(ctx, e->size.x);
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nEvtResizeHeight(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (g_lastEvent) {
                if (auto* e = g_lastEvent->getIf<sf::Event::Resized>())
                    return g_host->makeInt(ctx, e->size.y);
            }
            return g_host->makeInt(ctx, 0);
        }
        MTypeValue* nEvtTextUnicode(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            if (g_lastEvent) {
                if (auto* e = g_lastEvent->getIf<sf::Event::TextEntered>())
                    return g_host->makeInt(ctx, static_cast<int64_t>(e->unicode));
            }
            return g_host->makeInt(ctx, 0);
        }

        /* ---- realtime input (no event needed) ---- */

        MTypeValue* nKeyIsDown(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_key_is_down")) {
                return g_host->makeBool(ctx, 0);
            }
            int key = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx,
                sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(key)) ? 1 : 0);
        }
        MTypeValue* nMouseButtonIsDown(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_mouse_button_is_down")) {
                return g_host->makeBool(ctx, 0);
            }
            int btn = static_cast<int>(g_host->getInt(args[0]));
            return g_host->makeBool(ctx,
                sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(btn)) ? 1 : 0);
        }

        /* ---- Window controls (title / size / vsync / cursor / focus) ---- */

        MTypeValue* nWindowSetTitle(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_title")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return g_host->makeVoid(ctx);
            std::size_t len = 0;
            const char* utf8 = getStr(args[1], &len);
            w->setTitle(sf::String::fromUtf8(utf8, utf8 + len));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetPosition(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_window_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setPosition({static_cast<int>(g_host->getInt(args[1])),
                                    static_cast<int>(g_host->getInt(args[2]))});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetSize(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_window_set_size")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setSize({detail::getU(args[1]), detail::getU(args[2])});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetVsync(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_vsync")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setVerticalSyncEnabled(detail::getB(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetFramerateLimit(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_framerate_limit")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setFramerateLimit(detail::getU(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetKeyRepeat(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_key_repeat")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setKeyRepeatEnabled(detail::getB(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetMouseCursorVisible(void*, MTypeContext* ctx,
                                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_mouse_cursor_visible")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setMouseCursorVisible(detail::getB(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetMouseCursorGrabbed(void*, MTypeContext* ctx,
                                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_mouse_cursor_grabbed")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->setMouseCursorGrabbed(detail::getB(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowHasFocus(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_has_focus")) {
                return g_host->makeBool(ctx, 0);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            return g_host->makeBool(ctx, (w && w->hasFocus()) ? 1 : 0);
        }
        MTypeValue* nWindowRequestFocus(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_window_request_focus")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) w->requestFocus();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetMousePosition(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_window_set_mouse_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (w) sf::Mouse::setPosition({static_cast<int>(g_host->getInt(args[1])),
                                            static_cast<int>(g_host->getInt(args[2]))}, *w);
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMousePositionWindow(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_mouse_position_window")) {
                return detail::makeIntPair(ctx, 0, 0);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            if (!w) return detail::makeIntPair(ctx, 0, 0);
            auto p = sf::Mouse::getPosition(*w);
            return detail::makeIntPair(ctx, p.x, p.y);
        }

        /* ---- Cursor (system-defined cursor shapes) ---- */

        MTypeValue* nCursorCreateFromSystem(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_cursor_create_from_system")) {
                return g_host->makeInt(ctx, 0);
            }
            auto type = static_cast<sf::Cursor::Type>(g_host->getInt(args[0]));
            auto opt = sf::Cursor::createFromSystem(type);
            if (!opt) {
                g_host->raiseError(ctx, kEx,
                    "__native__sfml_cursor_create_from_system: unsupported cursor type on this platform");
                return g_host->makeInt(ctx, 0);
            }
            auto* c = new sf::Cursor(std::move(*opt));
            return g_host->makeInt(ctx, g_cursors.insert(c));
        }
        MTypeValue* nCursorDestroy(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_cursor_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_cursors.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nWindowSetMouseCursor(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_window_set_mouse_cursor")) {
                return g_host->makeVoid(ctx);
            }
            sf::RenderWindow* w = g_windows.find(g_host->getInt(args[0]));
            sf::Cursor* c = g_cursors.find(g_host->getInt(args[1]));
            if (w && c) w->setMouseCursor(*c);
            return g_host->makeVoid(ctx);
        }

        /* ---- Joystick (no handle, all static) ---- */

        MTypeValue* nJoystickUpdate(void*, MTypeContext* ctx,
                                       const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sfml_joystick_update")) {
                return g_host->makeVoid(ctx);
            }
            sf::Joystick::update();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nJoystickIsConnected(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_joystick_is_connected")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx,
                sf::Joystick::isConnected(detail::getU(args[0])) ? 1 : 0);
        }
        MTypeValue* nJoystickButtonCount(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_joystick_button_count")) {
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx,
                static_cast<std::int64_t>(sf::Joystick::getButtonCount(detail::getU(args[0]))));
        }
        MTypeValue* nJoystickAxisIsAvailable(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_joystick_axis_is_available")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx,
                sf::Joystick::hasAxis(detail::getU(args[0]),
                                       static_cast<sf::Joystick::Axis>(g_host->getInt(args[1]))) ? 1 : 0);
        }
        MTypeValue* nJoystickIsButtonPressed(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_joystick_is_button_pressed")) {
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx,
                sf::Joystick::isButtonPressed(detail::getU(args[0]),
                                                detail::getU(args[1])) ? 1 : 0);
        }
        MTypeValue* nJoystickAxisPosition(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_joystick_axis_position")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            return g_host->makeFloat(ctx,
                sf::Joystick::getAxisPosition(detail::getU(args[0]),
                                                static_cast<sf::Joystick::Axis>(g_host->getInt(args[1]))));
        }
    }

    /* Bridge for ImGui: forward the most-recently-polled SFML event to
     * ImGui::SFML::ProcessEvent. Called from ImGuiBindings.cpp via the
     * declaration in PluginGlobals.hpp so we don't have to expose
     * g_lastEvent across translation units. */
    void imguiProcessLastEvent(sf::RenderWindow& win)
    {
        if (g_lastEvent) {
            ImGui::SFML::ProcessEvent(win, *g_lastEvent);
        }
    }

    void registerWindowNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };

        reg("__native__sfml_create_window",     &nCreateWindow);
        reg("__native__sfml_destroy_window",    &nDestroyWindow);
        reg("__native__sfml_window_is_open",    &nIsOpen);
        reg("__native__sfml_window_close",      &nClose);
        reg("__native__sfml_window_clear",      &nClear);
        reg("__native__sfml_window_display",    &nDisplay);
        reg("__native__sfml_window_size",       &nWindowSize);

        reg("__native__sfml_poll_event",        &nPollEvent);

        reg("__native__sfml_evt_closed_id",                  &nEvt_Closed);
        reg("__native__sfml_evt_resized_id",                 &nEvt_Resized);
        reg("__native__sfml_evt_key_pressed_id",             &nEvt_KeyPressed);
        reg("__native__sfml_evt_key_released_id",            &nEvt_KeyReleased);
        reg("__native__sfml_evt_mouse_moved_id",             &nEvt_MouseMoved);
        reg("__native__sfml_evt_mouse_button_pressed_id",    &nEvt_MouseButtonPressed);
        reg("__native__sfml_evt_mouse_button_released_id",   &nEvt_MouseButtonReleased);
        reg("__native__sfml_evt_mouse_wheel_scrolled_id",    &nEvt_MouseWheelScrolled);
        reg("__native__sfml_evt_text_entered_id",            &nEvt_TextEntered);
        reg("__native__sfml_evt_focus_gained_id",            &nEvt_FocusGained);
        reg("__native__sfml_evt_focus_lost_id",              &nEvt_FocusLost);

        reg("__native__sfml_evt_mouse_x",        &nEvtMouseX);
        reg("__native__sfml_evt_mouse_y",        &nEvtMouseY);
        reg("__native__sfml_evt_mouse_button",   &nEvtMouseButton);
        reg("__native__sfml_evt_wheel_delta",    &nEvtWheelDelta);
        reg("__native__sfml_evt_key",            &nEvtKey);
        reg("__native__sfml_evt_resize_width",   &nEvtResizeWidth);
        reg("__native__sfml_evt_resize_height",  &nEvtResizeHeight);
        reg("__native__sfml_evt_text_unicode",   &nEvtTextUnicode);

        reg("__native__sfml_key_is_down",          &nKeyIsDown);
        reg("__native__sfml_mouse_button_is_down", &nMouseButtonIsDown);

        /* Window controls (Phase 3 — settings, focus, mouse helpers). */
        reg("__native__sfml_window_set_title",                 &nWindowSetTitle);
        reg("__native__sfml_window_set_position",              &nWindowSetPosition);
        reg("__native__sfml_window_set_size",                  &nWindowSetSize);
        reg("__native__sfml_window_set_vsync",                 &nWindowSetVsync);
        reg("__native__sfml_window_set_framerate_limit",       &nWindowSetFramerateLimit);
        reg("__native__sfml_window_set_key_repeat",            &nWindowSetKeyRepeat);
        reg("__native__sfml_window_set_mouse_cursor_visible",  &nWindowSetMouseCursorVisible);
        reg("__native__sfml_window_set_mouse_cursor_grabbed",  &nWindowSetMouseCursorGrabbed);
        reg("__native__sfml_window_has_focus",                 &nWindowHasFocus);
        reg("__native__sfml_window_request_focus",             &nWindowRequestFocus);
        reg("__native__sfml_window_set_mouse_position",        &nWindowSetMousePosition);
        reg("__native__sfml_mouse_position_window",            &nMousePositionWindow);

        /* Cursor handles. */
        reg("__native__sfml_cursor_create_from_system",        &nCursorCreateFromSystem);
        reg("__native__sfml_cursor_destroy",                   &nCursorDestroy);
        reg("__native__sfml_window_set_mouse_cursor",          &nWindowSetMouseCursor);

        /* Joystick (no handle). */
        reg("__native__sfml_joystick_update",                  &nJoystickUpdate);
        reg("__native__sfml_joystick_is_connected",            &nJoystickIsConnected);
        reg("__native__sfml_joystick_button_count",            &nJoystickButtonCount);
        reg("__native__sfml_joystick_axis_is_available",       &nJoystickAxisIsAvailable);
        reg("__native__sfml_joystick_is_button_pressed",       &nJoystickIsButtonPressed);
        reg("__native__sfml_joystick_axis_position",           &nJoystickAxisPosition);
    }
}
