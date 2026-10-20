// Pedestrians_Max and the Pedestrian%02u slots of the shipped levels, read as the level load 0x2ee4c8 reads them (0x23b350 by name
// hash, 0x2451d0 on the scene's level root). Provenance: the parameter records of the level tables, found by their name hash
// 0x7ecd8507 in the .blvl files (lvl_01 at file offset 0x4d3b8: flags 1, type 0, 5.0; lvl_07 at 0x52424: 10.0; lvl_04 has none);
// the slots Pedestrian02 .. Pedestrian15 of lvl_01 from `tools/chase_native.py level_objects`. Needs the extracted RomFS; exits
// with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/pedestrian_manager.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::Level levelOf(assets::FuseArchive& archive, const std::string& name) {
    const std::string path = "levels/levels/" + name + "/" + name + "_main.blvl";
    const auto entry = archive.find(path);
    if (!entry) throw std::runtime_error("missing " + path);
    return assets::parseLevel(archive.read(*entry));
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: pedestrian-limit-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "pedestrian-limit-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level cherryTreeHills = levelOf(archive, "lvl_01_cherrytreehills");
        require(runtime::pedestriansMaxOf(cherryTreeHills) == 5, "lvl_01 Pedestrians_Max 5.0");
        require(runtime::pedestriansMaxOf(levelOf(archive, "lvl_07_downtown")) == 10, "lvl_07 Pedestrians_Max 10.0");
        require(runtime::pedestriansMaxOf(levelOf(archive, "lvl_04_fortmeadows")) == 5, "lvl_04 has none: default 5");

        runtime::PedestrianManager manager;
        manager.init();
        const auto numbers = manager.loadLevel(
            [&](const std::string& name) {
                const std::uint32_t hash = assets::nameHash(name);
                for (const assets::LevelObject& object : cherryTreeHills.objects)
                    if (assets::nameHash(object.instance) == hash) return true;
                return false;
            },
            cherryTreeHills);
        require(numbers.size() == 14 && numbers.front() == 2 && numbers.back() == 15, "lvl_01 slots Pedestrian02 .. Pedestrian15");
        require(manager.limit() == 5, "lvl_01 limit");
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-limit-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pedestrian-limit-real-test ok\n";
    return 0;
}
