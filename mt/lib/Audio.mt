// mType wrapper around SFML 3's Audio module — SoundBuffer / Sound /
// Music / Listener.
//
// Lifetime: a Sound holds a non-owning reference to the SoundBuffer it
// was created from. Keep the SoundBuffer alive until every Sound built
// on top is destroyed. Music streams from disk and owns its own data —
// no such dependency.
//
// SoundStatus values match sf::SoundSource::Status (Stopped=0,
// Paused=1, Playing=2).

class SoundStatus {
    public static function stopped(): int { return 0; }
    public static function paused():  int { return 1; }
    public static function playing(): int { return 2; }
}

class SoundBuffer {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_sound_buffer_destroy(this.handle); }

    public function durationSeconds(): float {
        return __native__sfml_sound_buffer_duration_seconds(this.handle);
    }
    public function sampleRate(): int {
        return __native__sfml_sound_buffer_sample_rate(this.handle);
    }
    public function channelCount(): int {
        return __native__sfml_sound_buffer_channel_count(this.handle);
    }
}

class SoundBuffers {
    public static function load(string path): SoundBuffer {
        return new SoundBuffer(__native__sfml_sound_buffer_load_from_file(path));
    }
}

class Sound {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_sound_destroy(this.handle); }

    public function play():  void { __native__sfml_sound_play(this.handle); }
    public function pause(): void { __native__sfml_sound_pause(this.handle); }
    public function stop():  void { __native__sfml_sound_stop(this.handle); }

    // Volume in [0, 100].
    public function setVolume(float volume): void {
        __native__sfml_sound_set_volume(this.handle, volume);
    }
    // Pitch multiplier. 1.0 = original pitch.
    public function setPitch(float pitch): void {
        __native__sfml_sound_set_pitch(this.handle, pitch);
    }
    public function setLoop(bool loop): void {
        __native__sfml_sound_set_loop(this.handle, loop);
    }
    // 3D source position for spatial audio.
    public function setPosition(float x, float y, float z): void {
        __native__sfml_sound_set_position(this.handle, x, y, z);
    }

    public function getStatus(): int {
        return __native__sfml_sound_get_status(this.handle);
    }
    public function getPlayingOffsetSeconds(): float {
        return __native__sfml_sound_get_playing_offset_seconds(this.handle);
    }
    public function setPlayingOffsetSeconds(float seconds): void {
        __native__sfml_sound_set_playing_offset_seconds(this.handle, seconds);
    }
}

class Sounds {
    public static function create(SoundBuffer buf): Sound {
        return new Sound(__native__sfml_sound_create(buf.handle));
    }
}

class Music {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_music_destroy(this.handle); }

    public function play():  void { __native__sfml_music_play(this.handle); }
    public function pause(): void { __native__sfml_music_pause(this.handle); }
    public function stop():  void { __native__sfml_music_stop(this.handle); }

    public function setVolume(float volume): void {
        __native__sfml_music_set_volume(this.handle, volume);
    }
    public function setPitch(float pitch): void {
        __native__sfml_music_set_pitch(this.handle, pitch);
    }
    public function setLoop(bool loop): void {
        __native__sfml_music_set_loop(this.handle, loop);
    }

    public function durationSeconds(): float {
        return __native__sfml_music_get_duration_seconds(this.handle);
    }
    public function getPlayingOffsetSeconds(): float {
        return __native__sfml_music_get_playing_offset_seconds(this.handle);
    }
    public function setPlayingOffsetSeconds(float seconds): void {
        __native__sfml_music_set_playing_offset_seconds(this.handle, seconds);
    }
    public function getStatus(): int {
        return __native__sfml_music_get_status(this.handle);
    }
}

class MusicPlayer {
    public static function open(string path): Music {
        return new Music(__native__sfml_music_open_from_file(path));
    }
}

// Listener is global — affects all spatial Sounds.
class Listener {
    // Master volume in [0, 100].
    public static function setGlobalVolume(float volume): void {
        __native__sfml_listener_set_global_volume(volume);
    }
    public static function setPosition(float x, float y, float z): void {
        __native__sfml_listener_set_position(x, y, z);
    }
    public static function setDirection(float x, float y, float z): void {
        __native__sfml_listener_set_direction(x, y, z);
    }
}
