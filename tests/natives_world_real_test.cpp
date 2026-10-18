// Mission 1-2 scripts of lvl_01_cherrytreehills run by ScriptRunner with the natives of docs/re/script-natives-world.md: the call texts are
// the level file's (dumped with the C++ level loader on 2026-10-18). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
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
        std::cerr << "usage: natives-world-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-world-real-test: data not available, skipped\n";
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
        const auto noObject = [](const std::string&, float*) { return false; };

        {
            // The startup script (script hash e48e50f6 in the dump) calls AddToSemiTransNearCameraList for five waypoint models ("", the entry name),
            // EnableDistanceFade(Police_Plaque | FP_Ladder_01 | 1-2_HideoutGate_Left, 0) and SetPartyCharacter(Player1, 9, 1).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runtime::runLevelStartup(runner, 0.034f, 600);
            const auto& list = runner.effects().semiTransNearCamera();
            require(list.size() == 5 && list[0].model == "prop_waypoint_small" && list[4].model == "prop_waypoint_chequeredflag", "five semi-transparent entries");
            for (const char* name : {"police_plaque", "fp_ladder_01", "1-2_hideoutgate_left"})
                require(runner.effects().distanceFade(name) == false, std::string("EnableDistanceFade(") + name + ", 0)");
            const auto party = runner.takePartyRequests();
            require(!party.empty(), "the startup sets the party character");
            for (const runtime::PartyRequest& request : party)
                require(request.slot == 0 && request.character == 9 && request.flag, "SetPartyCharacter(Player1, 9, 1)");
            for (const char* name : {"addtosemitransnearcameralist", "enabledistancefade", "setpartycharacter"})
                require(runner.unhandledCalls(name) == 0, std::string(name) + " is handled");
        }
        {
            // 1-2_MissionStart (hash 3389fe1c): DestroyProp(1-4_Police_Car), EnableObject(1-4_Police_Car, 1), EffectFadeObject(1-4_Police_Car, 0, 1, 1.2, 1).
            // The car is switched off by the startup, so 0x23cd80 leaves it alone and the fade-in raises it from hidden to opaque.
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runtime::runLevelStartup(runner, 0.034f, 600);
            require(state.isDisabled("1-4_Police_Car"), "the police car is off before the mission starts");
            require(runner.start("1-2_MissionStart"), "1-2_MissionStart");
            bool hiddenSeen = false;
            for (int i = 0; i < 1200 && runner.threadCount(); ++i) {
                runner.update(1.0f / 30.0f);
                runner.director().update(1.0f / 30.0f, noObject);
                const auto fade = runner.effects().fade("1-4_police_car");
                if (fade && fade->hidden) hiddenSeen = true;
            }
            require(!state.destroyed.count("1-4_police_car"), "a disabled entity is not destroyed");
            for (const runtime::ObjectClassMessage& message : runner.takeObjectClassMessages())
                require(!(message.instance == "1-4_police_car" && message.message == runtime::kMessageDestroy), "no destroy message for the disabled car");
            const auto fade = runner.effects().fade("1-4_police_car");
            require(hiddenSeen && fade && !fade->hidden && fade->multiplier == 1.0f && runner.effects().activeFades() == 0, "hidden at the start, opaque after 1.2 s");
            for (const char* name : {"destroyprop", "effectfadeobject"}) require(runner.unhandledCalls(name) == 0, std::string(name) + " is handled");
        }
        {
            // 1-2_Cage_Key_Target_Script: ... StreamRoomIn("Room40"), TriggerObject(1-2_Dcam_Exclusion_DogHouse, 1), wait(1.5), DCamCutScenePlay ...
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runner.setRoomLoaded("Room40", false);
            require(runner.start("1-2_Cage_Key_Target_Script"), "1-2_Cage_Key_Target_Script");
            bool requested = false, triggered = false;
            for (int i = 0; i < 400; ++i) {
                runner.update(1.0f / 30.0f);
                runner.director().update(1.0f / 30.0f, noObject);
                for (const std::string& room : runner.takeRoomRequests()) requested = requested || room == "room40";
                for (const runtime::ScriptObjectMessage& message : runner.takeObjectMessages())
                    triggered = triggered || message.instance == "1-2_dcam_exclusion_doghouse";
            }
            require(requested, "the script waits at StreamRoomIn and asks for Room40");
            require(!triggered, "nothing after StreamRoomIn runs while Room40 is not loaded");
            runner.setRoomLoaded("Room40", true);
            for (int i = 0; i < 10; ++i) {
                runner.update(1.0f / 30.0f);
                for (const runtime::ScriptObjectMessage& message : runner.takeObjectMessages())
                    triggered = triggered || message.instance == "1-2_dcam_exclusion_doghouse";
            }
            require(triggered, "TriggerObject(1-2_Dcam_Exclusion_DogHouse) follows once Room40 is loaded");
            require(runner.unhandledCalls("streamroomin") == 0, "streamroomin is handled");
        }
    } catch (const std::exception& error) {
        std::cerr << "natives-world-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-world-real-test: ok\n";
    return 0;
}
