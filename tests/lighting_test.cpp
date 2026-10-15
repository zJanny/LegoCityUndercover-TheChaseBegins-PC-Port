// Fragment lighting of the original (src/runtime/lighting.cpp, docs/re/rendering-lighting.md): light mask table, lookup tables,
// register values of 0x12d884 against the oracle command list, and the light objects of lvl_01 and lvl_00. With two arguments
// (FIB NAMES) the level data is checked too; exits with 77 (skipped) when the data is missing.
#include "assets/assets.hpp"
#include "runtime/lighting.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t packed(const std::array<std::uint8_t, 3>& c) { return std::uint32_t(c[2]) | std::uint32_t(c[1]) << 10 | std::uint32_t(c[0]) << 20; }

void maskTable() {
    // Table 0x3798f8 read from artifacts/game/game.arm.elf (llvm-objdump; pointer at 0x129a44).
    const unsigned expected[16] = {0, 4, 0xf, 0, 4, 0xf, 0xf, 0xf, 0, 0xf, 0, 0, 0, 0, 0, 0};
    for (unsigned key = 0; key < 16; ++key) require(lightMask(key) == expected[key], "mask of lighting key " + std::to_string(key));
    require(lightMask(16) == 0, "mask beyond the table");
}

void tables() {
    // Hand evaluation of 0x24cae0 (float32 numpy): D0 pow(x, 20), DA 1 - x, SP 0.5 + 0.5 x over two signed halves.
    const LightLut d0 = distributionLut(), da = distanceLut(), sp = spotLut();
    require(d0[0] == 0 && d0[128] == 0 && d0[200] == 0x101f && d0[240] == 0x344c2 && d0[254] == 0x9aeca && d0[255] == 0xa6fff, "D0 table");
    require(da[0] == 0xfff && da[1] == 0xfef && da[128] == 0x7f7 && da[254] == 0x10 && da[255] == 0, "DA table");
    require(sp[0] == 0x8800 && sp[64] == 0x8c08 && sp[127] == 0x8fff && sp[128] == 0x8000 && sp[255] == 0x8800, "SP table");
}

void registers() {
    // Oracle gameplay-start, command list at 0x140037e0 (agentLIGHT cmdbuf1): the lvl_01 lights as the game holds them
    // (light objects 0x08e0b088 and 0x08e0b380) and a material with +0x10 and +0x14 = ffffffff, lighting key 2.
    LightSet set;
    set.globalAmbient = {136.0f / 255.0f, 159.0f / 255.0f, 179.0f / 255.0f};
    Light sun;
    sun.type = 1;
    sun.colour = {254.0f / 255.0f, 209.0f / 255.0f, 169.0f / 255.0f};
    sun.specular = {127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f};
    sun.vector = {-0.6508f, 0.6242f, -0.4322f};
    set.list.push_back(sun);
    const std::uint8_t white[4] = {255, 255, 255, 255};
    const FragmentLighting lit = fragmentLighting(set, 2, white, white, false);
    require(lit.enabled && packed(lit.globalAmbient) == 0x08827cb3, "register 0x1c0");
    require(lit.slots.size() == 1, "one light");
    require(packed(lit.slots[0].diffuse) == 0x0fe344a9, "register 0x142");
    require(packed(lit.slots[0].ambient) == 0, "register 0x143");
    require(packed(lit.slots[0].specular) == 0x07f1fc7f, "register 0x140");
    require(lit.slots[0].directional && !lit.slots[0].spotAttenuation && !lit.slots[0].distanceAttenuation, "register 0x149 = 1");
    // Same list, lighting key 1 (mask 4): the command list writes 0x1c0 = 0 and no light.
    const FragmentLighting baked = fragmentLighting(set, 1, white, white, false);
    require(baked.enabled && packed(baked.globalAmbient) == 0 && baked.slots.empty(), "lighting key 1");
    require(!fragmentLighting(set, 0, white, white, false).enabled, "lighting key 0");
    // Kind 2 (0x12dca0, hand evaluation): ambient, not diffuse; halved with byte 0xb bit 5; spot on; scale 1 / MaxRange.
    Light point;
    point.type = 2;
    point.colour = {1, 1, 1};
    point.specular = {1, 1, 1};
    point.intensity = 2;
    point.maxRange = 4;
    set.list = {point};
    const FragmentLighting near = fragmentLighting(set, 1, white, white, true);
    require(near.slots.size() == 1 && packed(near.slots[0].diffuse) == 0, "kind 2 diffuse");
    require(near.slots[0].ambient[0] == 127 && near.slots[0].specular[2] == 127, "kind 2 halved");
    require(near.slots[0].twoSided && near.slots[0].spotAttenuation && near.slots[0].distanceAttenuation, "kind 2 config");
    require(near.slots[0].distanceScale == 0.25f, "kind 2 distance scale");
}

const chase::assets::LightObject* named(const std::vector<chase::assets::LightObject>& lights, const std::string& name) {
    for (const auto& light : lights)
        if (light.instance == name) return &light;
    return nullptr;
}

int realData(const std::string& fibPath, const std::string& namesPath) {
    if (!std::filesystem::exists(fibPath) || !std::filesystem::exists(namesPath)) return 77;
    chase::assets::FuseArchive archive(fibPath);
    archive.loadNames(namesPath);
    const chase::assets::ClassLookup classOf = chase::assets::archiveClassLookup(archive);
    const auto load = [&](const std::string& path) {
        const chase::assets::FuseEntry* entry = archive.find(path);
        require(entry != nullptr, "missing " + path);
        return chase::assets::parseLevel(archive.read(*entry));
    };
    // lvl_01: tools/convert_levels.py (instance values) and the class files levels/lighting/*.blvl.
    const auto street = chase::assets::lightObjects(load("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl"), classOf);
    require(street.size() == 3, "lvl_01 has 3 light objects");
    const auto* ambient = named(street, "Ambient");
    const auto* sun = named(street, "Directional01");
    const auto* specular = named(street, "psp_specular");
    require(ambient && ambient->type == 0 && ambient->colour[2] == 179.0f, "lvl_01 ambient");
    require(sun && sun->type == 1 && sun->colour[0] == 254.0f && sun->specular[0] == 127.0f && sun->intensity == 1.0f, "lvl_01 sun");
    require(specular && specular->type == 4, "lvl_01 specular light");
    const LightSet set = levelLights(street, 0);
    // Oracle gameplay-start: one list entry (0x5943b0), direction +0x44 = (-0.6508, 0.6242, -0.4322), global ambient 0x1c0 = 0x08827cb3.
    require(set.list.size() == 1, "lvl_01 light list");
    require(std::fabs(set.list[0].vector[0] + 0.6508f) < 1e-4f && std::fabs(set.list[0].vector[1] - 0.6242f) < 1e-4f &&
                std::fabs(set.list[0].vector[2] + 0.4322f) < 1e-4f,
            "lvl_01 sun direction");
    const std::uint8_t white[4] = {255, 255, 255, 255};
    const FragmentLighting lit = fragmentLighting(set, 2, white, white, false);
    require(packed(lit.globalAmbient) == 0x08827cb3 && packed(lit.slots.at(0).diffuse) == 0x0fe344a9 &&
                packed(lit.slots.at(0).specular) == 0x07f1fc7f,
            "lvl_01 registers");
    // lvl_00: tools/convert_levels.py; Point and Point01 set Intensity 2 and MaxRange 1 (class lightpoint).
    const auto station = chase::assets::lightObjects(load("levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl"), classOf);
    require(station.size() == 9, "lvl_00 has 9 light objects");
    const auto* point = named(station, "Point");
    require(point && point->type == 2 && point->intensity == 2.0f && point->maxRange == 1.0f && point->specular[0] == 255.0f, "lvl_00 point");
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 3) return realData(argv[1], argv[2]);
        maskTable();
        tables();
        registers();
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "lighting tests passed\n";
    return 0;
}
