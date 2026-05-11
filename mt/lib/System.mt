// mType wrapper around SFML 3's System module. Currently exposes
// sf::Clock (frame timing) and sf::sleep. Times are flattened to
// float-seconds at the boundary — there's no separate Time handle.

class Clock {
    public int handle;
    public constructor(int h) { this.handle = h; }
    public function destroy(): void { __native__sfml_clock_destroy(this.handle); }

    // Seconds since construction (or since the last restart()).
    public function elapsedSeconds(): float {
        return __native__sfml_clock_elapsed_seconds(this.handle);
    }
    // Reset the clock and return the seconds that had accumulated. The
    // standard frame-loop pattern is `float dt = clock.restartSeconds();`.
    public function restartSeconds(): float {
        return __native__sfml_clock_restart_seconds(this.handle);
    }
}

class Clocks {
    public static function create(): Clock {
        return new Clock(__native__sfml_clock_create());
    }
}

// Free-function timing helpers.
class Time {
    public static function sleepMs(int milliseconds): void {
        __native__sfml_sleep_ms(milliseconds);
    }
}
