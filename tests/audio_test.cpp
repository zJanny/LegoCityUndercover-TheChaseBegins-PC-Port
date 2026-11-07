// Synthetic checks of the sound layer: per-voice gains of the mixer, the positional gains (0x107fa0 / 0x25c388), the
// mood rule of the level music (0x206c40), the footstep rise detector (0x19f10c) and the landing sound choice
// (0x1abc5c). The values are worked out by hand from the disassembly (docs/re/audio.md). Recorded game values are in
// audio_real_test.cpp.
#include "audio/effectvoices.hpp"
#include "audio/mixer.hpp"
#include "audio/spatial.hpp"
#include "runtime/music.hpp"
#include "runtime/sound_events.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace chase;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }
}

int main() {
    try {
        {
            audio::Mixer mixer(8);
            auto sound = std::make_shared<assets::Sound>();
            sound->rate = 8;
            sound->frames = 2;
            sound->channels = {{16384, 16384}};
            const int voice = mixer.play(sound, 0.25f, 1.0f);
            float out[4];
            mixer.mix(out, 2);
            require(near(out[0], 0.125f) && near(out[1], 0.5f), "left and right gain");
            const int second = mixer.play(sound, 1.0f, 1.0f);
            require(mixer.setGains(second, 0.0f, 0.5f) && !mixer.setGains(voice, 1, 1), "setGains");
            mixer.mix(out, 2);
            require(near(out[0], 0.0f) && near(out[1], 0.25f), "changed gains");
        }
        {
            // 0x1cef9c(1) holds the voices that run (state 1) until 0x1cef9c(0); a voice started in between plays (BulletTime, 20261103-large-D).
            audio::Mixer mixer(8);
            auto sound = std::make_shared<assets::Sound>();
            sound->rate = 8;
            sound->frames = 4;
            sound->channels = {{8192, 16384, 24576, 8192}};
            const int held = mixer.play(sound, 1.0f);
            float out[4];
            mixer.mix(out, 1);
            mixer.pauseAll();
            const int later = mixer.play(sound, 1.0f);
            require(mixer.paused(held) && !mixer.paused(later), "pauseAll holds the voices that run now only");
            mixer.mix(out, 2);
            require(near(out[0], 0.25f) && near(out[2], 0.5f), "the held voice stays silent, the later one plays from its start");
            mixer.stop(later);
            mixer.resumeAll();
            mixer.mix(out, 1);
            require(!mixer.paused(held) && near(out[0], 0.5f), "resumeAll continues at the held frame");
        }
        {
            audio::Mixer mixer(8);
            auto sound = std::make_shared<assets::Sound>();
            sound->rate = 8;
            sound->frames = 8;
            sound->channels = {{0, 8192, 16384, 24576, 16384, 8192, 0, 0}};
            audio::EffectVoices voices(mixer);
            const int voice = mixer.play(sound, 1.0f);
            voices.add(voice, 12, 7);
            require(voices.find(12, 7) == voice && voices.find(12, 8) == -1 && voices.find(13, 7) == -1, "find by sound and owner (0x23fcac)");
            float out[4];
            require(mixer.setGains(voice, 0.5f, 0.5f) && mixer.setPitch(voice, 2.0f), "gains and pitch of a running voice");
            mixer.mix(out, 2);
            require(near(out[0], 0.0f) && near(out[2], 0.5f * 16384.0f / 32768.0f) && near(out[3], out[2]), "half gain, double pitch skips every other frame");
            mixer.stop(voice);
            require(!mixer.setGains(voice, 1.0f, 1.0f) && !mixer.setPitch(voice, 1.0f) && voices.find(12, 7) == -1, "an ended voice takes no change");
        }
        {
            // Stop of effect voices by (sound id, owner), 0x23ae50 -> 0x23f75c -> 0x23fcac: the table fade 0 ends the voice at once
            // (door sounds 598..617 all have column 16 = 0), a positive time fades it out linearly (stand-in AUDIO-S7).
            audio::Mixer mixer(8);
            audio::EffectVoices voices(mixer);
            auto sound = std::make_shared<assets::Sound>();
            sound->rate = 8;
            sound->frames = 8;
            sound->loop = true;
            sound->channels = {{16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384}};
            const int doorA = mixer.play(sound, 1.0f), doorB = mixer.play(sound, 1.0f), other = mixer.play(sound, 1.0f);
            voices.add(doorA, 605, 1);
            voices.add(doorB, 605, 2);
            voices.add(other, 601, 1);
            require(!voices.stop(605, 3, 0.0f) && !voices.stop(602, 1, 0.0f) && mixer.activeVoices() == 3, "a stop without a matching voice does nothing");
            require(voices.stop(605, 1, 0.0f) && !mixer.playing(doorA) && mixer.playing(doorB) && mixer.playing(other), "stop ends only the voice of that sound and owner");
            require(!voices.stop(605, 1, 0.0f), "the stopped voice is gone");
            require(voices.stop(601, 1, 1.0f) && mixer.playing(other), "a fade keeps the voice running");
            float out[16];
            mixer.mix(out, 4);
            require(mixer.playing(other), "still audible after half the fade");
            mixer.mix(out, 8);
            require(!mixer.playing(other) && mixer.playing(doorB), "the fade ends the voice");
            const int again = mixer.play(sound, 1.0f);
            voices.add(again, 605, 1);
            require(voices.stop(605, 1, 0.0f) && !mixer.playing(again), "a new voice of the same sound and owner can be stopped");
        }
        {
            audio::Mixer mixer(8);
            auto sound = std::make_shared<assets::Sound>();
            sound->rate = 8;
            sound->frames = 8;
            sound->loop = true;
            sound->channels = {{16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384}};
            const int voice = mixer.play(sound, 1.0f);
            mixer.fadeOut(voice, 1.0f);
            float out[16];
            mixer.mix(out, 8);
            require(near(out[0], 0.5f) && near(out[2], 0.5f * 7.0f / 8.0f) && near(out[14], 0.5f * 1.0f / 8.0f), "linear fade over the time");
            require(!mixer.playing(voice), "the faded voice ends");
            const int quick = mixer.play(sound, 1.0f);
            mixer.fadeOut(quick, 0.0f);
            require(!mixer.playing(quick), "fade time 0 stops at once");
        }
        {
            require(near(audio::distanceVolume(5, 10, 30), 1.0f) && near(audio::distanceVolume(10, 10, 30), 1.0f), "inside the minimum distance");
            require(near(audio::distanceVolume(20, 10, 30), 0.5f) && near(audio::distanceVolume(25, 10, 30), 0.25f), "linear falloff");
            require(near(audio::distanceVolume(30, 10, 30), 0.0f) && near(audio::distanceVolume(40, 10, 30), 0.0f), "beyond the maximum distance");
            const audio::Listener listener = audio::listenerFromForward({0, 0, 0}, {0, 0, 1});
            require(near(listener.right[0], 1) && near(listener.up[1], 1), "listener axes");
            require(near(audio::panOf(listener, {5, 0, 0}), 1.0f) && near(audio::panOf(listener, {-5, 3, 0}), -1.0f), "pan to the sides");
            require(near(audio::panOf(listener, {0, 0, 7}), 0.0f) && near(audio::panOf(listener, {0, 4, 0}), 0.0f), "pan ahead and above");
            require(near(audio::panOf(listener, {3, 0, 3}), std::sqrt(0.5f)), "pan diagonal");
            const audio::EmitterShape shape{0.5f, 10, 30};
            const audio::StereoGains right = audio::positionalGains(shape, listener, {20, 0, 0}, 0.8f);
            require(near(right.left, 0.0f) && near(right.right, 0.5f * 0.5f * 0.8f), "emitter at the right");
            const audio::StereoGains diagonal = audio::positionalGains(shape, listener, {0, 0, 10}, 1.0f);
            require(near(diagonal.left, 0.5f) && near(diagonal.right, 0.5f), "emitter ahead at the minimum distance");
            const audio::StereoGains far = audio::positionalGains(shape, listener, {0, 0, 35}, 1.0f);
            require(near(far.left, 0.0f) && near(far.right, 0.0f), "emitter beyond the maximum distance");
            const audio::StereoGains flat = audio::flatGains(shape, 0.8f);
            require(near(flat.left, 0.4f) && near(flat.right, 0.4f), "flat gains");
            const audio::Listener tilted = audio::listenerFromForward({1, 2, 3}, {0.5f, -0.5f, 0.7f});
            require(near(tilted.right[1], 0.0f) && near(tilted.right[0] * tilted.right[0] + tilted.right[2] * tilted.right[2], 1.0f), "camera right axis is horizontal");
        }
        {
            using runtime::chooseMoodTrack;
            const assets::MusicSet quiet{0, 0}, fightOnly{255, 5}, both{2, 3};
            require(chooseMoodTrack(fightOnly, 0, 0).stop, "entry 255 stops the music");
            const runtime::MoodChoice action = chooseMoodTrack(fightOnly, 1, 0);
            require(!action.stop && action.track && *action.track == 5 && action.mood == 1, "action mood track");
            const runtime::MoodChoice swapped = chooseMoodTrack(quiet, 0, 7);
            require(swapped.mood == 1 && swapped.track && *swapped.track == 0, "entry 0 swaps the mood");
            require(!chooseMoodTrack(quiet, 0, 0).track, "track 0 while track 0 is current starts nothing");
            require(*chooseMoodTrack(both, 0, 0).track == 2 && !chooseMoodTrack(both, 0, 2).track, "same track keeps playing");
        }
        {
            runtime::FootstepDetector detector;
            require(!detector.update(0.0f) && detector.update(1.0f) && !detector.update(1.0f) && !detector.update(0.0f) && detector.update(1.0f), "rises over 0.5 fire once");
            require(!detector.update(0.2f) && !detector.update(0.6f), "a rise of 0.4 does not fire");
            detector.restart(1.0f);
            require(!detector.update(1.0f), "restart makes the first sample the reference");
            runtime::FootstepDetector weighted;
            require(!weighted.update(1.0f, 0.4f) && weighted.update(1.0f, 1.0f) == true, "weight scales the rise");
            assets::Animation animation;
            assets::AnimTrack track;
            track.hash = runtime::kFootstepTrackHash;
            for (const auto& [time, value] : {std::pair{0.0f, 0.0f}, {0.25f, 1.0f}, {0.5f, 0.0f}, {1.0f, 0.0f}}) {
                assets::CurveKey key;
                key.time = time;
                key.value = value;
                track.keys.push_back(key);
            }
            animation.subAnimations.push_back({track});
            require(runtime::footstepTrackValue(animation, 0.1f) == 0.0f && runtime::footstepTrackValue(animation, 0.25f) == 1.0f &&
                        runtime::footstepTrackValue(animation, 0.49f) == 1.0f && runtime::footstepTrackValue(animation, 0.5f) == 0.0f,
                    "footstep track is sampled as a step function");
            require(runtime::footstepTrackValue(assets::Animation(), 0.3f) == 0.0f, "no track, no footstep");
        }
        {
            std::array<std::uint16_t, assets::kCharacterSoundSlots> slots{};
            slots[runtime::kSlotFootstep] = 178;
            slots[runtime::kSlotLandFromHeight] = 410;
            slots[runtime::kSlotLandWater] = 182;
            slots[runtime::kSlotLandOnObject11] = 187;
            slots[runtime::kSlotLandHard] = 411;
            const runtime::CharacterSounds sounds(slots);
            const std::array<float, 3> where{1, 2, 3};
            auto soft = sounds.landing(1.0f, false, false, where);
            require(soft.size() == 1 && soft[0].id == 178 && soft[0].position == where, "a fall of 1.0 or less lands with the surface sound only");
            auto high = sounds.landing(1.01f, false, false, where);
            require(high.size() == 2 && high[0].id == 410 && high[1].id == 178, "a longer fall plays the impact sound first");
            const auto hard = sounds.hardLanding(where);
            require(hard.size() == 2 && hard[0].id == 285 && hard[1].id == 411, "hard landing plays big_fall and the slot 8 sound");
            require(sounds.landing(0, true, true, where)[0].id == 182 && sounds.landing(0, false, true, where)[0].id == 187, "water before object 11 before default");
        }
    } catch (const std::exception& error) {
        std::cerr << "audio-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "audio-test passed\n";
    return 0;
}
