// Scripts of lvl_01_cherrytreehills that use the object natives of batch SCRIPT-26, run by ScriptRunner. The expected values
// are the static call texts of the level file (dumped with the C++ level loader on 2026-10-14: every script of the level
// with its calls). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: natives-objects-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-objects-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        {
            // DestroyProp targets of lvl_01 (census over the 30 level files with the C++ loader, 2026-10-19): 1-3_smash_crate is a leprops/leprop_prop
            // (kind 67), 1-3_Fence_08 a leprops/leprop_wobblydestructible (119), 1-4_Police_Car a vehicles/vehicle_police_trooper (183). Reactions of
            // message 1: docs/re/script-natives-world.md, entry 2026-10-19T08:53:30.
            assets::Level destroyLevel = level;
            assets::LevelScript script;
            script.nameHash = assets::nameHash("r27_destroy");
            script.calls = {{"DestroyProp", {"1-3_smash_crate"}}, {"DestroyProp", {"1-3_Fence_08"}}, {"DestroyProp", {"1-4_Police_Car"}}};
            destroyLevel.scripts.push_back(script);
            assets::PlacedObjectState state = runtime::levelInitialState(destroyLevel, classOf);
            runtime::ScriptRunner runner(destroyLevel, state);
            runner.loadObjectKinds(classOf);
            const runtime::ObjectNativeState& natives = runner.objectNatives();
            require(natives.kindOf("1-3_smash_crate") == 67 && natives.kindOf("1-3_fence_08") == 119 && natives.kindOf("1-4_police_car") == 183, "kinds of the targets");
            require(runner.start("r27_destroy") && runner.threadCount() == 0, "r27_destroy");
            require(state.destroyed.count("1-3_smash_crate") && state.isHidden("1-3_smash_crate"), "kind 67 is destroyed and hidden");
            require(state.destroyed.count("1-3_fence_08") && state.isHidden("1-3_fence_08"), "kind 119 is destroyed and hidden");
            require(state.destroyed.count("1-4_police_car") && !state.isHidden("1-4_police_car"), "the vehicle is destroyed and stays visible (it breaks)");
        }

        {
            // lvl_04 4-1_Crate_Faller_Platform is a props/prop_platform (kind 64, DestroyProp target of the census, docs/re/script-natives-world.md 2026-10-19T10:02:26).
            // Message 8 of kind 64 is its own platform reset (0x1a90b8, docs/re/script-natives-world-reset.md 2026-10-19T11:06:52): it enables the entity and leaves the destroyed bit.
            const assets::FuseEntry* faller = archive.find("levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl");
            require(faller != nullptr, "lvl_04 level file");
            assets::Level resetLevel = assets::parseLevel(archive.read(*faller));
            auto lowered = [](std::string text) {
                for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return text;
            };
            std::string pickup;
            {
                assets::PlacedObjectState probeState;
                runtime::ScriptRunner probe(resetLevel, probeState);
                probe.loadObjectKinds(classOf);
                for (const assets::LevelObject& object : resetLevel.objects)
                    if (pickup.empty() && probe.objectNatives().kindOf(lowered(object.instance)) == 132) pickup = object.instance;
            }
            require(!pickup.empty(), "lvl_04 has a kind 132 pickup");
            assets::LevelScript script;
            script.nameHash = assets::nameHash("r30_reset");
            script.calls = {{"DestroyProp", {"4-1_Crate_Faller_Platform"}}, {"ResetObject", {"4-1_Crate_Faller_Platform"}}, {"DestroyProp", {pickup}}, {"ResetObject", {pickup}}};
            resetLevel.scripts.push_back(script);
            assets::PlacedObjectState state = runtime::levelInitialState(resetLevel, classOf);
            runtime::ScriptRunner runner(resetLevel, state);
            runner.loadObjectKinds(classOf);
            require(runner.objectNatives().kindOf("4-1_crate_faller_platform") == 64, "kind 64");
            require(runner.start("r30_reset") && runner.threadCount() == 0, "r30_reset");
            const std::string name = "4-1_crate_faller_platform";
            require(state.destroyed.count(name) && !state.isHidden(name), "the platform reset enables the entity and keeps the destroyed bit");
            require(runner.objectNatives().restoredPlacement(name) == nullptr, "no placement is recorded for kind 64");
            const std::string pickupKey = lowered(pickup);
            require(!state.destroyed.count(pickupKey) && !state.isHidden(pickupKey), "the pickup reset clears the destroyed bit and enables the entity");
            const runtime::PlacementReset* placement = runner.objectNatives().restoredPlacement(pickupKey);
            require(placement && placement->hasPosition, "pickup placement restored");
            for (const assets::Placement& placed : assets::objectPlacements(resetLevel, classOf))
                if (lowered(placed.instance) == pickupKey)
                    require(std::fabs(placed.position[0] - placement->position[0]) < 0.01f && std::fabs(placed.position[2] - placement->position[2]) < 0.01f,
                            "the restored placement is the one the level places");
        }
        {
            // Script 1-3_parkour_working: ToggleUseable(1-3_grab_01..04, 1), EnableObject(BrickGrab_Mount_Hint_06, 1).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-3_parkour_working") && runner.threadCount() == 0, "1-3_parkour_working");
            for (const char* grab : {"1-3_grab_01", "1-3_grab_02", "1-3_grab_03", "1-3_grab_04"}) {
                const runtime::ObjectNativeFlags* flags = runner.objectNatives().flags(grab);
                require(flags && flags->usable == true, std::string("usable ") + grab);
            }
            require(!runner.unhandledCalls("toggleuseable"), "ToggleUseable is handled");
        }
        {
            // Script 1-3_Platform_Down: ToggleUseable(1-3_Ledge_Platform_Switch_Ground, 0), PrintDebugString(...).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-3_Platform_Down"), "1-3_Platform_Down");
            const runtime::ObjectNativeFlags* flags = runner.objectNatives().flags("1-3_ledge_platform_switch_ground");
            require(flags && flags->usable == false && !runner.unhandledCalls("printdebugstring"), "Platform_Down");
        }
        {
            // Script 1-3_Ledge_Platform_Switch_Bottom_Script ends with ResetObject of 1-3_Ledge_Platform_Switch,
            // 1-3_Ledge_Platform and 1-3_Ledge_Platform_Switch_Ground; the natives before it have no handler and continue.
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-3_Ledge_Platform_Switch_Bottom_Script"), "Switch_Bottom_Script");
            for (int i = 0; i < 10 && runner.threadCount(); ++i) runner.update(1.0f / 30.0f);
            const auto messages = runner.takeObjectClassMessages();
            require(messages.size() == 3 && messages[0].instance == "1-3_ledge_platform_switch" && messages[1].instance == "1-3_ledge_platform" &&
                        messages[2].instance == "1-3_ledge_platform_switch_ground",
                    "ResetObject messages");
            for (const runtime::ObjectClassMessage& message : messages) require(message.message == runtime::kMessageReset, "message 8");
        }
        {
            // 1-2_Scanned_Pringles: Wait 1.5, EnableScanner, KillScript(1-2_Scanner_Enter_Script), Wait 2, ... ShowTipTextTimed after
            // 10.5 s. 1-2_Scanner_Enter_Script: PrintDebugString, Wait 1.5, ShowTipTextTimed(938638044, 5), Wait 5, ShowTipTextTimed ...
            assets::PlacedObjectState alone = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner control(level, alone);
            control.start("1-2_Scanner_Enter_Script");
            for (int i = 0; i < 60; ++i) control.update(0.1f);
            require(control.text().tip().key == 938638044u, "without the kill the scanner script shows its tip");

            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runner.start("1-2_Scanner_Enter_Script");
            runner.start("1-2_Scanned_Pringles");
            for (int i = 0; i < 60; ++i) runner.update(0.1f);
            require(runner.text().tip().key == 0, "KillScript ends the scanner script before its first tip");
            require(!runner.running("1-2_Scanner_Enter_Script", false), "the scanner script thread is gone");
            require(runner.running("1-2_Scanned_Pringles", false), "the killer continues");
            const runtime::ObjectNativeFlags* zone = runner.objectNatives().flags("1-2_scannerzoneaudio");
            require(zone && zone->usable == false, "ToggleUseable(1-2_ScannerZoneAudio, 0)");
        }
        {
            // Call texts of the main levels (dumped with the C++ loader, 2026-10-18): lvl_06 ToggleUseable(5-2_Crate_Push, 1) on a
            // leprops/leprop_pushable (kind 71) that nests four leprops/leprop_pushhandle (kind 72); lvl_01 EnableShadow(1-4_Booster_Strange, 0)
            // on a character (kind 27); lvl_04 EnableSwitch(4-6_Pig_01_SwitchWheel_Fake, 0) on a prop_switchpull (kind 58) and lvl_07
            // EnableSwitch(7-4_Skylight, 0) on an ability_breakandenter_skylight01 (kind 185).
            const auto loaded = [&](const char* levelFile) {
                const assets::FuseEntry* found = archive.find(levelFile);
                require(found != nullptr, std::string("not in archive: ") + levelFile);
                return assets::parseLevel(archive.read(*found));
            };
            const auto mainLevel = [](const char* name) { return std::string("levels/levels/") + name + "/" + name + "_main.blvl"; };
            {
                const assets::Level cityLevel = loaded(mainLevel("lvl_06_apolloisland").c_str());
                runtime::ObjectNativeState natives;
                natives.loadKinds(cityLevel, classOf);
                require(natives.kindOf("5-2_crate_push") == runtime::kKindPushable, "5-2_Crate_Push is a pushable");
                natives.apply(runtime::ObjectNativeKind::ToggleUseable, "5-2_crate_push", 1.0f);
                require(natives.flags("5-2_crate_push") == nullptr, "the pushable (kind 71) registers no use zone itself");
                for (const char* handle : {"handle01", "handle02", "handle03", "handle04"}) {
                    const std::string key = std::string("5-2_crate_push.") + handle;
                    require(natives.kindOf(key) == runtime::kKindPushHandle, key + " kind");
                    require(natives.flags(key) && natives.flags(key)->usable == true, key + " flag");
                }
                natives.apply(runtime::ObjectNativeKind::ToggleUseable, "5-2_crate_push", 0.0f);
                require(natives.flags("5-2_crate_push.handle03")->usable == false, "handle flag follows the pushable");
            }
            {
                const assets::Level level01 = loaded(kLevelFile);
                runtime::ObjectNativeState natives;
                natives.loadKinds(level01, classOf);
                natives.apply(runtime::ObjectNativeKind::EnableShadow, "1-4_booster_strange", 0.0f);
                require(natives.flags("1-4_booster_strange") && natives.flags("1-4_booster_strange")->shadow == false, "EnableShadow on a character");
            }
            {
                const assets::Level level04 = loaded(mainLevel("lvl_04_fortmeadows").c_str());
                runtime::ObjectNativeState natives;
                natives.loadKinds(level04, classOf);
                natives.apply(runtime::ObjectNativeKind::EnableSwitch, "4-6_pig_01_switchwheel_fake", 0.0f);
                const runtime::ObjectNativeFlags* flags = natives.flags("4-6_pig_01_switchwheel_fake");
                require(flags && flags->switchState == 3 && flags->usable == false, "EnableSwitch off on kind 58");
            }
            {
                // Call texts (dumped 2026-10-19 with the C++ loader): lvl_06 ToggleUseable(5-5_Door_02, 0) on a leprops/leprop_door (kind 42),
                // lvl_04 ToggleUseable(4-2_ContainerDoors, 0) on a leprops/leprop_simpleindestructible (kind 86); neither kind's create or
                // post-link function calls 0x23e130 (56 call sites read, docs/re/object-kinds.md), so 0x236624 finds no registry entry and
                // changes nothing. lvl_06 ToggleUseable on a kind 71 pushable writes only its kind 72 handles.
                const assets::Level cityLevel = loaded(mainLevel("lvl_06_apolloisland").c_str());
                runtime::ObjectNativeState natives;
                natives.loadKinds(cityLevel, classOf);
                require(natives.kindOf("5-5_door_02") == 42, "5-5_Door_02 is a door");
                natives.apply(runtime::ObjectNativeKind::ToggleUseable, "5-5_door_02", 0.0f);
                require(natives.flags("5-5_door_02") == nullptr, "a door registers no use zone");
                natives.apply(runtime::ObjectNativeKind::ToggleUseable, "5-2_crate_push", 1.0f);
                require(natives.flags("5-2_crate_push") == nullptr && natives.flags("5-2_crate_push.handle01")->usable == true,
                        "the pushable itself has no zone, its handles do");
                const assets::Level level04 = loaded(mainLevel("lvl_04_fortmeadows").c_str());
                runtime::ObjectNativeState natives04;
                natives04.loadKinds(level04, classOf);
                natives04.apply(runtime::ObjectNativeKind::ToggleUseable, "4-2_containerdoors", 0.0f);
                require(natives04.kindOf("4-2_containerdoors") == 86 && natives04.flags("4-2_containerdoors") == nullptr, "kind 86 registers no use zone");
            }
            {
                // enableobjectfade calls of the main levels (dumped 2026-10-19): lvl_02 on a dummy/dummymesh (kind 0), lvl_06 on a
                // props/prop_fanlift (kind 47) and a props/prop_prop (kind 67); no handler of those kinds compares the message with 24.
                const assets::Level level02 = loaded(mainLevel("lvl_02_albatrossisland").c_str());
                runtime::ObjectNativeState natives;
                natives.loadKinds(level02, classOf);
                require(natives.kindOf("00_searchlightbeam01") == 0 && natives.enableObjectFadeFallsBack("00_searchlightbeam01"), "kind 0 does not answer 24");
                const assets::Level cityLevel = loaded(mainLevel("lvl_06_apolloisland").c_str());
                runtime::ObjectNativeState natives06;
                natives06.loadKinds(cityLevel, classOf);
                require(natives06.kindOf("fp_tokenfan_blower_01") == 47 && natives06.enableObjectFadeFallsBack("fp_tokenfan_blower_01"), "kind 47 does not answer 24");
                require(natives06.kindOf("5-2_skylight_01_open") == 67 && natives06.enableObjectFadeFallsBack("5-2_skylight_01_open"), "kind 67 does not answer 24");
            }
            {
                const assets::Level level07 = loaded(mainLevel("lvl_07_downtown").c_str());
                runtime::ObjectNativeState natives;
                natives.loadKinds(level07, classOf);
                natives.apply(runtime::ObjectNativeKind::EnableSwitch, "7-4_skylight", 0.0f);
                const runtime::ObjectNativeFlags* flags = natives.flags("7-4_skylight");
                require(flags && !flags->switchState && flags->usable == false, "EnableSwitch off on kind 185 writes no state");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "natives-objects-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-objects-real-test: ok\n";
    return 0;
}
