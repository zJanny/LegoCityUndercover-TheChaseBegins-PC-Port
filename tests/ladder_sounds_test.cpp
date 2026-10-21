// Clip sound events (src/runtime/ladder_sounds.cpp) with made-up clips. The key times and records are the ones of the shipped clips
// (read by agent 20261021-audio-C with parseAnimation; the replay against the real files is in clip-sounds-real-data); the trigger rule is
// the integer compares of 0x2c0384 (pulse > 0x34000000 / < -0x34000000) and 0x19f128 (> 0.5), docs/re/player-traversal-ladders.md.
#include "runtime/ladder_sounds.hpp"
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::AnimTrack track(std::uint32_t hash, std::initializer_list<std::pair<float, float>> keys) {
    assets::AnimTrack result;
    result.hash = hash;
    for (const auto& [time, value] : keys) {
        assets::CurveKey key;
        key.time = time;
        key.value = value;
        result.keys.push_back(key);
    }
    return result;
}

assets::Bytes pulseData(std::uint32_t id, float chance = 1.0f) {
    assets::Bytes data(20, 0);
    const float one = 1.0f;
    std::memcpy(&data[0], &id, 4);
    std::memcpy(&data[4], &one, 4);
    std::memcpy(&data[8], &one, 4);
    std::memcpy(&data[12], &chance, 4);
    data[16] = 1;
    return data;
}

assets::Bytes characterData(std::uint32_t slot, float chance) {
    assets::Bytes data(16, 0);
    const float one = 1.0f;
    std::memcpy(&data[0], &slot, 4);
    std::memcpy(&data[4], &chance, 4);
    std::memcpy(&data[8], &one, 4);
    data[12] = 1;
    return data;
}

// ladder_up: 13 frames at 15 fps, pulse track keys 0:0 0.4179:1 0.5432:0 0.9008:1 0.9826:0 1.0:0 (20261021-audio-C, parseAnimation).
assets::Animation ladderUpLike(std::uint32_t id, float chance = 1.0f) {
    assets::Animation animation;
    animation.frames = 13;
    animation.subAnimations = {{track(kSoundPulseTrackHash, {{0, 0}, {0.4179f, 1}, {0.5432f, 0}, {0.9008f, 1}, {0.9826f, 0}, {1.0f, 0}})}, {}};
    animation.subAspects = {kAspectSoundPulse, 0xb945fcd8};
    animation.subData = {pulseData(id, chance), {}};
    return animation;
}

struct Run {
    std::vector<int> plays, stops;
    std::vector<SoundCall> calls;
};

Run replay(ClipSounds& sounds, const assets::Animation& animation, unsigned updates, double framesPerUpdate, const CharacterSounds& character, GameRandom& random) {
    Run run;
    double frame = 0;
    sounds.restart(animation, 0.0f);
    for (unsigned k = 1; k <= updates; ++k) {
        frame = std::fmod(frame + framesPerUpdate, double(animation.frames));
        std::vector<SoundCall> plays;
        std::vector<SoundStop> stops;
        sounds.update(animation, float(frame / double(animation.frames)), {1, 2, 3}, character, random, plays, stops);
        for (const SoundCall& call : plays) {
            run.plays.push_back(int(k));
            run.calls.push_back(call);
        }
        for (const SoundStop& stop : stops) {
            (void)stop;
            run.stops.push_back(int(k));
        }
    }
    return run;
}

void records() {
    const assets::Bytes ladderUp = {0x70, 0, 0, 0, 0, 0, 0x80, 0x3f, 0, 0, 0x80, 0x3f, 0, 0, 0x80, 0x3f, 0x01, 0xb4, 0x3a, 0x0c};
    const auto pulse = soundPulseRecord(ladderUp);
    require(pulse && pulse->id == 112 && pulse->volume == 1.0f && pulse->pitch == 1.0f && pulse->chance == 1.0f && pulse->flag == 1, "ladder_up record");
    const assets::Bytes idle = {0x1e, 0, 0, 0, 0, 0, 0, 0x3f, 0, 0, 0x80, 0x3f, 0x01, 0, 0, 0};
    const auto character = characterSoundRecord(idle);
    require(character && character->slot == 30 && character->chance == 0.5f && character->volume == 1.0f && character->flag == 1, "idle record");
    require(!soundPulseRecord(assets::Bytes(8, 0)) && !characterSoundRecord(assets::Bytes(4, 0)), "short blocks");
}

void pulsesOfALadderClip() {
    const CharacterSounds character;
    GameRandom random(1);
    ClipSounds oneShots;
    const assets::Animation up = ladderUpLike(112);
    // 15 fps at dt 1/30 is half a frame per update: the sample k is at k / 26 of the clip; the rises are the first samples at or after
    // 0.4179 (k = 11) and 0.9008 (k = 24); a one-shot sound (loop flag clear) has no stop.
    const Run run = replay(oneShots, up, 60, 0.5, character, random);
    require((run.plays == std::vector<int>{11, 24, 37, 50}), "one-shot plays");
    require(run.stops.empty(), "one-shot has no stops");
    require(run.calls[0].id == 112 && !run.calls[0].onlyIfIdle && run.calls[0].position[2] == 3.0f, "one-shot call");

    std::vector<bool> looping(200, false);
    looping[112] = true;
    ClipSounds loops(looping);
    const Run loop = replay(loops, up, 30, 0.5, character, random);
    require((loop.plays == std::vector<int>{11, 24}), "looping plays");
    require((loop.stops == std::vector<int>{15, 26}), "looping stops at the falling edges");
    require(loop.calls[0].onlyIfIdle && loop.calls[0].owner == kClipSoundOwnerStandIn, "looping call waits for the running voice");
}

void silentRecordAndFirstUpdate() {
    const CharacterSounds character;
    GameRandom random(1);
    ClipSounds sounds;
    require(replay(sounds, ladderUpLike(0), 60, 0.5, character, random).plays.empty(), "id 0 is silent");
    // A clip that starts inside the high part of the pulse: the restart takes the sample as the previous one.
    const assets::Animation up = ladderUpLike(112);
    sounds.restart(up, 0.45f);
    std::vector<SoundCall> plays;
    std::vector<SoundStop> stops;
    sounds.update(up, 0.46f, {0, 0, 0}, character, random, plays, stops);
    require(plays.empty() && stops.empty(), "no edge in the first update");
}

void chanceDrawsFromTheGameGenerator() {
    const CharacterSounds character;
    GameRandom drawn(7), reference(7);
    ClipSounds sounds;
    const Run run = replay(sounds, ladderUpLike(112, 0.5f), 26, 0.5, character, drawn);
    const bool first = gameRandomUnit(reference) < 0.5f, second = gameRandomUnit(reference) < 0.5f;
    require(int(run.plays.size()) == int(first) + int(second), "chance 0.5 plays when the draw is below it");
    require(drawn.table() == reference.table() && drawn.firstCursor() == reference.firstCursor(), "one draw per rising edge");
}

void characterSoundTrack() {
    // ladder_dismount sub 2: aspect 0x3a01a706, track 0x13e3511c keys 0:0 0.6739:0.969 0.8265:0.02 1:0, slot 7, chance 1.0; 16 frames.
    assets::Animation dismount;
    dismount.frames = 16;
    dismount.subAnimations = {{track(kCharacterSoundTrackHash, {{0, 0}, {0.6739f, 0.969f}, {0.8265f, 0.02f}, {1.0f, 0}})}};
    dismount.subAspects = {kAspectCharacterSound};
    dismount.subData = {characterData(7, 1.0f)};
    std::array<std::uint16_t, assets::kCharacterSoundSlots> slots{};
    slots[7] = 410;
    const CharacterSounds character(slots);
    GameRandom random(1);
    ClipSounds sounds;
    const Run run = replay(sounds, dismount, 30, 0.5, character, random);
    require((run.plays == std::vector<int>{22}) && run.calls[0].id == 410 && run.calls[0].owner == 0, "slot 7 at the first sample after 0.6739");
    require(run.stops.empty(), "not a looping sound");

    slots[7] = 0;
    GameRandom other(1);
    ClipSounds silent;
    require(replay(silent, dismount, 30, 0.5, CharacterSounds(slots), other).plays.empty(), "slot without sound");
}

void otherAspectsAreIgnored() {
    assets::Animation animation = ladderUpLike(112);
    animation.subAspects[0] = 0xb945fcd8;
    GameRandom random(1);
    ClipSounds sounds;
    require(replay(sounds, animation, 60, 0.5, CharacterSounds(), random).plays.empty(), "only the sound aspects fire");
}
}

int main() {
    try {
        records();
        pulsesOfALadderClip();
        silentRecordAndFirstUpdate();
        chanceDrawsFromTheGameGenerator();
        characterSoundTrack();
        otherAspectsAreIgnored();
    } catch (const std::exception& error) {
        std::cerr << "ladder-sounds-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ladder-sounds-test: ok\n";
    return 0;
}
