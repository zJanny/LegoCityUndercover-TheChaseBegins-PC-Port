// The idle animations of the level characters on real data. Provenance (oracle save state before the donut shop waypoint of mission 1-1, GDB reads of the animation slots of
// 1-1_AI_DonutShopMan, 1-1_AI_DonutShopMan01 and 1-3_Blocker, docs/re/npc-animation.md): all three play models/char_minifig/class_generic/
// idle_minifig (slot length 0x13 = 19 frames) and idle2_minifig (0x45 = 69 frames) at 15.0 frames per second. Needs the extracted RomFS;
// exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: npc-idle-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "npc-idle-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase::assets;
        FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const Level level = parseLevel(archive.read(*archive.find(kLevelFile)));
        const DrawableObjects drawable = drawableObjects(archive, level);
        for (const char* instance : {"1-1_AI_DonutShopMan", "1-1_AI_DonutShopMan01", "1-3_Blocker"}) {
            bool found = false;
            for (const CharacterObject& character : drawable.characters) {
                if (character.placement.instance != instance) continue;
                found = true;
                require(character.placement.className == "chars/char_ai_actor", std::string(instance) + " class");
                require(character.animationClass == "class_generic", std::string(instance) + " animation class");
            }
            require(found, std::string(instance) + " is a drawn character");
        }
        const Animation idle = parseAnimation(archive.read(*archive.find("models/char_minifig/class_generic/idle_minifig.bfnanm")));
        const Animation idle2 = parseAnimation(archive.read(*archive.find("models/char_minifig/class_generic/idle2_minifig.bfnanm")));
        require(idle.frames == 19 && std::abs(animationFramesPerSecond(idle) - 15.0f) < 1e-6f, "idle clip");
        require(idle2.frames == 69 && std::abs(animationFramesPerSecond(idle2) - 15.0f) < 1e-6f, "idle2 clip");
        for (const char* folder : {"class_construction", "class_farmer", "class_female", "class_fire", "class_miner", "class_police", "class_robber", "class_space"}) {
            require(archive.find(std::string("models/char_minifig/") + folder + "/idle_minifig.bfnanm") != nullptr, std::string(folder) + " idle");
            require(archive.find(std::string("models/char_minifig/") + folder + "/idle2_minifig.bfnanm") != nullptr, std::string(folder) + " idle2");
        }
    } catch (const std::exception& error) {
        std::cerr << "npc-idle-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "npc-idle-real-test ok\n";
    return 0;
}
