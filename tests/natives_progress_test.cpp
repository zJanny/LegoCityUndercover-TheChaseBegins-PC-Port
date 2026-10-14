// Synthetic scripts for the progress natives of batch SCRIPT-27 (src/runtime/natives_progress.cpp). The rules and offsets
// come from the handlers' disassembly (docs/re/script-natives.md, batch 27 entry; llvm-objdump on game.arm.elf): slot
// base 0x51b6b0; UnlockAbility 0x1cde84 / LockAbility 0x1b24ac (bytes at 0x52089e = slot+0x51ee), UnlockLevel 0x1b276c with
// 0x1b2098 (0x520890 = slot+0x51e0, bit 2*level), SetRaceWon 0x1a30a8 / CountRacesWon 0x1cd910 (slot+0x5200, bits 0..6),
// AwardRacingCar 0x1d9c84 / IsRacingCarAwarded 0x285f64 (byte slot+0x520c), GetCharacterUnlocked 0x29e2c0 with 0x213444
// (slot+0x51bc, ids 1..7 map to 9,19,..,69, id 8 is true), UnlockCharacter 0x263918 (group byte slot+0x51bb),
// IsRedbrickBought 0x26fd30 with 0x1ffb5c (slot+0x51b8), AddStuds 0x18838c, EnableStudsByGroup 0x285ed4, GetLanguage 0x1b241c
// with 0x29e2ec. The scripts are made up.
#include "runtime/script_runner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelParam;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::ProgressNativeState;
using chase::runtime::ScriptFunctionSupport;
using chase::runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void abilitiesAndLevels() {
    Level level;
    add(level, "s", {{"UnlockAbility", {"49"}},
                     {"UnlockAbility", {"2"}},
                     {"LockAbility", {"2"}},
                     {"UnlockLevel", {"3"}},
                     {"IsLevelUnlocked", {"3", "Three"}},
                     {"IsLevelUnlocked", {"4", "Four"}},
                     {"UnlockLevel", {"-1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const ProgressNativeState& natives = runner.progressNatives();
    require(natives.slotImage()[0x51ee + 6] == 0x02 && natives.slotImage()[0x51ee] == 0, "UnlockAbility 49 sets bit 1 of byte 6; 2 was set and cleared");
    require(natives.slotImage()[0x51e0] == 0x40, "UnlockLevel 3 sets bit 6 of the byte at slot+0x51e0");
    require(runner.variable("Three") == 1.0f && runner.variable("Four") == 0.0f, "IsLevelUnlocked sees the unlock");
    require(natives.outOfRangeAccesses() == 1, "a negative level index (unsigned shift) leaves the image");
    require(runner.unhandledFunctions().empty(), "no native of the batch is reported as unhandled");
}

void races() {
    Level level;
    add(level, "s", {{"CountRacesWon", {"None"}},
                     {"SetRaceWon", {"2"}},
                     {"SetRaceWon", {"8"}},
                     {"SetRaceWon", {"2"}},
                     {"SetRaceWon", {"9"}},
                     {"CountRacesWon", {"Count"}},
                     {"IsRacingCarAwarded", {"Before"}},
                     {"AwardRacingCar", {}},
                     {"IsRacingCarAwarded", {"After"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const ProgressNativeState& natives = runner.progressNatives();
    require(runner.variable("None") == 0.0f, "no race won at the start");
    require(natives.slotImage()[0x5200] == 0x01 + 0x40 + 0x80, "races 2, 8 and 9 set bits 0, 6 and 7");
    require(runner.variable("Count") == 2.0f, "CountRacesWon counts bits 0..6 only");
    require(natives.saveRequests() == 4, "three new race bits and AwardRacingCar request a save; the repeated race does not");
    require(runner.variable("Before") == 0.0f && runner.variable("After") == 1.0f && natives.slotImage()[0x520c] == 1, "racing car flag byte");
    require(natives.slotImage()[0x51e3 + 10] == 0x01, "AwardRacingCar sets bit 80 of the id set at slot+0x51e3 (0x205d5c(41, 0))");
    require(natives.slotImage()[0x51e3 + 64 + 5] == 0x02, "and bit 41 of the 'new' set 64 bytes after it");
}

void characters() {
    Level level;
    add(level, "s", {{"GetCharacterUnlocked", {"8", "Eight"}},
                     {"GetCharacterUnlocked", {"1", "OneBefore"}},
                     {"UnlockCharacter", {"9"}},
                     {"GetCharacterUnlocked", {"1", "OneAfter"}},
                     {"GetCharacterUnlocked", {"9", "Nine"}},
                     {"UnlockCharacter", {"19"}},
                     {"UnlockCharacter", {"16"}},
                     {"GetCharacterUnlocked", {"141", "Beyond"}},
                     {"GetCharacterUnlocked", {"16", "Sixteen"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const ProgressNativeState& natives = runner.progressNatives();
    require(runner.variable("Eight") == 1.0f, "character 8 is always unlocked");
    require(runner.variable("OneBefore") == 0.0f && runner.variable("OneAfter") == 1.0f && runner.variable("Nine") == 1.0f,
            "character 1 stands for the first of its group (9)");
    require(natives.slotImage()[0x51bb] == 0x03, "group bits of ids 9 and 19 in the byte at slot+0x51bb");
    require(natives.slotImage()[0x51bc + 2] == 0x04 && natives.slotImage()[0x51bc + 4] == 0x41,
            "unlocked bits 2*9 = 18, 2*19 = 38 and 2*16 = 32");
    require(natives.slotImage()[0x5211 + 2] == 0x09, "the 'new' set at slot+0x5211 holds ids 19 and 16 but not 9 (0x232028)");
    require(runner.variable("Beyond") == 0.0f && runner.variable("Sixteen") == 1.0f, "ids from 141 are locked");
    require(natives.charactersUnlocked().size() == 3, "the requests are recorded");
}

void redBricksAndStuds() {
    Level level;
    add(level, "s", {{"IsRedbrickBought", {"2", "Two"}},
                     {"IsRedbrickBought", {"3", "Three"}},
                     {"AddStuds", {"250"}},
                     {"AddStuds", {"-5"}},
                     {"AddStuds", {"10"}},
                     {"EnableStudsByGroup", {"2", "8"}},
                     {"EnableStudsByGroup", {"258", "6"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    std::vector<std::uint8_t> image(chase::runtime::kSlotImageSize, 0);
    image[0x51b8] = 0x04;
    runner.progressNatives().setSlotImage(image);
    runner.start("s");
    require(runner.variable("Two") == 1.0f && runner.variable("Three") == 0.0f, "IsRedbrickBought reads bit n of slot+0x51b8");
    require(runner.progressNatives().takeStudsAdded() == 260, "AddStuds converts to unsigned: -5 adds 0");
    require(runner.progressNatives().takeStudsAdded() == 0, "taken once");
    const auto groups = runner.progressNatives().takeStudGroupRequests();
    require(groups.size() == 2 && groups[0].group == 2 && groups[0].value == 8 && groups[1].group == 2 && groups[1].value == 6,
            "EnableStudsByGroup keeps the low bytes");
}

void switchesAndLanguage() {
    Level level;
    LevelParam spanish;
    spanish.hash = chase::assets::nameHash("Language_Spanish");
    spanish.type = 5;
    spanish.value = 2;
    level.defaults.push_back(spanish);
    add(level, "s", {{"GetPlayerNumber", {"Player"}},
                     {"GetLanguage", {"English"}},
                     {"SetRaceInProgress", {"1"}},
                     {"EnableSuperBricks", {"2"}},
                     {"SetParachuteDeployHeight", {"12.5"}},
                     {"UnlockScanner", {}},
                     {"DisableSuperBuiltItSave", {"Helipad.SBPlate", "1"}},
                     {"DisableSuperBuiltItSave", {"Other", "2"}},
                     {"GiveReward", {"1"}},
                     {"QuickSave", {}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const ProgressNativeState& natives = runner.progressNatives();
    require(runner.variable("Player") == 1.0f, "GetPlayerNumber");
    require(runner.variable("English") == -1.0f, "a language without its Language_ parameter gives -1 (0x233b80 default)");
    require(natives.raceInProgress() == true && natives.superBricksEnabled() == true && natives.parachuteDeployHeight() == 12.5f &&
                natives.scannerUnlocked(),
            "global switches");
    require(natives.superBuiltItSaves().size() == 2 && natives.superBuiltItSaves()[0].instance == "helipad.sbplate" &&
                natives.superBuiltItSaves()[0].disabled && !natives.superBuiltItSaves()[1].disabled,
            "DisableSuperBuiltItSave stores bit 0 of the argument");
    require(natives.rewards().size() == 1 && natives.rewards()[0] == 1 && natives.saveRequests() == 2, "GiveReward and QuickSave request saves");

    Level spanishLevel;
    spanishLevel.defaults.push_back(spanish);
    add(spanishLevel, "s", {{"GetLanguage", {"Current"}}});
    PlacedObjectState other;
    ScriptRunner spanishRunner(spanishLevel, other);
    spanishRunner.progressNatives().setLanguage(5);
    spanishRunner.start("s");
    require(spanishRunner.variable("Current") == 2.0f, "language 5 reads Language_Spanish");
    ScriptRunner none(spanishLevel, other);
    none.progressNatives().setLanguage(0);
    none.start("s");
    require(none.variable("Current") == -1.0f, "language 0 gives -1");
}

void supportClasses() {
    for (const char* name : {"CountRacesWon", "IsRacingCarAwarded", "UnlockLevel", "GetCharacterUnlocked", "IsRedbrickBought"})
        require(chase::runtime::scriptFunctionSupport(name) == ScriptFunctionSupport::Implemented, name);
    for (const char* name : {"AddStuds", "UnlockAbility", "LockAbility", "UnlockCharacter", "LockCharacter", "GiveReward", "QuickSave", "SetRaceWon", "AwardRacingCar",
                             "EnableStudsByGroup", "GetLanguage", "GetPlayerNumber", "UnlockScanner", "SetRaceInProgress", "EnableSuperBricks",
                             "SetParachuteDeployHeight", "DisableSuperBuiltItSave"})
        require(chase::runtime::scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name);
    require(chase::runtime::progressNativeNames().size() == 22, "22 natives in the batch");
}
}

int main() {
    try {
        abilitiesAndLevels();
        races();
        characters();
        redBricksAndStuds();
        switchesAndLanguage();
        supportClasses();
    } catch (const std::exception& error) {
        std::cerr << "natives-progress-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-progress-test: ok\n";
    return 0;
}
