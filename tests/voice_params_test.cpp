// Tests of the volume and rate fields of sound voices (src/audio/voice_params.*, SoundPlayer::setVolume / scaleFrequency; docs/re/audio.md,
// entry of 2026-10-21). Values worked out by hand from llvm-objdump of the original (20261021-audio-A): voice set-up 0x23f82c / 0x23f940
// (volume = percent * 0.01f, 0x3c23d70a; rate boost r0 + (r0 >> 1) at 0x23f908), SetSoundVolume 0x219178 -> 0x227ff4 (float stored at voice
// +0x50, dirty bit 0), ScaleSoundFrequency 0x22f770 (vcvt.s32.f32 of scale * (float)rate) -> 0x227f4c (-1 stores the source rate, +0x58, dirty
// bit 2), voice synchroniser 0x25c388 (volume = voice +0x50 * [0x37636c + 4]; pitch = (float)(int)rate * doppler / (float)u32 [sound +0xc],
// clamped at 0 by 0x25b3b4; Doppler 1 - dot * 0x3b3ec9f8 * [0x37636c + 8], at least 0x3a83126f) and the emitter update 0x107fa0 (voice volume
// times the distance factor). The sound rows come from tables/tablessoundfx.bxls (build/chase-tabledump, sheet 0, 2026-10-21): row 1
// ui/brickcounter1 percent 80, distance 20..30; row 178 fs_default1 percent 20, distance 10..30; no row has bit 3 of column 3.
// Arguments: fib, names. Without them only the arithmetic runs; with data missing it exits with 77.
#include "assets/assets.hpp"
#include "audio/mixer.hpp"
#include "audio/soundplayer.hpp"
#include "audio/voice_params.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace chase;
using namespace chase::audio;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

void arithmetic() {
    expect(initialVoiceVolume(80) == 0.79999995f && initialVoiceVolume(100) == 1.0f && initialVoiceVolume(0) == 0.0f, "volume is percent * 0.01f");
    expect(boostedRate(12000) == 18000 && boostedRate(11999) == 17998 && boostedRate(7025) == 10537, "the boost adds half of the rate, rounded down");
    expect(scaledRate(0.5f, 12000, false) == 6000 && scaledRate(1.5f, 7025, false) == 10537, "the scaled rate truncates towards zero");
    expect(scaledRate(1.0f, 12000, true) == 18000 && scaledRate(2.0f, 12000, true) == 36000, "the boost applies before the scale");
    expect(scaledRate(-0.5f, 12000, false) == -6000 && scaledRate(0.0f, 12000, false) == 0, "negative and zero scales");
    expect(scaledRate(1e10f, 12000, false) == std::numeric_limits<std::int32_t>::max() &&
               scaledRate(-1e10f, 12000, false) == std::numeric_limits<std::int32_t>::min(),
           "the conversion saturates");
    expect(scaledRate(std::numeric_limits<float>::quiet_NaN(), 12000, false) == 0, "NaN converts to 0");
    expect(storedRate(-1, 12000) == 12000 && storedRate(-2, 12000) == -2 && storedRate(0, 12000) == 0 && storedRate(7000, 12000) == 7000,
           "only -1 stores the source rate");
    expect(scaledRate(-1.0f / 12000.0f, 12000, false) == -1 && storedRate(scaledRate(-1.0f / 12000.0f, 12000, false), 12000) == 12000,
           "a scale that gives -1 restores the source rate");
    expect(pitchOfRate(12000, 1.0f, 12000) == 1.0f && pitchOfRate(6000, 1.0f, 12000) == 0.5f && pitchOfRate(6000, 2.0f, 12000) == 1.0f,
           "pitch is rate * doppler / source rate");
    expect(pitchOfRate(-5, 1.0f, 12000) == 0.0f && pitchOfRate(0, 1.0f, 12000) == 0.0f, "pitch stays at 0 or above");
    const std::array<float, 3> still = {0, 0, 0};
    expect(dopplerFactor(still, still, {0, 0, 20}, kDopplerScale) == 1.0f, "no relative velocity, no Doppler");
    expect(near(dopplerFactor(still, {0, 0, 10}, {0, 0, 20}, 1.0f), 1.0291121f, 1e-6f), "the listener moves towards the emitter: pitch up by 10 / 343.5");
    expect(near(dopplerFactor({0, 0, 10}, still, {0, 0, 20}, 1.0f), 0.9708879f, 1e-6f), "the emitter moves away: pitch down");
    expect(near(dopplerFactor({0, 0, 10}, still, {0, 0, -20}, 1.0f), 1.0291121f, 1e-6f), "the direction sign matters");
    expect(near(dopplerFactor({0, 0, 10}, still, {0, 0, 20}, 2.0f), 0.9417758f, 1e-6f), "the scale multiplies the shift");
    expect(dopplerFactor({0, 0, 1000}, still, {0, 0, 20}, 1.0f) == 0.001f, "the factor is at least 0.001 (0x3a83126f)");
    expect(near(dopplerFactor({0, 0, 10}, still, {0, 0, 1e-9f}, 1.0f), 0.9708879f, 1e-6f), "a direction under 2^-23 becomes (0, 0, 1)");
    expect(near(dopplerFactor({3, 4, 0}, still, {3, 4, 0}, 1.0f), 1.0f - 5.0f * 0.0029112082f, 1e-6f), "the direction is normalised");
}

float expectedSample(const std::vector<std::int16_t>& channel, double position) {
    const std::size_t at = std::size_t(position);
    const double fraction = position - double(at);
    const std::size_t next = at + 1 < channel.size() ? at + 1 : at;
    return float((channel[at] + (channel[next] - channel[at]) * fraction) / 32768.0);
}

std::vector<float> mixed(Mixer& mixer, std::size_t frames) {
    std::vector<float> out(2 * frames);
    mixer.mix(out.data(), frames);
    return out;
}

void run(const std::string& fib, const std::string& names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::Table table = assets::parseTable(archive.read(*archive.find("tables/tablessoundfx.bxls")));
    const std::vector<assets::SoundEffect> effects = assets::parseSoundEffects(table);
    for (const assets::SoundEffect& effect : effects)
        if (effect.column3 & kRateBoostRecordBit) {
            std::printf("FAIL: sound row %u has the rate boost bit\n", effect.id);
            ++failures;
        }
    const unsigned flatId = 1, positionalId = 178;
    expect(effects[flatId].volumePercent == 80 && effects[positionalId].volumePercent == 20 && effects[positionalId].paramA == 10 &&
               effects[positionalId].paramB == 30,
           "sound rows 1 and 178");
    const assets::Sound flat = assets::decodeSound(archive.read(*archive.find(assets::soundEffectPath(effects[flatId]))));
    const assets::Sound step = assets::decodeSound(archive.read(*archive.find(assets::soundEffectPath(effects[positionalId]))));
    expect(flat.frames > 200 && step.frames > 200 && flat.rate > 0 && step.rate > 0 && !flat.loop && !step.loop, "the two sounds are long and not looping");
    constexpr std::size_t frames = 40;
    const float base = initialVoiceVolume(80);

    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        expect(player.play(flatId, nullptr, 7) >= 0, "flat sound plays");
        const std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f) same = same && near(out[2 * f], expectedSample(flat.channels[0], double(f)) * base) && near(out[2 * f + 1], out[2 * f]);
        expect(same, "an untouched flat voice plays at its table volume and source rate");
    }
    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        player.play(flatId, nullptr, 7);
        expect(player.setVolume(flatId, 7, 0.4f) && !player.setVolume(flatId, 8, 0.4f) && !player.setVolume(flatId + 1, 7, 0.4f), "SetSoundVolume finds the voice by sound and owner");
        const std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f) same = same && near(out[2 * f], expectedSample(flat.channels[0], double(f)) * 0.4f);
        expect(same, "SetSoundVolume replaces the table volume (0.4, not 0.8 * 0.4)");
    }
    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        player.setMasterVolume(0.5f);
        player.play(flatId, nullptr, 7);
        player.setVolume(flatId, 7, 1.5f);
        std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f) same = same && near(out[2 * f], expectedSample(flat.channels[0], double(f)) * 0.75f);
        expect(same, "the master volume multiplies the voice volume, which is not limited to 1");
    }
    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        player.play(flatId, nullptr, 7);
        player.setMasterVolume(0.25f);
        const std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f) same = same && near(out[2 * f], expectedSample(flat.channels[0], double(f)) * base * 0.25f);
        expect(same, "a master volume change reaches the running voice (0x1dca5c)");
    }
    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        player.play(flatId, nullptr, 7);
        expect(player.scaleFrequency(flatId, 7, 2.0f) && !player.scaleFrequency(flatId, 9, 2.0f), "ScaleSoundFrequency finds the voice by sound and owner");
        const std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f) same = same && near(out[2 * f], expectedSample(flat.channels[0], double(2 * f)) * base);
        expect(same, "twice the rate skips every other source frame");
    }
    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        player.play(flatId, nullptr, 7);
        player.scaleFrequency(flatId, 7, 0.5f);
        player.scaleFrequency(flatId, 7, 1.0f);
        const std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f) same = same && near(out[2 * f], expectedSample(flat.channels[0], double(f)) * base);
        expect(same, "a scale of 1 restores the source rate");
    }
    {
        Mixer mixer(flat.rate);
        SoundPlayer player(archive, mixer);
        player.play(flatId, nullptr, 7);
        player.scaleFrequency(flatId, 7, -1.0f);
        const std::vector<float> out = mixed(mixer, frames);
        bool silentStart = true;
        for (std::size_t f = 1; f < frames; ++f) silentStart = silentStart && near(out[2 * f], out[0]);
        expect(silentStart, "a negative rate keeps the pitch at 0: the voice stands still");
    }
    {
        Mixer mixer(step.rate);
        SoundPlayer player(archive, mixer);
        player.setListener(listenerFromForward({0, 0, 0}, {0, 0, 1}));
        const std::array<float, 3> emitter = {20, 0, 0};
        expect(player.play(positionalId, &emitter, 3) >= 0, "positional sound plays");
        player.setVolume(positionalId, 3, 1.0f);
        player.update();
        const std::vector<float> out = mixed(mixer, frames);
        bool same = true;
        for (std::size_t f = 0; f < frames; ++f)
            same = same && near(out[2 * f], 0.0f) && near(out[2 * f + 1], expectedSample(step.channels[0], double(f)) * 0.5f);
        expect(same, "a positional voice at 20 of 10..30 to the right: left 0, right = volume 1 * distance 0.5");
    }
    {
        Mixer mixer(step.rate);
        SoundPlayer player(archive, mixer);
        Listener listener = listenerFromForward({0, 0, 0}, {0, 0, 1});
        listener.velocity = {0, 0, 10};
        player.setListener(listener);
        const std::array<float, 3> emitter = {0, 0, 20};
        player.play(positionalId, &emitter, 3);
        player.update();
        const std::vector<float> out = mixed(mixer, frames);
        const float pitch = pitchOfRate(step.rate, dopplerFactor({0, 0, 0}, {0, 0, 10}, emitter, kDopplerScale), std::uint32_t(step.rate));
        const double stepSize = double(pitch) * double(step.rate) / double(step.rate);
        double position = 0.0;
        bool same = true;
        const float gain = initialVoiceVolume(20) * 0.5f;
        for (std::size_t f = 0; f < frames; ++f, position += stepSize) same = same && near(out[2 * f], expectedSample(step.channels[0], position) * gain);
        expect(same && pitch > 1.029f && pitch < 1.0292f, "the listener velocity bends the pitch of a positional voice");
    }
}

} // namespace

int main(int argc, char** argv) {
    arithmetic();
    if (argc >= 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("skipped: RomFS missing\n");
            return failures ? 1 : 77;
        }
        try {
            run(argv[1], argv[2]);
        } catch (const std::exception& error) {
            std::printf("FAIL: %s\n", error.what());
            return 1;
        }
    }
    if (failures) return 1;
    std::printf("voice params: ok\n");
    return 0;
}
