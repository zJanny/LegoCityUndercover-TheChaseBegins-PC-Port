// Objective natives of batch SCRIPT-31 (src/runtime/objectives.cpp, docs/re/objective-hud.md). Values come from the
// disassembly: SetActiveWayPoint 0x27bde8 (entity, enable) -> 0x26e168 -> 0x229c98; SubObjectiveGoToEnable 0x2adaac ->
// 0x2a62b8 with the registry 0x1fcc68 / 0x1fce14 (record type 0, 32 targets of 4 bytes, count at +0xa0, remove by moving
// the last entry down); SubObjectiveEnemyEnable 0x2b4c3c is `mov r0, #1; bx lr`. With no argument only the synthetic part
// runs; with FIB NAMES the real part runs 1-1_MissionStart of lvl_01 (it switches 1-1_Waypoint_01 on) and exits with 77
// when the data is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(assets::Level& level, const std::string& name, std::vector<assets::ScriptCall> calls) {
    assets::LevelScript script;
    script.nameHash = assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void synthetic() {
    assets::Level level;
    add(level, "s", {{"SetActiveWayPoint", {"Wp_A", "1"}},
                     {"SetActiveWayPoint", {"Wp_B", "1"}},
                     {"SubObjectiveGoToEnable", {"Wp_A", "Car", "1"}},
                     {"SubObjectiveGoToEnable", {"Wp_A", "Van", "1"}},
                     {"SubObjectiveGoToEnable", {"Wp_A", "Bike", "1"}},
                     {"SubObjectiveGoToEnable", {"Wp_A", "Van", "1"}},
                     {"SubObjectiveEnemyEnable", {"Wp_A", "Enemy", "1"}},
                     {"EnableMinimapWipe", {"1"}},
                     {"PanMapToObject", {"Wp_B", "2", "2", "2"}}});
    add(level, "off", {{"SubObjectiveGoToEnable", {"Wp_A", "Car", "0"}}, {"SetActiveWayPoint", {"Wp_A", "0"}}});
    assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("s"), "start");
    const runtime::ObjectiveState& objectives = runner.objectives();
    require(objectives.waypointActive("wp_a") && objectives.waypointActive("wp_b"), "both waypoints on, names in lower case");
    require(objectives.activeWaypoints().size() == 2 && objectives.activeWaypoints()[0] == "wp_a", "activation order");
    const runtime::SubObjectiveGroup* group = objectives.group("wp_a", 0);
    require(group && group->targets.size() == 3, "a duplicate target is not added twice (0x1fce14 scans the list)");
    require(group->targets[0] == "car" && group->targets[1] == "van" && group->targets[2] == "bike", "targets in order of enabling");
    require(objectives.groups().size() == 1, "SubObjectiveEnemyEnable (no-op 0x2b4c3c) makes no record");
    require(objectives.minimapWipe() && objectives.recordedCalls(runtime::ObjectiveNativeKind::EnableMinimapWipe) == 1, "stand-in records");
    require(objectives.panMapObject() == "wp_b", "PanMapToObject records its object");
    require(runner.start("off"), "start off");
    group = objectives.group("wp_a", 0);
    require(group && group->targets.size() == 2 && group->targets[0] == "bike" && group->targets[1] == "van",
            "removing the first entry moves the last one into its place (0x2a62b8)");
    require(!objectives.waypointActive("wp_a") && objectives.waypointActive("wp_b"), "waypoint off");

    require(runtime::scriptFunctionSupport("SetActiveWayPoint") == runtime::ScriptFunctionSupport::StandIn, "SetActiveWayPoint keeps stand-in details (SCRIPT-S269)");
    for (const char* name : {"SubObjectiveGoToEnable", "SubObjectiveEnemyEnable", "SubObjectiveArrowGoToEnable", "SubObjectiveArrowEnemyEnable"})
        require(runtime::scriptFunctionSupport(name) == runtime::ScriptFunctionSupport::Implemented, std::string("transcribed ") + name);
    for (const char* name : {"EnableMinimapWipe", "EnableScanner", "OverrideScannable",
                             "PanMapToObject", "SetMapDummyLocObj", "ClearMapDummyLocObj", "EnableScannerButton", "BlackoutMap",
                             "RemoveScannerMapMarker", "DisableIcon"})
        require(runtime::scriptFunctionSupport(name) == runtime::ScriptFunctionSupport::StandIn, std::string("stand-in ") + name);

    assets::Level full;
    std::vector<assets::ScriptCall> many;
    for (int i = 0; i < 40; ++i) many.push_back({"SubObjectiveGoToEnable", {"w", "t" + std::to_string(i), "1"}});
    add(full, "many", std::move(many));
    assets::PlacedObjectState fullState;
    ScriptRunner fullRunner(full, fullState);
    fullRunner.start("many");
    require(fullRunner.objectives().group("w", 0)->targets.size() == 32, "a record holds 32 targets");
}

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level missing");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
    ScriptRunner runner(level, state);
    require(runner.start("1-1_MissionStart"), "script");
    require(runner.objectives().waypointActive("1-1_waypoint_01"), "1-1_MissionStart switches 1-1_Waypoint_01 on");
    require(assets::objectStart(level, "1-1_Waypoint_01").has_value(), "the waypoint is a placed object");
    require(runner.start("1-1_Release_Car_Script"), "release script");
    for (int update = 0; update < 600 && !runner.objectives().waypointActive("1-1_waypoint_03"); ++update) runner.update(1.0f / 30);
    require(runner.objectives().waypointActive("1-1_waypoint_03"), "1-1_Release_Car_Script switches 1-1_Waypoint_03 on");
    require(!runner.unhandledCalls("setactivewaypoint") && !runner.unhandledCalls("subobjectivegotoenable"), "handled");
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            synthetic();
            return 0;
        }
        if (argc != 3) {
            std::cerr << "usage: objectives-test [FIB NAMES]\n";
            return 2;
        }
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "objectives-test: data not available, skipped\n";
            return 77;
        }
        realData(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "objectives-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
