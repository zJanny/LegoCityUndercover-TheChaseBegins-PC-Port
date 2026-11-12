// The throw clip of the goon in MELEE PUTDOWN against the character mover (runtime/character_air_mover.hpp): grabbed_putdown_07, the clip of fight
// move 3 the oracle recorded (docs/re/combat-characters.md §2026-11-02T15:56:52), carries the fall-allowed track (event kind 0x0c39ee40, hash
// 0xc87674da); the player's grab_putdown_07 does not. Keys recorded by 20261112-enemy-E,
// 2026-11-12, from the shipped clips with assets::parseAnimation (a scratch scan of every .bfnanm of the archive).
#include "runtime/character_air_mover.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: character-air-mover-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "character-air-mover-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const auto clip = [&](const std::string& name) {
            const assets::FuseEntry* entry = archive.find("models/char_minifig/class_generic/" + name + "_minifig.bfnanm");
            require(entry != nullptr, "not in archive: " + name);
            return assets::parseAnimation(archive.read(*entry));
        };
        const assets::Animation thrown = clip("grabbed_putdown_07");
        require(thrown.frames == 47, "grabbed_putdown_07 has 47 frames");
        CharacterAir air;
        sampleFallAllowed(air, thrown, 0.28f);
        require(!air.fallAllowed, "the track is 0 before its key at 0.282526");
        sampleFallAllowed(air, thrown, 0.2826f);
        require(air.fallAllowed, "1.0 from 0.282526 on: bit 4 set");
        sampleFallAllowed(air, thrown, 46.0f / 47.0f);
        require(air.fallAllowed, "still 1.0 in the last frame");
        sampleFallAllowed(air, thrown, 1.0f);
        require(!air.fallAllowed, "the last key at 1.0 is 0");

        CharacterAir player;
        player.fallAllowed = true;
        sampleFallAllowed(player, clip("grab_putdown_07"), 0.1f);
        require(player.fallAllowed, "the player's clip has no fall-allowed track: bit 4 is left");

    } catch (const std::exception& error) {
        std::cerr << "character-air-mover-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "character-air-mover-real-test ok\n";
    return 0;
}
