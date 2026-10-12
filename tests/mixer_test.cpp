// Mixer core: resampling step, mono-to-stereo, gain, loop wrap and voice expiry.
#include "audio/mixer.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace chase;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::shared_ptr<assets::Sound> sound(std::vector<std::vector<std::int16_t>> channels, int rate, bool loop,
                                     std::uint32_t loopStart = 0) {
    auto s = std::make_shared<assets::Sound>();
    s->rate = rate;
    s->loop = loop;
    s->loopStart = loopStart;
    s->frames = std::uint32_t(channels[0].size());
    s->channels = std::move(channels);
    return s;
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }
}

int main() {
    try {
        audio::Mixer mixer(8);
        // Mono one-shot at the output rate: copied to both sides, then the voice ends.
        int a = mixer.play(sound({{16384, -16384}}, 8, false), 0.5f);
        float out[8];
        mixer.mix(out, 4);
        require(near(out[0], 0.25f) && near(out[1], 0.25f) && near(out[2], -0.25f) && near(out[4], 0.0f),
                "mono one-shot mixing");
        require(!mixer.playing(a) && mixer.activeVoices() == 0, "one-shot voice not released");
        // Stereo source at half the output rate: linear interpolation between frames.
        int b = mixer.play(sound({{0, 32767, 0}, {0, -32767, 0}}, 4, false));
        mixer.mix(out, 2);
        require(near(out[0], 0.0f) && near(out[2], 0.5f) && near(out[3], -0.5f), "resampling / stereo");
        mixer.stop(b);
        // Looping voice wraps to the loop start and keeps playing.
        int c = mixer.play(sound({{1000, 2000, 3000}}, 8, true, 1));
        float loopOut[12];
        mixer.mix(loopOut, 6);
        const float expect[6] = {1000, 2000, 3000, 2000, 3000, 2000};
        for (int i = 0; i < 6; ++i) require(near(loopOut[2 * i], expect[i] / 32768.0f), "loop wrap");
        require(mixer.playing(c), "looping voice released");
        bool threw = false;
        try { mixer.play(sound({{1}}, 8, true, 5)); } catch (const std::runtime_error&) { threw = true; }
        require(threw, "loop start beyond end accepted");
    } catch (const std::exception& error) {
        std::cerr << "mixer-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "mixer-test passed\n";
    return 0;
}
