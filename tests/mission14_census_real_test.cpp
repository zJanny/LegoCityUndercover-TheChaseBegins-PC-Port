// The gap list of mission 1-4 (docs/plans/features/script-system-missions-1-4.md, docs/re/scripts-missions-1-4.md) against the level data:
// the scripts named 1-4_*, their natives without a handler, the trigger events by name, the characters and the AI spawners.
// Counts recorded 20261101-mission-C (2026-11-01) with the C++ level loader (parseLevel, drawableObjects) on lvl_01_cherrytreehills_main.blvl and
// the handler table of ScriptRunner. The four natives that had no handler (BulletTime, SpawnDebris2Colour, VehicleSetAI, VehicleSetChased) got
// theirs in 20261102-mission14-C (2026-11-02): no native of the 29 scripts lacks one. When an event gets a check, this test fails: update the gap list with it.
// Needs the extracted RomFS; exits with 77 (skipped) when missing.
#include "assets/assets.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/script_triggers.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool startsWith(const std::string& text, const std::string& prefix) { return text.rfind(prefix, 0) == 0; }

const char* const kScripts[] = {
    "1-4_AI_Goon_10_Run_Off_Script", "1-4_CameraBoundsOFF", "1-4_CameraBoundsON", "1-4_Car_Chase", "1-4_Car_Crash", "1-4_Character_Arrestable_Script",
    "1-4_FallArrestBoundsOFF", "1-4_FallArrestBoundsON", "1-4_Hatch_Open", "1-4_Jump_On_Roof", "1-4_MissionEnd", "1-4_MissionHide", "1-4_MissionSkip",
    "1-4_MissionStart", "1-4_Pursuit_Booster_Spawner_Script", "1-4_RobberCaught", "1-4_RooftopReached", "1-4_Run_Point_01", "1-4_Run_Point_02",
    "1-4_Run_Point_03", "1-4_Run_Point_04", "1-4_Run_Point_05", "1-4_ShowRoom_Stud_Trail", "1-4_ShowroomArrival", "1-4_SO_LadderFallen", "1-4_SO_Phase1",
    "1-4_SO_Phase2", "1-4_Spawner_06_Script", "1-4_Spawner_07_Script"};
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: mission14-census-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "mission14-census-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        require(entry != nullptr, "lvl_01 not in archive");
        const assets::Level level = assets::parseLevel(archive.read(*entry));

        std::map<std::uint32_t, const assets::LevelScript*> byHash;
        for (const assets::LevelScript& script : level.scripts) byHash[script.nameHash] = &script;
        std::set<std::string> withoutHandler;
        for (const char* name : kScripts) {
            const auto found = byHash.find(assets::nameHash(name));
            require(found != byHash.end(), std::string("script not in the level: ") + name);
            for (const assets::ScriptCall& call : found->second->calls)
                if (runtime::scriptFunctionSupport(call.function) == runtime::ScriptFunctionSupport::None) withoutHandler.insert(lowerCase(call.function));
        }
        if (!withoutHandler.empty()) require(false, "a native of the 29 scripts of mission 1-4 has no handler: " + *withoutHandler.begin());
        const std::set<std::string> closedNatives = {"bullettime", "spawndebris2colour", "vehiclesetai", "vehiclesetchased"};
        for (const std::string& name : closedNatives)
            require(runtime::scriptFunctionSupport(name) == runtime::ScriptFunctionSupport::StandIn, name + " is a stand-in (SCRIPT-S323)");

        std::map<std::string, int> events;
        for (const assets::LevelTrigger& trigger : level.triggers)
            if (startsWith(trigger.scriptName, "1-4_")) ++events[trigger.event];
        const std::map<std::string, int> expectedEvents = {{"AIMove", 4},           {"BoundEntered", 3},      {"CharacterArrestable", 2},
                                                           {"CharacterArrested", 2}, {"ObjectBoundEntered", 1}, {"ObjectTriggered", 8},
                                                           {"PlayerOnFootBoundEntered", 5}, {"VehicleChaseEnded", 1}};
        require(events == expectedEvents, "trigger events of the scripts named 1-4_* changed");
        const auto unhandled = [](const std::string& event) {
            return runtime::classifyTriggerEvent(lowerCase(event)) == runtime::TriggerEvent::Unmodelled;
        };
        require(unhandled("CharacterArrestable") && unhandled("CharacterArrested") && unhandled("VehicleChaseEnded") && unhandled("AIMove"),
                "an event of the gap list got a check");

        int objects = 0, spawners = 0, goons = 0;
        for (const assets::LevelObject& object : level.objects) {
            if (!startsWith(object.instance, "1-4_")) continue;
            ++objects;
            if (object.className == "props/prop_aispawner") ++spawners;
            if (startsWith(object.instance, "1-4_AI_Goon_") && object.className.find("char_ai_cherrytreehillsgang") != std::string::npos) ++goons;
        }
        require(objects == 150, "objects named 1-4_*: " + std::to_string(objects));
        require(spawners == 8, "AI spawners named 1-4_*: " + std::to_string(spawners));
        require(goons == 15, "goons named 1-4_AI_Goon_*: " + std::to_string(goons));

        const assets::DrawableObjects drawable = assets::drawableObjects(archive, level, assets::archiveClassLookup(archive));
        int characters = 0;
        for (const assets::CharacterObject& character : drawable.characters)
            if (startsWith(character.placement.instance, "1-4_")) ++characters;
        require(characters == 22, "characters named 1-4_* placed: " + std::to_string(characters));
    } catch (const std::exception& error) {
        std::cerr << "mission14-census-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "mission14-census-real-test: ok\n";
    return 0;
}
