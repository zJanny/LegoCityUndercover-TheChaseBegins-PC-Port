// The entity ids of the object loader 0x1a4130 (runtime::SceneEntityIds) on the shipped levels. Record byte +0xc, decoded by the C++
// level loader (20261021-audio-B, 2026-10-21): in lvl_the_city the eight WorldLevel_* objects have bit 0 (never created; the world levels
// are scenes of their own, docs/re/streaming.md) and Player1 and Bits bit 1 (created only in the scene at 0x589a1c, which is
// lvl_the_city); lvl_01_cherrytreehills gives bit 1 to Player1, Player2 and Bits and to no other object. Needs the extracted RomFS;
// exits 77 without it.
#include "assets/assets.hpp"
#include "runtime/entity_ids.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

chase::assets::Level load(chase::assets::FuseArchive& archive, const std::string& path) {
    const chase::assets::FuseEntry* entry = archive.find(path);
    require(entry != nullptr, path + " missing");
    return chase::assets::parseLevel(archive.read(*entry));
}

void city(chase::assets::FuseArchive& archive, const chase::assets::ClassLookup& classOf) {
    const chase::assets::Level level = load(archive, "levels/levels/lvl_the_city.blvl");
    std::set<std::string> neverCreated, defaultSceneOnly;
    for (const chase::assets::LevelObject& object : level.objects) {
        if (object.createFlags & 1) neverCreated.insert(object.instance);
        if (object.createFlags & 2) defaultSceneOnly.insert(object.instance);
    }
    require(neverCreated.size() == 8 && std::all_of(neverCreated.begin(), neverCreated.end(), [](const std::string& name) { return name.rfind("WorldLevel_", 0) == 0; }),
            "the eight WorldLevel objects of lvl_the_city are never created");
    require(defaultSceneOnly == std::set<std::string>{"Player1", "Bits"}, "Player1 and Bits of lvl_the_city are created only in the scene at 0x589a1c");
    const chase::runtime::SceneEntityIds ids(level, classOf, true);
    require(ids.find("player1").has_value() && !ids.find("worldlevel_01_cherrytreehills").has_value(),
            "in the city scene Player1 has an id, a world level object has none");
}

void cherryTreeHills(chase::assets::FuseArchive& archive, const chase::assets::ClassLookup& classOf) {
    const chase::assets::Level level = load(archive, "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    std::set<std::string> defaultSceneOnly;
    for (const chase::assets::LevelObject& object : level.objects) {
        require(!(object.createFlags & 1), object.instance + " is never created");
        if (object.createFlags & 2) defaultSceneOnly.insert(object.instance);
    }
    require(defaultSceneOnly == std::set<std::string>{"Player1", "Player2", "Bits"}, "lvl_01 gives bit 1 to Player1, Player2 and Bits only");
    const chase::runtime::SceneEntityIds ids(level, classOf, false);
    int previous = 0;
    for (const chase::assets::LevelObject& object : level.objects) {
        const auto id = ids.find(lower(object.instance));
        if (object.createFlags & 2) {
            require(!id, object.instance + " gets no id in a world level");
            continue;
        }
        require(id && int(*id) > previous, object.instance + " has an id above the one of the object before it");
        previous = *id;
    }
    require(ids.find("1-4_ladder01.ladder_part_02") == *ids.find("1-4_ladder01") + 1, "the nested ladder half follows its parent");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: entity-ids-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "entity-ids-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const chase::assets::ClassLookup classOf = chase::assets::archiveClassLookup(archive);
        city(archive, classOf);
        cherryTreeHills(archive, classOf);
    } catch (const std::exception& error) {
        std::cerr << "entity-ids-real-data: " << error.what() << '\n';
        return 1;
    }
    std::cout << "entity-ids-real-data: ok\n";
    return 0;
}
