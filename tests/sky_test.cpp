// Sky of the original (src/runtime/sky.cpp, docs/re/rendering-sky.md). Core: the lift of the sky camera, from the constants
// 120.0 (0x1813a0) and 20.0 (0x1813a4) and the interpolation 0x22a048 read in game.arm.elf. With two arguments (FIB NAMES) the
// sky models and the level objects that place them are checked too; exits with 77 (skipped) when the data is missing.
#include "assets/assets.hpp"
#include "runtime/sky.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;
namespace assets = chase::assets;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

void core() {
    require(near(skyLift(0, 0), 0), "lift at sea level");
    require(near(skyLift(60, 0), 10), "lift at 60");
    require(near(skyLift(120, 0), 20), "lift at 120");
    require(near(skyLift(500, 0), 20), "lift is capped at 20");
    require(near(skyLift(-5, 0), 0), "lift is not negative");
    require(near(skyLift(16.567f, 0), 20.0f * 16.567f / 120.0f), "lift on the lvl_01 street");
    require(near(skyLift(130, 10), 20), "lift is relative to the Sea object");
    const float eye[3] = {900, 20, 1110};
    const auto offset = skyTranslation(eye, 5);
    require(near(offset[0], 900) && near(offset[1], 15) && near(offset[2], 1110), "sky translation");
}

// Recorded with chase-tabledump-style probes over the archive (manual inspection, 2026-10-15): every level that has a sky places
// one class props/prop_skybox object (lvl_06 two); lvl_00 has none.
struct LevelSky {
    const char* level;
    const char* firstMesh;
    int count;
};

void realData(const std::string& fib, const std::string& names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const LevelSky levels[] = {
        {"levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl", "", 0},
        {"levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", "SkyBox_Skyline_Cherry", 1},
        {"levels/levels/lvl_02_albatrossisland/lvl_02_albatrossisland_main.blvl", "SkyBox_Skyline_Albatross", 1},
        {"levels/levels/lvl_06_apolloisland/lvl_06_apolloisland_main.blvl", "SkyBox_Skyline_Apollo", 2},
    };
    for (const LevelSky& expected : levels) {
        const assets::FuseEntry* entry = archive.find(expected.level);
        require(entry != nullptr, std::string("missing ") + expected.level);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const auto classOf = assets::archiveClassLookup(archive);
        int count = 0;
        std::string first;
        for (const assets::Placement& placement : assets::objectPlacements(level, classOf))
            if (placement.className == "props/prop_skybox") {
                if (count++ == 0) first = placement.mesh;
            }
        require(count == expected.count, std::string("sky objects in ") + expected.level);
        require(first == expected.firstMesh, std::string("first sky mesh in ") + expected.level);
    }
    const assets::FuseEntry* model = archive.find("models/skybox_skyline_cherry/skybox_skyline_cherry.bfnmdl");
    require(model != nullptr, "missing sky model");
    const assets::Model sky = assets::parseModel(archive.read(*model));
    require(sky.meshes.size() == 1 && sky.meshes[0].parts.size() == 1, "sky model has one mesh with one part");
    const assets::MaterialState material = assets::materialState(sky.meshes[0].parts[0]);
    require(material.supported && !material.fogged && !material.lighting, "sky material is unfogged and unlit");
    require(std::fabs(sky.meshes[0].half[0] - 99.9435f) < 1e-3f, "sky dome radius 99.9435");
}
}

int main(int argc, char** argv) {
    try {
        core();
        if (argc >= 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
            realData(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "sky-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
