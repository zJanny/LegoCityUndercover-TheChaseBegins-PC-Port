// Clip sound events of the shipped player clips (src/runtime/ladder_sounds.cpp, docs/re/player-traversal-ladders.md). The values are what
// agent 20261021-audio-C read with parseAnimation and chase-tabledump (tables/tablessoundfx.bxls) from the extracted RomFS, and the rule of
// the handler 0x2c0384 (REA, Ghidra 12.1.2, ev_5d1e34b82f2ace12a91d4a5b333eb081d3fd7b1dbbeed33ba3e561b18dc862cf). No oracle run: what the
// original plays while climbing is recorded as open in TRAV-60. Exits with 77 without the RomFS.
#include "assets/assets.hpp"
#include "runtime/ladder_sounds.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

constexpr const char* kGeneric = "models/char_minifig/class_generic/";
constexpr const char* kPolice = "models/char_minifig/class_police/";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::Animation load(assets::FuseArchive& archive, const std::string& path) {
    const assets::FuseEntry* entry = archive.find(path);
    require(entry != nullptr, "not in archive: " + path);
    return assets::parseAnimation(archive.read(*entry));
}

struct Run {
    std::vector<unsigned> plays, stops;
    std::vector<SoundCall> calls;
};

// The port's animate(): the clip starts at frame 0 in the update that switches to it and advances by dt x fps per update (dt 1/30).
Run replay(const assets::Animation& animation, const std::vector<bool>& looping, const CharacterSounds& character, unsigned updates) {
    Run run;
    ClipSounds sounds(looping);
    GameRandom random(1);
    const double framesPerUpdate = double(assets::animationFramesPerSecond(animation)) / 30.0;
    double frame = framesPerUpdate;
    sounds.restart(animation, float(frame / double(animation.frames)));
    for (unsigned k = 2; k <= updates; ++k) {
        frame = std::fmod(frame + framesPerUpdate, double(animation.frames));
        std::vector<SoundCall> plays;
        std::vector<SoundStop> stops;
        sounds.update(animation, float(frame / double(animation.frames)), {0, 0, 0}, character, random, plays, stops);
        for (const SoundCall& call : plays) {
            run.plays.push_back(k);
            run.calls.push_back(call);
        }
        for (std::size_t i = 0; i < stops.size(); ++i) run.stops.push_back(k);
    }
    return run;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: ladder-sounds-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "ladder-sounds-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* soundTable = archive.find("tables/tablessoundfx.bxls");
        require(soundTable != nullptr, "tablessoundfx");
        const assets::Table table = assets::parseTable(archive.read(*soundTable));
        const std::vector<assets::SoundEffect> effects = assets::parseSoundEffects(table);
        const assets::SoundTables tables = assets::parseSoundTables(table);
        std::vector<bool> looping;
        for (const assets::SoundEffect& effect : effects) looping.push_back((effect.column3 & kSoundFlagLoop) != 0);
        // chase-tabledump tables/tablessoundfx.bxls: 112 abilities/parkour_dangle/dangle_foot_move_01, 288 movement/characters/jump_whoosh_01 and 410
        // vo/chase_vo/effort/vo_chase_effort_small_01 have column 3 = 2 (positional, no loop); 273 props/grapple_move_loop has 3.
        require(!looping[112] && !looping[288] && !looping[410] && looping[273], "loop flags");
        require(effects[112].name == "abilities/parkour_dangle/dangle_foot_move_01", "sound 112");
        const CharacterSounds chase(tables.characterSounds.at(2));  // police row (audio.md): slot 5 = 178, slot 7 = 410, slot 30 = 0
        require(chase.id(7) == 410 && chase.id(30) == 0, "character slots");

        // ladder_up / ladder_down (13 frames, 15 fps): aspect 0x6c22818f, sound 112, volume, pitch and chance 1.0, flag 1; the pulse track
        // 0x0ffe43cf has keys 0:0 0.4179:1 0.5432:0 0.9008:1 0.9826:0 1:0 (down 0.4167 0.5447 0.9062 0.9838).
        const assets::Animation up = load(archive, std::string(kGeneric) + "ladder_up_minifig.bfnanm");
        const assets::Animation down = load(archive, std::string(kGeneric) + "ladder_down_minifig.bfnanm");
        for (const assets::Animation* clip : {&up, &down}) {
            require(clip->frames == 13 && clip->subAspects.size() == 2 && clip->subAspects[0] == kAspectSoundPulse && clip->subAspects[1] == 0xb945fcd8, "ladder clip aspects");
            const auto record = soundPulseRecord(clip->subData[0]);
            require(record && record->id == 112 && record->volume == 1.0f && record->pitch == 1.0f && record->chance == 1.0f && record->flag == 1, "ladder pulse record");
            require(clip->subData[1].empty(), "root motion sub has no data");
        }
        require(std::abs(stepTrackValue(up.subAnimations[0], kSoundPulseTrackHash, 0.43f) - 1.0f) < 1e-6f &&
                    std::abs(stepTrackValue(up.subAnimations[0], kSoundPulseTrackHash, 0.5f) - 1.0f) < 1e-6f &&
                    stepTrackValue(up.subAnimations[0], kSoundPulseTrackHash, 0.6f) == 0.0f, "ladder_up pulse track");
        // Update k of a clip is at k / 26 (13 frames at half a frame per update): the first samples at or after 0.4179 and 0.9008 are 11 and 24
        // (down: 0.4167 and 0.9062 give 11 and 24 too); each play is sound 112 and the sound is not a loop, so nothing stops it.
        for (const assets::Animation* clip : {&up, &down}) {
            const Run run = replay(*clip, looping, chase, 52);
            require((run.plays == std::vector<unsigned>{11, 24, 37, 50}) && run.stops.empty(), "ladder pulses");
            for (const SoundCall& call : run.calls) require(call.id == 112 && !call.onlyIfIdle, "ladder sound id");
        }

        // ladder_dismount (16 frames): pulse record 288 (keys 0:0 0.2555:1 0.3556:0.02 1:0), character sound record slot 7 chance 1.0 (track
        // 0x13e3511c keys 0:0 0.6739:0.969 0.8265:0.02 1:0); the footstep sub 0xcacfb995 is the port's FootstepDetector, not this class.
        const assets::Animation dismount = load(archive, std::string(kGeneric) + "ladder_dismount_minifig.bfnanm");
        require(dismount.frames == 16 && dismount.subAspects.size() == 4 && dismount.subAspects[0] == kAspectSoundPulse &&
                    dismount.subAspects[1] == 0xcacfb995 && dismount.subAspects[2] == kAspectCharacterSound && dismount.subAspects[3] == 0xb945fcd8, "dismount aspects");
        const auto whoosh = soundPulseRecord(dismount.subData[0]);
        require(whoosh && whoosh->id == 288 && whoosh->chance == 1.0f, "dismount pulse record");
        const auto effort = characterSoundRecord(dismount.subData[2]);
        require(effort && effort->slot == 7 && effort->chance == 1.0f && effort->volume == 1.0f && effort->flag == 1, "dismount character record");
        const Run run = replay(dismount, looping, chase, 30);
        // k / 32: 0.2555 is reached at k = 9 (sound 288), 0.6739 at k = 22 (character slot 7 = 410).
        require((run.plays == std::vector<unsigned>{9, 22}) && run.calls[0].id == 288 && run.calls[1].id == 410 && run.stops.empty(), "dismount sounds");

        // jump (class_police): the pulse record has sound 0, so the clip is silent here (the take-off sound comes from the state, audio.md); idle
        // (class_police): aspect 0x3a01a706 with slot 30 (0 in the police row), chance 0.5.
        const assets::Animation jump = load(archive, std::string(kPolice) + "jump_minifig.bfnanm");
        require(jump.subAspects[0] == kAspectSoundPulse && soundPulseRecord(jump.subData[0])->id == 0, "jump record");
        require(replay(jump, looping, chase, 60).plays.empty(), "jump is silent");
        const assets::Animation idle = load(archive, std::string(kPolice) + "idle_minifig.bfnanm");
        const auto grunt = characterSoundRecord(idle.subData[0]);
        require(idle.subAspects[0] == kAspectCharacterSound && grunt && grunt->slot == 30 && grunt->chance == 0.5f, "idle record");
        require(replay(idle, looping, chase, 200).plays.empty(), "idle slot 30 is silent for the police row");
        const assets::Animation walk = load(archive, std::string(kPolice) + "walk_minifig.bfnanm");
        require(walk.subAspects[0] == 0xcacfb995 && replay(walk, looping, chase, 60).plays.empty(), "walk has no clip sound");
    } catch (const std::exception& error) {
        std::cerr << "ladder-sounds-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ladder-sounds-real-test: ok\n";
    return 0;
}
