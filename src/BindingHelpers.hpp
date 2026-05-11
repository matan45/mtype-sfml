#pragma once
/*
 * Helpers shared by the binding TUs. Each .cpp picks its own exception-type
 * string ("SfmlError" / "GraphicsError") via the `kEx` constant in its
 * anonymous namespace so call sites stay terse:
 *
 *   if (!requireArgs(ctx, argc, 3, "__native__foo", kEx)) return ...;
 *   sf::RenderWindow* w = findOrRaise(g_windows, g_host->getInt(args[0]),
 *                                     ctx, "__native__foo", kEx);
 *   if (!w) return ...;
 *
 * The newer helpers (Registrar, getF/getU/getU8/getB, getColor,
 * makeIntPair / makeFloatPair / makeFloatQuad) are used by the System,
 * Audio, and other recent bindings to compress the boilerplate. The
 * older WindowBindings/GraphicsBindings sources still spell out the
 * patterns inline — keep both working.
 */

#include "PluginGlobals.hpp"
#include "HandleRegistry.hpp"

#include <SFML/Graphics/Color.hpp>

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

    /* Scalar extraction shorthands. The host vtable exposes ints as int64
     * and floats as double; binding code almost always wants the narrower
     * SFML-side types (float, unsigned, std::uint8_t for color channels). */
    inline float        getF (const MTypeValue* v) { return static_cast<float>(g_host->getFloat(v)); }
    inline unsigned     getU (const MTypeValue* v) { return static_cast<unsigned>(g_host->getInt(v)); }
    inline std::uint8_t getU8(const MTypeValue* v) { return static_cast<std::uint8_t>(g_host->getInt(v)); }
    inline bool         getB (const MTypeValue* v) { return g_host->getBool(v) != 0; }

    /* Read four consecutive (r,g,b,a) int args starting at `off`. */
    inline sf::Color getColor(const MTypeValue* const* args, int off)
    {
        return sf::Color(getU8(args[off]),
                          getU8(args[off + 1]),
                          getU8(args[off + 2]),
                          getU8(args[off + 3]));
    }

    /* Result builders for the two common "array return" shapes. */
    inline MTypeValue* makeIntPair(MTypeContext* ctx, std::int64_t a, std::int64_t b)
    {
        MTypeValue* out = g_host->makeArray(ctx, MT_TAG_INT, 2);
        g_host->arraySet(out, 0, g_host->makeInt(ctx, a));
        g_host->arraySet(out, 1, g_host->makeInt(ctx, b));
        return out;
    }
    inline MTypeValue* makeFloatPair(MTypeContext* ctx, double x, double y)
    {
        MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 2);
        g_host->arraySet(out, 0, g_host->makeFloat(ctx, x));
        g_host->arraySet(out, 1, g_host->makeFloat(ctx, y));
        return out;
    }
    inline MTypeValue* makeFloatQuad(MTypeContext* ctx,
                                       double x, double y, double z, double w)
    {
        MTypeValue* out = g_host->makeArray(ctx, MT_TAG_FLOAT, 4);
        g_host->arraySet(out, 0, g_host->makeFloat(ctx, x));
        g_host->arraySet(out, 1, g_host->makeFloat(ctx, y));
        g_host->arraySet(out, 2, g_host->makeFloat(ctx, z));
        g_host->arraySet(out, 3, g_host->makeFloat(ctx, w));
        return out;
    }
    inline MTypeValue* makeIntQuad(MTypeContext* ctx,
                                     std::int64_t a, std::int64_t b,
                                     std::int64_t c, std::int64_t d)
    {
        MTypeValue* out = g_host->makeArray(ctx, MT_TAG_INT, 4);
        g_host->arraySet(out, 0, g_host->makeInt(ctx, a));
        g_host->arraySet(out, 1, g_host->makeInt(ctx, b));
        g_host->arraySet(out, 2, g_host->makeInt(ctx, c));
        g_host->arraySet(out, 3, g_host->makeInt(ctx, d));
        return out;
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

    /* Batched name-prefix registrar. Collapses long `reg("__native__sfml_xxx", &fn)`
     * tables into chained calls:
     *
     *   Registrar r{ctx, "__native__sfml_"};
     *   r("create_window", &nCreateWindow)
     *    ("destroy_window", &nDestroyWindow);
     */
    struct Registrar
    {
        MTypeContext* ctx;
        const char*   prefix;

        Registrar& operator()(const char* suffix, MTypeNativeFn fn)
        {
            std::string name = std::string(prefix) + suffix;
            g_host->registerFunction(ctx, name.c_str(), fn, nullptr);
            return *this;
        }
    };
}
