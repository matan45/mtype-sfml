#pragma once
/*
 * Helpers shared by WindowBindings.cpp and GraphicsBindings.cpp. Each
 * .cpp picks its own exception-type string ("SfmlError" / "GraphicsError")
 * via the `kEx` constant in its anonymous namespace so call sites stay terse:
 *
 *   if (!requireArgs(ctx, argc, 3, "__native__foo", kEx)) return ...;
 *   sf::RenderWindow* w = findOrRaise(g_windows, g_host->getInt(args[0]),
 *                                     ctx, "__native__foo", kEx);
 *   if (!w) return ...;
 */

#include "PluginGlobals.hpp"
#include "HandleRegistry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace mtypesfml::detail
{
    inline bool requireArgs(MTypeContext* ctx, int argc, int expected,
                            const char* name, const char* exType)
    {
        if (argc != expected) {
            std::string m = std::string(name) + ": expected " + std::to_string(expected)
                          + " args, got " + std::to_string(argc);
            g_host->raiseError(ctx, exType, m.c_str());
            return false;
        }
        return true;
    }

    inline const char* getStr(const MTypeValue* v, std::size_t* outLen = nullptr)
    {
        if (g_host->getTag(v) != MT_TAG_STRING) {
            if (outLen) *outLen = 0;
            return "";
        }
        return g_host->getString(v, outLen);
    }

    /* Look up `id` in `reg`. On miss, raise an error like
     * "<op>: invalid handle id <n>" and return nullptr; the caller should
     * return promptly. */
    template <typename T>
    T* findOrRaise(HandleRegistry<T>& reg, std::int64_t id,
                   MTypeContext* ctx, const char* op, const char* exType)
    {
        T* p = reg.find(id);
        if (!p) {
            std::string m = std::string(op) + ": invalid handle id "
                          + std::to_string(id);
            g_host->raiseError(ctx, exType, m.c_str());
        }
        return p;
    }
}
