// Level switches between the police station and the street (docs/re/streaming.md, the scene change entries): the
// scenechange objects of the master level, their destination points, the walk-in bound of the lobby exit and the
// request order of the manager. Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/scene_change.hpp"
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

bool near(float a, float b, float tolerance = 0.0005f) { return std::fabs(a - b) <= tolerance; }

assets::Level load(assets::FuseArchive& archive, const std::string& path) {
    const assets::FuseEntry* entry = archive.find(path);
    require(entry != nullptr, "not in archive: " + path);
    return assets::parseLevel(archive.read(*entry));
}

void flowOrder() {
    runtime::SceneChangeDestination destination;
    destination.worldLevel.instance = "WorldLevel_01_CherryTreeHills";
    runtime::SceneChangeFlow flow;
    require(!flow.pending() && !flow.pageOpen(), "idle at the start");
    require(flow.request(destination) && flow.pending() && flow.pageOpen(), "the request opens the loading page");
    require(!flow.request(destination), "a second request while one is pending is ignored");
    for (int i = 0; i < 30; ++i) {
        flow.update(false);
        require(flow.pageOpen() && !flow.takeSwitch(), "the page waits for A");
    }
    flow.update(true);
    require(!flow.pageOpen() && flow.pending(), "A closes the page, the switch is still pending");
    for (unsigned i = 0; i < runtime::kSceneChangeSwitchDelayUpdates; ++i) {
        require(!flow.takeSwitch(), "no switch before the delay");
        flow.update(false);
    }
    const auto due = flow.takeSwitch();
    require(due && due->worldLevel.instance == destination.worldLevel.instance, "the switch follows 4 updates after the press");
    require(!flow.pending(), "the request is cleared");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: scene-change-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "scene-change-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        flowOrder();
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level master = load(archive, runtime::kMasterLevelFile);
        const std::vector<runtime::SceneChangeObject> objects = runtime::sceneChangeObjects(master);
        // Provenance: streaming.md 2026-10-13T22:27 (35 scenechange objects, 8 world levels in lvl_the_city).
        require(objects.size() == 35, "35 scenechange objects in the master level");
        require(runtime::worldLevels(master).size() == 8, "8 world levels");

        const std::string stationFile = "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl";
        const std::string streetFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
        require(runtime::worldLevelOfFile(master, streetFile).instance == "WorldLevel_01_CherryTreeHills", "street instance name");
        require(runtime::worldLevelOfFile(master, stationFile).instance == "WorldLevel_00_PoliceStation", "station instance name");

        // Exit: oracle save state in the police station lobby in front of the exit / the save state at the loading page of the police station exit (agent H, 2026-10-14): walking into the bound
        // Lobby_to_CherryTree fired SceneChange_LobbyToCherryTree; after the A press on the loading page the player
        // stood at (902.00848, 16.57730, 1111.12634) (trace sw_walk frame 13), forward (-0.935, 0, -0.354) = yaw 249.25.
        const runtime::SceneChangeObject* exit = runtime::findSceneChange(objects, "WorldLevel_00_PoliceStation", "Lobby_to_CherryTree");
        require(exit && exit->instance == "SceneChange_LobbyToCherryTree" && exit->destination, "exit object");
        require(exit->destination->worldLevel.levelFile == streetFile, "exit leads to the street");
        const auto exitPoint = runtime::destinationPoint(load(archive, streetFile), *exit->destination);
        require(exitPoint.has_value(), "exit point LobbyToCherryTree_P1");
        require(exitPoint->instance == "LobbyToCherryTree_P1", "exit point name");
        require(near(exitPoint->position[0], 902.00848f) && near(exitPoint->position[1], 16.57730f) && near(exitPoint->position[2], 1111.12634f),
                "street landing position");
        require(near(exitPoint->orientation[1], 249.2519f, 0.001f), "street landing yaw");

        // Entry: oracle save state at the police station entrance after mission 1-1 (trace ent_d frame 139, first update after the switch): the player
        // stood at (-1.11520, 2.09010, 15.70230) and then fell to y 1.43053; forward (0.465, 0, -0.886) = yaw 152.32.
        const runtime::SceneChangeObject* entry = runtime::findSceneChange(objects, "WorldLevel_01_CherryTreeHills", "Police_ExtToInt");
        require(entry && entry->instance == "SceneChange_Police_ExtToInt" && entry->destination, "entry object");
        require(entry->destination->worldLevel.levelFile == stationFile, "entry leads to the station");
        const auto entryPoint = runtime::destinationPoint(load(archive, stationFile), *entry->destination);
        require(entryPoint.has_value() && entryPoint->instance == "Lobby_Dest_CherryTree_P1", "entry point name");
        require(near(entryPoint->position[0], -1.11520f) && near(entryPoint->position[1], 2.09010f) && near(entryPoint->position[2], 15.70230f),
                "lobby landing position");
        require(near(entryPoint->orientation[1], 152.3217f, 0.001f), "lobby landing yaw");

        // Walk-in: trace exit_walk of save state in the police station lobby after mission 1-1 with the stick held: x -1.97041, y 1.42379; the update-12 row
        // z 19.67306 is outside the bound (z 20.1114 to 23.3938), the update-13 row z 20.15606 is inside and the request
        // fired in that update (trap run exit_trap1: 0x3037e8 at frame 13). The start position is outside.
        const assets::Level station = load(archive, stationFile);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        assets::PlacedObjectState state = runtime::levelInitialState(station, classOf);
        runtime::ScriptRunner runner(station, state);
        runner.setAllRoomsLoaded();
        const float start[3] = {-1.97041f, 1.43587f, 14.843f}, before[3] = {-1.97041f, 1.42379f, 19.67306f}, inside[3] = {-1.97041f, 1.42379f, 20.15606f};
        require(!runner.playerInNamedBound("Lobby_to_CherryTree", start), "start outside the exit bound");
        require(!runner.playerInNamedBound("Lobby_to_CherryTree", before), "update 12 outside the exit bound");
        require(runner.playerInNamedBound("Lobby_to_CherryTree", inside), "update 13 inside the exit bound");
        std::cout << "scene-change-real-test: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "scene-change-real-test: " << error.what() << '\n';
        return 1;
    }
}
