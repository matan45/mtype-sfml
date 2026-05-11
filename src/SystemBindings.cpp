/*
 * SFML 3 System bindings — sf::Clock + sf::sleep.
 *
 * Time itself isn't exposed as a separate handle type; methods that return
 * a sf::Time are flattened to float-seconds at the boundary. The shorthand
 * matches how scripts use it (dt in seconds for game loops).
 */

#include "PluginGlobals.hpp"
#include "BindingHelpers.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>

namespace mtypesfml
{
    namespace
    {
        constexpr const char* kEx = "SfmlError";

        inline bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            return detail::requireArgs(ctx, argc, expected, name, kEx);
        }

        MTypeValue* nClockCreate(void*, MTypeContext* ctx,
                                   const MTypeValue* const* /*args*/, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sfml_clock_create")) {
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_clocks.insert(new sf::Clock()));
        }
        MTypeValue* nClockDestroy(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_clock_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_clocks.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nClockElapsedSeconds(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_clock_elapsed_seconds")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            sf::Clock* c = g_clocks.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeFloat(ctx, 0.0);
            return g_host->makeFloat(ctx, c->getElapsedTime().asSeconds());
        }
        MTypeValue* nClockRestartSeconds(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_clock_restart_seconds")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            sf::Clock* c = g_clocks.find(g_host->getInt(args[0]));
            if (!c) return g_host->makeFloat(ctx, 0.0);
            return g_host->makeFloat(ctx, c->restart().asSeconds());
        }
        MTypeValue* nSleepMs(void*, MTypeContext* ctx,
                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sleep_ms")) {
                return g_host->makeVoid(ctx);
            }
            sf::sleep(sf::milliseconds(static_cast<std::int32_t>(g_host->getInt(args[0]))));
            return g_host->makeVoid(ctx);
        }
    }

    void registerSystemNatives(MTypeContext* ctx)
    {
        detail::Registrar r{ctx, "__native__sfml_"};
        r("clock_create",            &nClockCreate)
         ("clock_destroy",           &nClockDestroy)
         ("clock_elapsed_seconds",   &nClockElapsedSeconds)
         ("clock_restart_seconds",   &nClockRestartSeconds)
         ("sleep_ms",                &nSleepMs);
    }
}
