/*
 * SFML 3 Audio bindings — SoundBuffer / Sound / Music / Listener.
 *
 * Lifetime: sf::Sound holds a non-owning reference to the SoundBuffer it
 * was constructed from. Scripts must keep the SoundBuffer alive (don't
 * destroy it) until every Sound built on top is also gone. Same contract
 * as Sprite/Texture. Music owns its own streaming data so it has no such
 * dependency.
 *
 * SFML 3 renamed setLoop/getLoop to setLooping/isLooping on SoundSource;
 * Sound + Music both inherit. Times are flattened to float-seconds at the
 * boundary (matches SystemBindings).
 */

#include "PluginGlobals.hpp"
#include "BindingHelpers.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Listener.hpp>
#include <SFML/System/Time.hpp>

#include <string>

namespace mtypesfml
{
    namespace
    {
        constexpr const char* kEx = "AudioError";

        inline bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            return detail::requireArgs(ctx, argc, expected, name, kEx);
        }
        inline const char* getStr(const MTypeValue* v, size_t* outLen = nullptr)
        {
            return detail::getStr(v, outLen);
        }

        /* SFML 3 SoundSource::Status is an enum class. Flatten to int for the
         * script side; values match the SFML enum order (Stopped=0, Paused=1,
         * Playing=2). */
        inline int statusToInt(sf::SoundSource::Status s) {
            return static_cast<int>(s);
        }

        /* ---- SoundBuffer ---- */

        MTypeValue* nSoundBufferLoadFromFile(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_buffer_load_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            auto* buf = new sf::SoundBuffer();
            if (!buf->loadFromFile(path)) {
                delete buf;
                std::string m = std::string("__native__sfml_sound_buffer_load_from_file: failed for '")
                              + path + "'";
                g_host->raiseError(ctx, kEx, m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_soundBuffers.insert(buf));
        }
        MTypeValue* nSoundBufferDestroy(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_buffer_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_soundBuffers.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundBufferDuration(void*, MTypeContext* ctx,
                                           const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_buffer_duration_seconds")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            sf::SoundBuffer* b = g_soundBuffers.find(g_host->getInt(args[0]));
            if (!b) return g_host->makeFloat(ctx, 0.0);
            return g_host->makeFloat(ctx, b->getDuration().asSeconds());
        }
        MTypeValue* nSoundBufferSampleRate(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_buffer_sample_rate")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::SoundBuffer* b = g_soundBuffers.find(g_host->getInt(args[0]));
            return g_host->makeInt(ctx, b ? static_cast<std::int64_t>(b->getSampleRate()) : 0);
        }
        MTypeValue* nSoundBufferChannelCount(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_buffer_channel_count")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::SoundBuffer* b = g_soundBuffers.find(g_host->getInt(args[0]));
            return g_host->makeInt(ctx, b ? static_cast<std::int64_t>(b->getChannelCount()) : 0);
        }

        /* ---- Sound (bound to a SoundBuffer at creation) ---- */

        MTypeValue* nSoundCreate(void*, MTypeContext* ctx,
                                   const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_create")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::SoundBuffer* b = g_soundBuffers.find(g_host->getInt(args[0]));
            if (!b) {
                g_host->raiseError(ctx, kEx, "__native__sfml_sound_create: invalid SoundBuffer id");
                return g_host->makeInt(ctx, 0);
            }
            auto* s = new sf::Sound(*b);
            return g_host->makeInt(ctx, g_sounds.insert(s));
        }
        MTypeValue* nSoundDestroy(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_sounds.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundPlay(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_play")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->play();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundPause(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_pause")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->pause();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundStop(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_stop")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->stop();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundSetVolume(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_sound_set_volume")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->setVolume(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundSetPitch(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_sound_set_pitch")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->setPitch(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundSetLoop(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_sound_set_loop")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->setLooping(detail::getB(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundSetPosition(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 4, "__native__sfml_sound_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->setPosition({detail::getF(args[1]),
                                    detail::getF(args[2]),
                                    detail::getF(args[3])});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nSoundGetStatus(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_get_status")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            return g_host->makeInt(ctx, s ? statusToInt(s->getStatus()) : 0);
        }
        MTypeValue* nSoundGetPlayingOffset(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_sound_get_playing_offset_seconds")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (!s) return g_host->makeFloat(ctx, 0.0);
            return g_host->makeFloat(ctx, s->getPlayingOffset().asSeconds());
        }
        MTypeValue* nSoundSetPlayingOffset(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_sound_set_playing_offset_seconds")) {
                return g_host->makeVoid(ctx);
            }
            sf::Sound* s = g_sounds.find(g_host->getInt(args[0]));
            if (s) s->setPlayingOffset(sf::seconds(detail::getF(args[1])));
            return g_host->makeVoid(ctx);
        }

        /* ---- Music (streams from file; owns its data) ---- */

        MTypeValue* nMusicOpenFromFile(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_open_from_file")) {
                return g_host->makeInt(ctx, 0);
            }
            const char* path = getStr(args[0]);
            auto* m = new sf::Music();
            if (!m->openFromFile(path)) {
                delete m;
                std::string msg = std::string("__native__sfml_music_open_from_file: failed for '")
                                + path + "'";
                g_host->raiseError(ctx, kEx, msg.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_music.insert(m));
        }
        MTypeValue* nMusicDestroy(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_destroy")) {
                return g_host->makeVoid(ctx);
            }
            delete g_music.erase(g_host->getInt(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicPlay(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_play")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->play();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicPause(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_pause")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->pause();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicStop(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_stop")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->stop();
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicSetVolume(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_music_set_volume")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->setVolume(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicSetPitch(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_music_set_pitch")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->setPitch(detail::getF(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicSetLoop(void*, MTypeContext* ctx,
                                    const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_music_set_loop")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->setLooping(detail::getB(args[1]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicGetDuration(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_get_duration_seconds")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (!m) return g_host->makeFloat(ctx, 0.0);
            return g_host->makeFloat(ctx, m->getDuration().asSeconds());
        }
        MTypeValue* nMusicGetPlayingOffset(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_get_playing_offset_seconds")) {
                return g_host->makeFloat(ctx, 0.0);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (!m) return g_host->makeFloat(ctx, 0.0);
            return g_host->makeFloat(ctx, m->getPlayingOffset().asSeconds());
        }
        MTypeValue* nMusicSetPlayingOffset(void*, MTypeContext* ctx,
                                              const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__sfml_music_set_playing_offset_seconds")) {
                return g_host->makeVoid(ctx);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            if (m) m->setPlayingOffset(sf::seconds(detail::getF(args[1])));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nMusicGetStatus(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_music_get_status")) {
                return g_host->makeInt(ctx, 0);
            }
            sf::Music* m = g_music.find(g_host->getInt(args[0]));
            return g_host->makeInt(ctx, m ? statusToInt(m->getStatus()) : 0);
        }

        /* ---- Listener (global, no handle) ---- */

        MTypeValue* nListenerSetGlobalVolume(void*, MTypeContext* ctx,
                                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sfml_listener_set_global_volume")) {
                return g_host->makeVoid(ctx);
            }
            sf::Listener::setGlobalVolume(detail::getF(args[0]));
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nListenerSetPosition(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_listener_set_position")) {
                return g_host->makeVoid(ctx);
            }
            sf::Listener::setPosition({detail::getF(args[0]),
                                        detail::getF(args[1]),
                                        detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
        MTypeValue* nListenerSetDirection(void*, MTypeContext* ctx,
                                             const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sfml_listener_set_direction")) {
                return g_host->makeVoid(ctx);
            }
            sf::Listener::setDirection({detail::getF(args[0]),
                                         detail::getF(args[1]),
                                         detail::getF(args[2])});
            return g_host->makeVoid(ctx);
        }
    }

    void registerAudioNatives(MTypeContext* ctx)
    {
        detail::Registrar r{ctx, "__native__sfml_"};

        /* SoundBuffer */
        r("sound_buffer_load_from_file",     &nSoundBufferLoadFromFile)
         ("sound_buffer_destroy",            &nSoundBufferDestroy)
         ("sound_buffer_duration_seconds",   &nSoundBufferDuration)
         ("sound_buffer_sample_rate",        &nSoundBufferSampleRate)
         ("sound_buffer_channel_count",      &nSoundBufferChannelCount);

        /* Sound */
        r("sound_create",                    &nSoundCreate)
         ("sound_destroy",                   &nSoundDestroy)
         ("sound_play",                      &nSoundPlay)
         ("sound_pause",                     &nSoundPause)
         ("sound_stop",                      &nSoundStop)
         ("sound_set_volume",                &nSoundSetVolume)
         ("sound_set_pitch",                 &nSoundSetPitch)
         ("sound_set_loop",                  &nSoundSetLoop)
         ("sound_set_position",              &nSoundSetPosition)
         ("sound_get_status",                &nSoundGetStatus)
         ("sound_get_playing_offset_seconds",&nSoundGetPlayingOffset)
         ("sound_set_playing_offset_seconds",&nSoundSetPlayingOffset);

        /* Music */
        r("music_open_from_file",            &nMusicOpenFromFile)
         ("music_destroy",                   &nMusicDestroy)
         ("music_play",                      &nMusicPlay)
         ("music_pause",                     &nMusicPause)
         ("music_stop",                      &nMusicStop)
         ("music_set_volume",                &nMusicSetVolume)
         ("music_set_pitch",                 &nMusicSetPitch)
         ("music_set_loop",                  &nMusicSetLoop)
         ("music_get_duration_seconds",      &nMusicGetDuration)
         ("music_get_playing_offset_seconds",&nMusicGetPlayingOffset)
         ("music_set_playing_offset_seconds",&nMusicSetPlayingOffset)
         ("music_get_status",                &nMusicGetStatus);

        /* Listener */
        r("listener_set_global_volume",      &nListenerSetGlobalVolume)
         ("listener_set_position",           &nListenerSetPosition)
         ("listener_set_direction",          &nListenerSetDirection);
    }
}
