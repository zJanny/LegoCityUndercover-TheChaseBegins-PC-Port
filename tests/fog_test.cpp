// Fog of the original (src/runtime/fog.cpp, docs/re/rendering-fog.md): the 128-entry table of 0x24e9d8 against captures from the
// Azahar oracle, and the level parameters of lvl_01 and lvl_00. With two arguments (FIB NAMES) the level data is checked too;
// exits with 77 (skipped) when the data is missing.
#include "assets/assets.hpp"
#include "runtime/fog.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// Oracle capture at breakpoint 0x24ed98 (street-after-1-1, floats 0x376160/0x376164 set to 20 and 120, cache 0x37616c cleared; r8 -> 128 words).
const FogLut kRecordedB = {{
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffffba, 0xf75f95, 0xea1f58, 0xd51ed2, 0xaf5d3d, 0x571d47,
}};

// Oracle capture at breakpoint 0x24ed98 (street-after-1-1, floats 0x376160/0x376164 set to 60 and 200, cache 0x37616c cleared; r8 -> 128 words).
const FogLut kRecordedC = {{
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000,
    0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xffe000, 0xfffe83, 0xd0797c,
}};

// Oracle capture at 0x24ed98 at street-after-1-1 (start 220, end 260 as the level sets them): entries 0..126 are 1.0, entry 127 falls to 0.
FogLut recordedStreet() {
    FogLut lut;
    lut.fill(0xffe000);
    lut[127] = 0xfff800;
    return lut;
}

void requireLut(const FogLut& built, const FogLut& recorded, const std::string& name) {
    for (std::size_t i = 0; i < kFogLutEntries; ++i)
        require(built[i] == recorded[i], name + " entry " + std::to_string(i));
}

void coreChecks() {
    requireLut(buildFogLut(220.0f, 260.0f), recordedStreet(), "street 220 260");
    requireLut(buildFogLut(20.0f, 120.0f), kRecordedB, "20 120");
    requireLut(buildFogLut(60.0f, 200.0f), kRecordedC, "60 200");

    std::vector<chase::assets::FogObject> objects(3);
    objects[0].instance = "Global";
    objects[0].room = "Objects";
    objects[0].start = 20, objects[0].end = 150, objects[0].colour = {1, 2, 3};
    objects[1].instance = "Room5";
    objects[1].room = "room5";
    objects[1].start = 10, objects[1].end = 60, objects[1].colour = {4, 5, 6};
    objects[2].instance = "Broken";
    objects[2].room = "Objects";
    objects[2].start = 50, objects[2].end = 50, objects[2].colour = {7, 8, 9};
    FogSetting fog = levelFog(objects, 3);
    require(fog.enabled && fog.start == 20 && fog.end == 150 && fog.colour[0] == 1, "global object applies, invalid one is ignored");
    fog = levelFog(objects, 5);
    require(fog.enabled && fog.start == 10 && fog.end == 60 && fog.colour[2] == 6, "room object replaces the global one");
    require(!levelFog({}, 1).enabled, "no object: fog off");
}

void levelChecks(const char* fib, const char* names) {
    chase::assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const chase::assets::ClassLookup classOf = chase::assets::archiveClassLookup(archive);
    auto fogOf = [&](const char* path) {
        const chase::assets::FuseEntry* entry = archive.find(path);
        require(entry != nullptr, std::string("missing ") + path);
        return chase::assets::fogObjects(chase::assets::parseLevel(archive.read(*entry)), classOf);
    };
    const auto street = fogOf("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(street.size() == 1, "lvl_01 has one fog object");
    require(street[0].start == 220.0f && street[0].end == 260.0f, "lvl_01 range 220 260 (oracle 0x376160 / 0x376164)");
    require(street[0].colour[0] == 183 && street[0].colour[1] == 199 && street[0].colour[2] == 221, "lvl_01 colour (oracle 0x376168)");
    const FogSetting setting = levelFog(street, 7);
    require(setting.enabled && setting.start == 220.0f, "street fog applies in Room7");

    const auto station = fogOf("levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl");
    require(station.size() == 3, "lvl_00 has three fog objects");
    bool basement = false;
    for (const chase::assets::FogObject& object : station)
        if (object.instance == "Ambient_Basement") basement = object.room == "room3" && object.start == 20 && object.end == 100 && object.colour[0] == 158;
    require(basement, "Ambient_Basement: room3, 20, 100, (158, 154, 122)");
}
}

int main(int argc, char** argv) {
    try {
        coreChecks();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "fog-test: data not available, skipped\n";
                return 77;
            }
            levelChecks(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "fog-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "fog-test ok\n";
    return 0;
}
