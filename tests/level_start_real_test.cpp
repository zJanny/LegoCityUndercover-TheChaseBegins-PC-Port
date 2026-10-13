// Level start state of lvl_01_cherrytreehills (src/runtime/level_start.cpp) against what the original game had at
// gameplay-start (Azahar oracle, docs/re/scripts.md). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
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
    } catch (const std::exception& error) {
        std::cerr << "level-start-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "level-start-real-test: ok\n";
    return 0;
}
