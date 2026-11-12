// The arrest of a character that left a wrecked car (src/runtime/arrest_after_crash.*, docs/re/combat-characters-arrest.md entry on the arrest after a
// car crash). The arrest states are those of character_arrest.* (0x1c7208, 0x26abcc, 0x25ebe8, 0x282e6c, read from the code by 20261102-mission14b-B);
// what makes the character arrestable is the stand-in COMBAT-S9 (written by 20261111-chase-D, 2026-11-12). The real-data part checks the level data
// the chain needs: the CharacterArrested trigger of `1-4_Booster_Strange_Chaser` starts `1-4_RobberCaught`, whose last calls SpawnScript
// `1-4_MissionEnd` (level data of lvl_01, listed by 20261111-chase-D with the C++ level loader); it exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/arrest_after_crash.hpp"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const std::string& what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what.c_str());
        ++failures;
    }
}

void targets() {
    CrashArrestTargets crash;
    expect(!crash.candidate("chaser", false), "a character that did not leave a vehicle is not a crash target");
    crash.characterLeft({"chaser", "car", {1, 2, 3}});
    expect(crash.contains("chaser") && !crash.contains("goon"), "only the named character is listed");
    const std::optional<ArrestCandidate> candidate = crash.candidate("chaser", false);
    expect(candidate && arrestTargetPasses(*candidate), "the character that left a vehicle passes the arrest test (0x2107e0)");
    const std::optional<ArrestCandidate> avoided = crash.candidate("chaser", true);
    expect(avoided && !arrestTargetPasses(*avoided), "a CharacterAvoidArrest trigger still refuses it");
    crash.remove("chaser");
    expect(!crash.candidate("chaser", false), "removed after the arrest");
}

TakedownTables tables() {
    TakedownTables t;
    t.moves.resize(57);
    t.moves[0] = {{262, 306, 307, 317, 318}, 1, 20};
    return t;
}

void arrestCycle() {
    CrashArrestTargets crash;
    crash.characterLeft({"chaser", "car", {0, 0, 0}});
    const std::optional<ArrestCandidate> candidate = crash.candidate("chaser", false);
    expect(candidate && arrestTargetPasses(*candidate), "set-up: the chaser can be arrested");

    const TakedownTables t = tables();
    TakedownCharacter character;
    CharacterHitTarget target;
    target.health = 3;
    target.maxHealth = 3;
    target.arrestHealth = 2;
    ArrestableList list;
    CharacterTakedownStates takedown;
    CharacterArrestStates arrest;
    arrest.start();
    ArrestStep step = arrest.update(character, target, t, list, takedown, "chaser", false, 1.0f / 30);
    expect(arrest.state() == kArrestState && step.clip && *step.clip == 317, "ARREST plays cuffed_front (row 0 of sheet 2: 317)");
    step = arrest.update(character, target, t, list, takedown, "chaser", true, 1.0f / 30);
    expect(arrest.state() == kArrestedState && step.arrested && step.clip && *step.clip == 318, "the clip's end enters ARRESTED, which raises CharacterArrested");
    int updates = 0;
    while (!arrest.done() && updates < 100) {
        step = arrest.update(character, target, t, list, takedown, "chaser", false, 1.0f / 30);
        ++updates;
    }
    expect(arrest.done() && step.done && character.destroyed && target.destroyed && updates == 30, "ARRESTED ends 30 updates after its enter at 1/30 s: destroyed");
}

std::string lower(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    expect(levelEntry != nullptr, "lvl_01");
    if (!levelEntry) return;
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    std::string script;
    for (const assets::LevelTrigger& trigger : level.triggers)
        if (lower(trigger.event) == "characterarrested" && lower(trigger.objectName) == "1-4_booster_strange_chaser") script = trigger.scriptName;
    expect(script == "1-4_RobberCaught", "CharacterArrested of the chaser starts " + script);
    const std::uint32_t robberCaught = assets::nameHash("1-4_RobberCaught");
    bool spawnsMissionEnd = false;
    for (const assets::LevelScript& levelScript : level.scripts)
        if (levelScript.nameHash == robberCaught)
            for (const assets::ScriptCall& call : levelScript.calls)
                if (call.function == "SpawnScript" && call.arguments.size() == 1 && call.arguments[0] == "1-4_MissionEnd") spawnsMissionEnd = true;
    expect(spawnsMissionEnd, "1-4_RobberCaught spawns 1-4_MissionEnd");
}

}

int main(int argc, char** argv) {
    targets();
    arrestCycle();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("arrest-after-crash-test: data not available, skipped\n");
            return failures ? 1 : 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}
