// Scripts of lvl_01_cherrytreehills that call the progress natives of batch SCRIPT-27, run by ScriptRunner. The scripts are
// found by the calls they contain (static call texts of the level file, dumped with the C++ level loader on 2026-10-14) and
// named through the trigger and CallScript / SpawnScript references. Needs the extracted RomFS; exits with 77 (skipped)
// when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::map<std::uint32_t, std::string> scriptNames(const assets::Level& level) {
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelTrigger& trigger : level.triggers) names[assets::nameHash(trigger.scriptName)] = trigger.scriptName;
    for (const assets::LevelScript& script : level.scripts)
        for (const assets::ScriptCall& call : script.calls) {
            const std::string function = lowerCase(call.function);
            if ((function == "callscript" || function == "spawnscript") && !call.arguments.empty())
                names[assets::nameHash(call.arguments[0])] = call.arguments[0];
        }
    return names;
}

// The first script whose calls contain every given "function argument ..." text.
std::string scriptWith(const assets::Level& level, const std::vector<std::string>& wanted) {
    const auto names = scriptNames(level);
    for (const assets::LevelScript& script : level.scripts) {
        std::size_t found = 0;
        for (const std::string& text : wanted)
            for (const assets::ScriptCall& call : script.calls) {
                std::string line = call.function;
                for (const std::string& argument : call.arguments) line += " " + argument;
                while (!line.empty() && line.back() == ' ') line.pop_back();
                if (lowerCase(line) == lowerCase(text)) {
                    ++found;
                    break;
                }
            }
        if (found == wanted.size()) {
            const auto name = names.find(script.nameHash);
            require(name != names.end(), "script without a known name: " + wanted[0]);
            return name->second;
        }
    }
    throw std::runtime_error("no script with " + wanted[0]);
}

void runToEnd(runtime::ScriptRunner& runner, const std::string& script) {
    require(runner.start(script), "script " + script);
    const auto noObject = [](const std::string&, float*) { return false; };
    for (int i = 0; i < 3000 && runner.threadCount() > 0; ++i) {
        runner.update(0.034f);
        runner.director().update(0.034f, noObject);
    }
    require(runner.threadCount() == 0, "script ends: " + script);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: natives-progress-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-progress-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const auto fresh = [&](assets::PlacedObjectState& state) { state = runtime::levelInitialState(level, classOf); };

        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"UnlockAbility 49", "UnlockAbility 46", "UnlockAbility 48", "UnlockAbility 47"}));
            const auto& image = runner.progressNatives().slotImage();
            require(image[0x51ee + 5] == 0x40 + 0x80 && image[0x51ee + 6] == 0x01 + 0x02, "abilities 46..49 are bits 6, 7 of byte 5 and 0, 1 of byte 6");
            require(runner.unhandledCalls("unlockability") == 0, "UnlockAbility is handled");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"UnlockLevel 3", "GiveReward 1"}));
            require(runner.progressNatives().slotImage()[0x51e0] == 0x40, "UnlockLevel 3");
            require(runner.progressNatives().rewards() == std::vector<std::uint32_t>{1}, "GiveReward 1");
            require(runner.progressNatives().saveRequests() >= 1, "GiveReward asks for a save");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"GetCharacterUnlocked 16 PlainclothesAlreadyCollected", "UnlockCharacter 16"}));
            require(runner.variable("PlainclothesAlreadyCollected") == 0.0f, "character 16 is locked in a new slot");
            require(runner.progressNatives().charactersUnlocked() == std::vector<std::uint32_t>{16}, "UnlockCharacter 16");
            require(runner.progressNatives().slotImage()[0x51bc + 4] & 0x01, "unlocked bit 32");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"SetRaceWon 2", "CountRacesWon AllRacesWon"}));
            require(runner.progressNatives().slotImage()[0x5200] == 0x01, "race 2 is bit 0");
            require(runner.variable("AllRacesWon") == 1.0f, "one race won");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"EnableStudsByGroup 2 8"}));
            const auto groups = runner.progressNatives().takeStudGroupRequests();
            require(!groups.empty() && groups[0].group == 2 && groups[0].value == 8, "EnableStudsByGroup 2 8");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"UnlockScanner"}));
            require(runner.progressNatives().scannerUnlocked(), "UnlockScanner");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"DisableSuperBuiltItSave 1-3_Helipad.SBPlate 1"}));
            const auto& saves = runner.progressNatives().superBuiltItSaves();
            require(saves.size() == 1 && saves[0].instance == "1-3_helipad.sbplate" && saves[0].disabled, "DisableSuperBuiltItSave");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runner.progressNatives().setLanguage(5);
            runToEnd(runner, scriptWith(level, {"GetLanguage CurrentLanguage"}));
            require(runner.variable("CurrentLanguage") == 2.0f, "language 5 reads the saved default of Language_Spanish (2)");
        }
        {
            assets::PlacedObjectState state;
            fresh(state);
            runtime::ScriptRunner runner(level, state);
            runToEnd(runner, scriptWith(level, {"GetPlayerNumber PlayerNumber"}));
            require(runner.variable("PlayerNumber") == 1.0f, "GetPlayerNumber");
        }
    } catch (const std::exception& error) {
        std::cerr << "natives-progress-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-progress-real-test: ok\n";
    return 0;
}
