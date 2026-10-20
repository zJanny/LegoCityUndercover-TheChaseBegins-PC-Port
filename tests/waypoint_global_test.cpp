// The waypoint globals of the system 0x529c18: *(0x529c18 + 0x24) = the entity "Bits.WaypointForCarryIt" (writer 0x2e7484, reader
// 0x230ef4 and the disable branch of 0x229c98). Synthetic part: the disable of that entity, no entity disable for any other
// waypoint, the field is 0 without the entity. With FIB NAMES the real part checks levels/charbits.blvl: object WaypointForCarryIt of
// class props/map_waypoint (probe over every .blvl of the archive, 2026-10-20: the only object whose name contains "carry" and
// "waypoint"; no level script names it). Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/waypoints.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::WaypointSet;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelParam intParam(std::uint32_t hash, std::uint32_t value) {
    assets::LevelParam param;
    param.hash = hash;
    param.flags = 6;
    param.type = 5;
    param.value = value;
    return param;
}

assets::Level manualClass() {
    assets::Level cls;
    cls.classParams = {intParam(0xc93e4625, 0)};
    return cls;
}

assets::Level levelOf(const std::string& instance) {
    assets::Level level;
    assets::LevelObject object;
    object.className = "props/map_waypoint";
    object.instance = instance;
    object.room = "room8";
    level.objects.push_back(object);
    return level;
}

void synthetic() {
    const assets::Level cls = manualClass();
    const assets::ClassLookup classOf = [&](const std::string&) { return &cls; };
    WaypointSet set;
    set.load(levelOf("Wp_A"), classOf);
    require(!set.isCarryItWaypoint("bits.waypointforcarryit") && set.waypoints().size() == 1, "before 0x2e7484 the field is 0");
    set.loadCarryItWaypoint(levelOf("Debris01"), classOf);
    require(!set.isCarryItWaypoint("bits.waypointforcarryit") && set.waypoints().size() == 1, "0x23b350 finds no such entity: the field stays 0");
    set.loadCarryItWaypoint(levelOf("WaypointForCarryIt"), classOf);
    require(set.isCarryItWaypoint("bits.waypointforcarryit") && !set.isCarryItWaypoint("wp_a") && !set.isCarryItWaypoint(""),
            "0x230ef4 is the entity named Bits.WaypointForCarryIt");
    const runtime::Waypoint* carry = set.find("bits.waypointforcarryit");
    require(carry && !carry->entityEnabled && !carry->active && carry->mapSlot == -1, "0x2e75cc disables the entity at the start");

    set.setActive("bits.waypointforcarryit", true);
    require(carry->entityEnabled && carry->active && carry->mapSlot == 0, "an enable clears the disabled flag and takes a map slot");
    require(!set.setActive("bits.waypointforcarryit", false), "AutoTrigger 0: the disable is accepted");
    require(!carry->entityEnabled && carry->mapSlot == -1 && carry->disableFade, "0x229d70: the disable of the carry-it waypoint runs 0x2428c0");

    set.setActive("wp_a", true);
    set.setActive("wp_a", false);
    require(set.find("wp_a")->entityEnabled, "any other waypoint keeps its entity enabled on a disable");

    set.load(levelOf("Wp_A"), classOf);
    require(!set.isCarryItWaypoint("bits.waypointforcarryit") && set.waypoints().size() == 1, "a level load resets the field (0x2e75f4)");
}

void real(const std::string& fib, const std::string& names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::Level bits = assets::parseLevel(archive.read(*archive.find("levels/charbits.blvl")));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    WaypointSet set;
    set.loadCarryItWaypoint(bits, classOf);
    require(set.isCarryItWaypoint("bits.waypointforcarryit"), "levels/charbits.blvl holds WaypointForCarryIt of props/map_waypoint");
    const runtime::Waypoint* carry = set.find("bits.waypointforcarryit");
    require(carry && !carry->entityEnabled, "the entity starts disabled");
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        if (argc == 1) {
            std::cout << "waypoint-global-test: synthetic part passed\n";
            return 0;
        }
        if (argc != 3) {
            std::cerr << "usage: waypoint-global-test [FIB NAMES]\n";
            return 2;
        }
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "waypoint-global-test: data not available, skipped\n";
            return 77;
        }
        real(argv[1], argv[2]);
        std::cout << "waypoint-global-test: passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "waypoint-global-test: " << error.what() << '\n';
        return 1;
    }
}
