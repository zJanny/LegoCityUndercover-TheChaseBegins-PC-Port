// Level start state of lvl_01_cherrytreehills (src/runtime/level_start.cpp) against what the original game had at
// gameplay-start (Azahar oracle, docs/re/scripts.md). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/object_visibility.hpp"
#include "runtime/scene_start.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kStationFile = "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl";
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// Instances the stand-in list of df898de held: the grid read at gameplay-start gave their entity flag bit 0 or no grid entry.
const std::vector<std::string> kDisabledAtStart = {
    "1-1_BI_DonutVanParts", "1-1_BI_DonutVanParts_Prop", "1-1_BrokenVan", "1-1_Broken_Lampost", "1-1_Crate_Trolley",
    "1-1_Donut_Van_Body", "1-1_Donut_Van_Collision", "1-1_DonutsDEMO", "1-1_DonutsDEMO01", "1-1_Mail_Box_Collision",
    "1-1_Mail_Box_Collision01", "1-2_Dog", "1-2_Dog_Bone", "1-2_Dog_Cage", "1-2_Dog_House", "1-2_Dtv_DogPath_XSpot",
    "1-2_Temp_Trampoline", "1-2_TrapDoor_01", "1-3_Camera_chopper", "1-3_Camera_park", "1-3_Cart_crate",
    "1-3_Chopper", "1-3_CraneHook_Collision", "1-3_CraneHook_Down_Collision", "1-3_Crane_Collision",
    "1-3_Crane_Girder", "1-3_Crane_bits", "1-3_Crow", "1-3_Dtv_BrickSpot_XSpot_02", "1-3_Garage_Key",
    "1-3_Gardner_Prop", "1-3_Gardner_Prop_02", "1-3_Generator_Pull_Switch", "1-3_Lawnmower_01", "1-3_Lawnmower_02",
    "1-3_Lawnmower_03", "1-3_Lawnmower_Final", "1-3_Ledge_Platform_Switch", "1-3_Ledge_Platform_Switch_Bits",
    "1-3_Ledge_Platform_Switch_Prop", "1-3_Lift_Crate_02", "1-3_Lift_Crate_03", "1-3_Push_Cart_bits",
    "1-3_Push_Platform", "1-3_Push_cart", "1-3_Roundabout", "1-3_Seesaw", "1-3_Swing", "1-3_TrapDoor_02",
    "1-3_Wheelbarrow02", "1-3_crane_built", "1-3_grab_bits", "1-4_Roadblock_12", "1-4_Roadblock_13",
    "1-4_Roadblock_18", "1-4_Roadblock_19", "1-4_Showroom_Ladder", "1-5_Generic_Prop_10", "1-8_BossGate_01",
    "1-8_BossGate_02", "1-8_Cage_01", "1-8_Goon_Collision_01", "1-8_Goon_Collision_02", "1-9_Ferry",
    "6-1_PlaceDonutsWaypoint", "9_Generic_Prop_081", "Bouncer_01", "CTG_Tshirt", "Donut_Box_01",
    "FP_ElectricRepair_PoliceGarageDoor02", "FP_Platform_TrainStation_MineCart", "FP_Sky_Dive_01", "FP_Spang_01",
    "FP_Task_ASTRONAUT_CTH_01", "FP_Task_ASTRONAUT_CTH_02", "FP_Task_ASTRONAUT_CTH_03", "FP_Task_ASTRONAUT_CTH_04",
    "Fast_Travel_Ferry", "GrappleGun", "Police_Plaque_Floor", "Race01EndBanner", "Race01StartBanner", "Trampoline_03",
    "Trampoline_06", "Trampoline_07",
};
// In the same list, but nothing in the level data hides them: detective X-spots that have no collision (open item).
const std::vector<std::string> kNotExplained = {"1-2_Dtv_DogPath_XSpot", "1-3_Dtv_BrickSpot_XSpot_02"};
// Co-located twins: the grid read has two entities at the position, flags 0x741 (bit 0 set) and 0x740. The first is the one named here.
const std::vector<std::string> kTwinDisabled = {"1-2_Dumpster_Open", "1-3_Garage_Key_Switch", "1-5_WireTap_01", "1-5_WireTap_02",
                                                "1-5_WireTap_03", "1-5_WireTap_04", "Donut_Box", "FP_Warp_01"};
const std::vector<std::string> kTwinEnabled = {"1-2_Dumpster_Closed", "1-3_Garage_Key_Base", "Donut_Box_Empty"};
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: level-start-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "level-start-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const chase::assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const chase::assets::Level level = chase::assets::parseLevel(archive.read(*entry));

        // Provenance: tools/convert_levels-style census with the same parser: 314 scripts in the lvl_01 level file, one named "startup".
        require(level.scripts.size() == 314, "script count changed");
        const std::uint32_t startup = chase::assets::nameHash("startup");
        std::size_t startupCalls = 0, startupScripts = 0;
        for (const auto& script : level.scripts)
            if (script.nameHash == startup) {
                ++startupScripts;
                startupCalls = script.calls.size();
            }
        require(startupScripts == 1 && startupCalls == 339, "startup script changed: " + std::to_string(startupCalls));

        const chase::assets::PlacedObjectState state =
            chase::runtime::levelStartState(level, chase::assets::archiveClassLookup(archive));
        for (const auto& name : kDisabledAtStart) {
            bool unexplained = false;
            for (const auto& other : kNotExplained) unexplained = unexplained || other == name;
            require(state.isDisabled(name) != unexplained, (unexplained ? "unexpectedly disabled: " : "not disabled: ") + name);
        }
        for (const auto& name : kTwinDisabled) require(state.isDisabled(name), "twin not disabled: " + name);
        for (const auto& name : kTwinEnabled) require(!state.isDisabled(name), "twin disabled: " + name);
        require(state.isDisabled("FP_SuperBuildit_Statue"), "super build-it target not hidden");
        for (const char* enabled : {"Police_Door_02", "Prop_Lampost_01", "1-1_Mail_Box"})
            require(!state.isDisabled(enabled), std::string("disabled: ") + enabled);

        require(state.collisionEnabled("1-1_Mail_Box", false), "EnableCollision of 1-1_Mail_Box not applied");
        require(!state.collisionEnabled("1-1_DonutVan01", true), "EnableCollision 0 of 1-1_DonutVan01 not applied");
        require(!state.collisionEnabled("Police_Door_02", false) && state.collisionEnabled("Police_Door_02", true), "default collision flag changed");
        std::cout << "level-start: " << state.disabled.size() << " disabled instances\n";
        // Provenance: an exploration script (Python, same control words) disabled 226 names through the startup script alone;
        // the total of the script and the revealer links is the value this test recorded from the C++ implementation.
        require(state.disabled.size() == 249, "disabled instance count: " + std::to_string(state.disabled.size()));

        // Provenance: LEVOBJ-11. The pale disc at the police entrance was the placement Police_Plaque_Floor
        // (props/prop_respawnabledestructible, the Prop_PolicePlaque model lying on the ground); the original hides it at
        // the start (entity flag bit 0 in the oracle grid read, EnableObject 0x2428c0 hides the scene node; the oracle picture
        // dcam-oracle-2500ms shows no disc). GrappleGun is hidden the same way; the wall plaque Police_Plaque is drawn.
        {
            const chase::assets::DrawableObjects drawable = chase::assets::drawableObjects(archive, level);
            const std::vector<bool> drawn = chase::runtime::drawnPlacements(drawable.objects, state);
            std::size_t hidden = 0;
            bool floorFound = false, wallDrawn = false, grappleFound = false;
            for (std::size_t i = 0; i < drawn.size(); ++i) {
                hidden += drawn[i] ? 0 : 1;
                const std::string& name = drawable.objects[i].instance;
                if (name == "Police_Plaque_Floor") floorFound = true, require(!drawn[i], "Police_Plaque_Floor is drawn");
                if (name == "GrappleGun") grappleFound = true, require(!drawn[i], "GrappleGun is drawn");
                if (name == "Police_Plaque") wallDrawn = drawn[i];
            }
            require(floorFound && grappleFound && wallDrawn, "Police plaque placements changed");
            std::cout << "level-start: " << hidden << " of " << drawn.size() << " drawable objects hidden at the start\n";
            // The count is the value recorded from the C++ implementation (the disabled set is pinned against the oracle above).
            require(hidden == 127, "hidden drawable object count: " + std::to_string(hidden));
        }

        chase::assets::PlacedObjectState runState = chase::runtime::levelInitialState(level, chase::assets::archiveClassLookup(archive));
        chase::runtime::ScriptRunner runner(level, runState);
        const chase::assets::ClassLookup classOf = chase::assets::archiveClassLookup(archive);
        chase::runtime::SceneStart sceneStart(level, classOf, runner);
        chase::runtime::SceneStartOptions startOptions;
        startOptions.dt = chase::runtime::kStartupStepDt;
        startOptions.maxUpdates = chase::runtime::kStartupMaxUpdates;
        const std::uint32_t updates = sceneStart.run(startOptions);
        std::cout << "level-start: startup threads ended after " << updates << " updates; functions without handler:";
        for (const auto& name : runner.unhandledFunctions()) std::cout << ' ' << name << '(' << runner.unhandledCalls(name) << ')';
        std::cout << '\n';
        // Provenance: oracle read at gameplay-start (docs/re/scripts.md 2026-10-14): the thread lists +0x1310 of both
        // resident scenes are empty and the executor 0x234204 is not entered in 3 updates (trap scripts3).
        require(runner.threadCount() == 0 && sceneStart.classThreadCount() == 0, "startup threads still running: " + std::to_string(runner.threadCount()));
        require(runState.disabled == state.disabled, "runner state differs from levelStartState");

        // Provenance: oracle `read` at gameplay-start (docs/re/scripts.md 2026-10-14): enable byte +0 of the 241 trigger
        // records at owner+0x34 (0x08eb22e0, 0x20 each, same order as the file) and byte +0xa of the 482 bounds
        // (0x08663354, 0x28 each). The port runs only the level's startup script, so a few differ (open item SCRIPT-10).
        const std::vector<int> oracleTriggersOff = {1, 67, 68, 69, 70, 71, 79, 80, 81, 82, 98, 99, 110, 141, 203};
        const std::vector<int> knownTriggerDifferences = {};
        for (std::size_t i = 0; i < level.triggers.size(); ++i) {
            const bool off = std::find(oracleTriggersOff.begin(), oracleTriggersOff.end(), int(i)) != oracleTriggersOff.end();
            const bool known = std::find(knownTriggerDifferences.begin(), knownTriggerDifferences.end(), int(i)) != knownTriggerDifferences.end();
            require((runner.triggers()[i].enabled == !off) != known, "trigger enable byte differs from the oracle: " + std::to_string(i));
        }
        const std::vector<int> oracleBoundsOff = {
            2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 18, 19, 20, 21, 22, 23, 24, 25, 29, 30, 38, 42, 44, 45, 46, 47, 48, 51, 52, 53, 54, 55, 67, 68, 70,
            79, 80, 81, 82, 87, 88, 91, 92, 94, 95, 96, 99, 100, 101, 111, 118, 119, 120, 121, 122, 123, 126, 127, 129, 138, 139, 140, 141,
            142, 143, 150, 151, 152, 153, 154, 161, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 202, 203, 204, 223, 224,
            251, 252, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 266, 271, 272, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300,
            301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 332, 333, 334,
            335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360,
            361, 362, 363, 364, 402, 422, 423, 424, 425, 426, 427, 428, 429, 430, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452,
            453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 471, 472, 473, 474, 480};
        const std::vector<int> knownBoundDifferences = {};
        require(level.bounds.size() == 482, "bound count changed");
        for (std::size_t i = 0; i < level.bounds.size(); ++i) {
            const bool off = std::find(oracleBoundsOff.begin(), oracleBoundsOff.end(), int(i)) != oracleBoundsOff.end();
            const bool known = std::find(knownBoundDifferences.begin(), knownBoundDifferences.end(), int(i)) != knownBoundDifferences.end();
            require((bool(runner.boundsEnabled()[i]) == !off) != known, "bound enable byte differs from the oracle: " + level.bounds[i].name);
        }
        // Provenance: the same read, byte +0xb: 1 exactly for the bounds without a room and the bounds of the 12 rooms
        // loaded at gameplay-start (docs/re/streaming.md 2026-10-13T22:27 lists the same rooms).
        for (const char* room : {"Room1", "Room2", "Room3", "Room7", "Room8", "Room9", "Room14", "Room15", "Room16", "Room17", "Room22", "Room23"})
            runner.setRoomLoaded(room, true);
        std::size_t active = 0;
        for (std::size_t i = 0; i < level.bounds.size(); ++i) active += runner.boundActive(i) ? 1 : 0;
        require(active == 482 - 225, "bounds with byte +0xb set: " + std::to_string(active));
        require(runner.boundActive(87), "Police_Station_Transition_Lower active (oracle byte +0xb = 1)");
        require(level.bounds[87].name == "Police_Station_Transition_Lower" && level.bounds[87].shape == 4, "bound 87 changed");

        // Provenance: census with the C++ parser (docs/re/scene-start.md): 24 placed objects of lvl_01 have a class file with a
        // "startup" script (object indices 1119 ... 2870), started from the last to the first.
        require(sceneStart.classOwners().size() == 24, "class startup owners: " + std::to_string(sceneStart.classOwners().size()));
        require(sceneStart.classOwners().front()->instance == "SuperJump_02" && sceneStart.classOwners().front()->objectIndex == 2870, "first owner started");
        require(sceneStart.classOwners().back()->instance == "CharacterLocker" && sceneStart.classOwners().back()->objectIndex == 1119, "last owner started");
        // Provenance: scripts.md 2026-10-14 oracle read: the five triggers and three bounds that the level scripts leave on but the
        // original has off (triggers 79-82 and 141, bounds 266, 271, 272) are exactly the ones these two rules switch.
        require((chase::runtime::triggersWithoutObject(level) == std::vector<std::size_t>{79, 80, 81, 82, 141}), "triggers without object");
        require((chase::runtime::cameraBoundsSwitchedOffAtStart(level, classOf) == std::vector<std::size_t>{266, 271, 272}), "camera bounds switched off");

        // Provenance: oracle state police-station (New 3DS), `read` of the runtime trigger records (owner+0x34 = 0x08e65ac0,
        // 53 x 0x20 bytes) and of the 137 bound records (0x08fa4634, 0x28 each) of the lvl_00 level data 0x08f9cba0: all 53 trigger
        // enable bytes are 1 and bound byte +0xa is 0 for bounds 6, 13 and 14 only
        // (artifacts/oracle/agentM-out/lvl00-police-station-read.txt, parsed with a Python script of the session).
        const chase::assets::FuseEntry* stationEntry = archive.find(kStationFile);
        require(stationEntry != nullptr, "lvl_00 not in archive");
        const chase::assets::Level station = chase::assets::parseLevel(archive.read(*stationEntry));
        require(station.triggers.size() == 53 && station.bounds.size() == 137, "lvl_00 sizes");
        chase::assets::PlacedObjectState stationState = chase::runtime::levelInitialState(station, classOf);
        chase::runtime::ScriptRunner stationRunner(station, stationState);
        chase::runtime::SceneStart stationStart(station, classOf, stationRunner);
        chase::runtime::SceneStartOptions stationOptions;
        stationOptions.dt = chase::runtime::kStartupStepDt;
        stationOptions.maxUpdates = chase::runtime::kStartupMaxUpdates;
        stationOptions.startRoom = 1;
        stationStart.run(stationOptions);
        for (std::size_t i = 0; i < station.triggers.size(); ++i) require(stationRunner.triggers()[i].enabled, "lvl_00 trigger enabled: " + std::to_string(i));
        const std::vector<std::size_t> stationBoundsOff = {6, 13, 14};
        for (std::size_t i = 0; i < station.bounds.size(); ++i) {
            const bool off = std::find(stationBoundsOff.begin(), stationBoundsOff.end(), i) != stationBoundsOff.end();
            require(bool(stationRunner.boundsEnabled()[i]) == !off, "lvl_00 bound byte +0xa: " + station.bounds[i].name);
        }
        require(station.bounds[13].name == "HINT_Basement" && station.bounds[14].name == "HINT_Roof_Lift", "lvl_00 bound names");
    } catch (const std::exception& error) {
        std::cerr << "level-start-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "level-start-real-test: ok\n";
    return 0;
}
