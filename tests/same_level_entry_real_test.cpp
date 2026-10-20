// Entries inside a loaded level (docs/re/level-transitions.md): the lift objects of lvl_00 and the master objects whose
// destination is the level they are bound in. Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/same_level_entry.hpp"
#include "runtime/script_runner.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::Level load(assets::FuseArchive& archive, const std::string& path) {
    const assets::FuseEntry* entry = archive.find(path);
    require(entry != nullptr, "not in archive: " + path);
    return assets::parseLevel(archive.read(*entry));
}

void flowOrder() {
    runtime::SameLevelEntry entry{"lift", "bound", 1};
    runtime::SameLevelEntryFlow flow;
    require(!flow.pending() && !flow.takeEntry(), "idle at the start");
    require(flow.request(entry) && flow.pending(), "the request is pending");
    require(!flow.request(entry), "a second request while one is pending is ignored");
    require(!flow.takeEntry(), "not due before an update");
    for (unsigned i = 0; i < runtime::kSameLevelEntryDelayUpdates; ++i) flow.update();
    const auto due = flow.takeEntry();
    require(due && due->instance == "lift" && !flow.pending(), "due after the delay, request cleared");
}

const assets::LevelBound* boundNamed(const assets::Level& level, const std::string& name) {
    for (const assets::LevelBound& bound : level.bounds)
        if (bound.name == name) return &bound;
    return nullptr;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: same-level-entry-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "same-level-entry-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        flowOrder();
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level master = load(archive, runtime::kMasterLevelFile);
        const std::string stationFile = "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl";
        const assets::Level station = load(archive, stationFile);
        const runtime::WorldLevelEntry stationLevel = runtime::worldLevelOfFile(master, stationFile);
        const std::vector<runtime::SceneChangeObject> objects = runtime::sceneChangeObjects(master);

        // Provenance: tools/convert_levels.py on lvl_00_policestation_main.blvl (2026-10-20): four scenechange objects,
        // SceneChangeTo records {world level hash 0, point hash, 0, flags 2, scene index 0}.
        const std::vector<runtime::SameLevelEntry> entries = runtime::sameLevelEntries(station, objects, stationLevel.instance);
        require(entries.size() == 4, "four same-level entries in the police station");
        struct Expected {
            const char *bound, *instance, *point;
        };
        const Expected expected[] = {{"Bound_Basement2Lineup", "Basement2Lineup", "Lineup_Dest_Basement_P1"},
                                     {"Basement2Lobby_SceneChange", "Basement2Lobby", "Lobby_Dest_Basement_P1"},
                                     {"Lineup_to_Basement", "Lineup2Basement", "Basement_Dest_Lineup_P1"},
                                     {"Lobby2Basement_SceneChange", "Lobby2Basement", "Basement_Dest_Lobby_P1"}};
        for (const Expected& want : expected) {
            const runtime::SameLevelEntry* entry = runtime::findSameLevelEntry(entries, want.bound);
            require(entry && entry->instance == want.instance, std::string("entry for ") + want.bound);
            const auto point = runtime::destinationPoint(station, {{}, entry->pointHash});
            require(point && point->instance == want.point, std::string("destination point ") + want.point);
        }

        // Master objects bound in a world level and leading into the same one (the lobby and street links are not).
        const std::vector<runtime::SameLevelEntry> downtown = runtime::masterSameLevelEntries(objects, "WorldLevel_07_Downtown");
        require(downtown.size() == 4, "BankEx, BankIn, DowntownToTruckChase and TruckChaseToDowntown stay in Downtown");
        require(runtime::masterSameLevelEntries(objects, stationLevel.instance).empty(), "no master object of the station stays in it");
        require(runtime::masterSameLevelEntries(objects, "WorldLevel_04_FortMeadows").size() == 2, "the shack links stay in Fort Meadows");

        // Walk-in at the lift bound: startup disables Lobby2Basement_SceneChange (docs/re/scene-start.md), the lift script
        // LIFT_basement enables it while Lobby2Basement_Lift_Leave is 1 (script dump of lvl_00, 2026-10-20). The positions
        // are the centre of the bound in the level data (20.572, 2.5452, 14.8266, node 1) and a point 5 m beside it; the landing is the point Basement_Dest_Lobby_P1 of the level data.
        const assets::LevelBound* lift = boundNamed(station, "Lobby2Basement_SceneChange");
        require(lift != nullptr && lift->room == "room4", "bound Lobby2Basement_SceneChange names room4, which lvl_00 does not have (rooms 1, 3, 5)");
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        assets::PlacedObjectState state = runtime::levelInitialState(station, classOf);
        runtime::ScriptRunner runner(station, state);
        runner.setSceneRooms({"Room1", "Room3", "Room5"});
        runner.setRoomLoaded("Room1", true);
        require(runner.start("startup"), "lvl_00 has the startup script");
        runner.update(1.0f / 60.0f);
        const float inside[3] = {lift->centre[0], lift->centre[1], lift->centre[2]};
        const float outside[3] = {lift->centre[0] + 5.0f, lift->centre[1], lift->centre[2] + 5.0f};
        require(!runner.playerInNamedBound("Lobby2Basement_SceneChange", inside), "the lift bound is disabled after startup");
        runner.setVariable(assets::nameHash("Lobby2Basement_Lift_Leave"), 1.0f);
        require(runner.start("LIFT_basement"), "lvl_00 has the script LIFT_basement");
        runner.update(1.0f / 60.0f);
        require(runner.playerInNamedBound("Lobby2Basement_SceneChange", inside), "the lift script enables the bound");
        require(!runner.playerInNamedBound("Lobby2Basement_SceneChange", outside), "outside the lift bound");

        const runtime::SameLevelEntry* entry = runtime::findSameLevelEntry(entries, "Lobby2Basement_SceneChange");
        const auto landing = runtime::destinationPoint(station, {{}, entry->pointHash});
        require(landing && landing->room == "room3", "the lobby lift lands in the basement room");
        require(std::fabs(landing->position[0] + 299.37f) < 0.001f && std::fabs(landing->position[1] - 5.7328f) < 0.001f &&
                    std::fabs(landing->position[2] - 6.0847f) < 0.001f && std::fabs(landing->orientation[1] - 43.604f) < 0.001f,
                "Basement_Dest_Lobby_P1 position and yaw");
        std::cout << "same-level-entry-real-test: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "same-level-entry-real-test: " << error.what() << '\n';
        return 1;
    }
}
