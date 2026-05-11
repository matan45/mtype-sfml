// SFML 3 audio demo: plays a short Sound effect on Space, streams
// background Music, exposes volume + pitch sliders via ImGui.
//
// Assets are vendored under vendor/SFML/examples/sound/resources/.
// Run from the project root so the relative paths resolve.

import * from "../lib/Sfml.mt";
import * from "../lib/Graphics.mt";
import * from "../lib/Audio.mt";
import * from "../lib/ImGui.mt";
import * from "../lib/System.mt";

__plugin_load("mt/mtype_sfml.dll");

RenderWindow window = Sfml::createWindow("SFML 3 Audio Demo", 720, 360);
ImGui::init(window);

// Short-clip sound. SoundBuffer holds the PCM data; the Sound is a cheap
// playback handle on top.
SoundBuffer ballBuf = SoundBuffers::load("vendor/SFML/examples/tennis/resources/ball.wav");
Sound ping = Sounds::create(ballBuf);

// Streaming music. Music decodes from disk on the fly — don't destroy
// it while it's playing.
Music music = MusicPlayer::open("vendor/SFML/examples/sound/resources/doodle_pop.ogg");
music.setLoop(true);
music.setVolume(40.0);
music.play();

// Pre-cache event constants + keys.
int evClosed     = Sfml::closedEventId();
int evKeyPressed = Sfml::keyPressedEventId();
int keyEsc   = Key::escape();
int keySpace = Key::space();

float sfxVol   = 100.0;
float sfxPitch = 1.0;
float musicVol = 40.0;
bool  musicOn  = true;

Clock frame = Clocks::create();

while (window.isOpen()) {
    int ev = window.pollEvent();
    while (ev != 0) {
        ImGui::processSfmlEvent(window);
        if (ev == evClosed) {
            window.close();
        } else if (ev == evKeyPressed) {
            int k = Event::key();
            if (k == keyEsc)   { window.close(); }
            if (k == keySpace) { ping.play(); }
        }
        ev = window.pollEvent();
    }

    float dtMs = frame.restartSeconds() * 1000.0;
    ImGui::update(window, dtMs);

    if (ImGui::begin("Audio Controls")) {
        ImGui::text("Press SPACE to play the sfx.");
        ImGui::separator();

        sfxVol = ImGui::sliderFloat("Sfx volume", sfxVol, 0.0, 100.0);
        ping.setVolume(sfxVol);

        sfxPitch = ImGui::sliderFloat("Sfx pitch", sfxPitch, 0.25, 3.0);
        ping.setPitch(sfxPitch);

        ImGui::separator();
        musicVol = ImGui::sliderFloat("Music volume", musicVol, 0.0, 100.0);
        music.setVolume(musicVol);

        musicOn = ImGui::checkbox("Music playing", musicOn);
        if (musicOn) {
            if (music.getStatus() != SoundStatus::playing()) { music.play(); }
        } else {
            if (music.getStatus() == SoundStatus::playing()) { music.pause(); }
        }

        ImGui::text("Music offset: " + music.getPlayingOffsetSeconds() + "s / " +
                    music.durationSeconds() + "s");
    }
    ImGui::end();

    window.clear(20, 22, 32, 255);
    ImGui::render(window);
    window.display();
}

music.destroy();
ping.destroy();
ballBuf.destroy();
frame.destroy();
ImGui::shutdown();
window.destroy();
__plugin_unload("mt/mtype_sfml.dll");
