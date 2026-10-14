// Script text natives of batch SCRIPT-30 (src/runtime/script_text.cpp). Values come from the disassembly (llvm-objdump on
// game.arm.elf; docs/re/script-text.md): ShowTipTextOnce 0x263850 (duration 5.0 from the literal at 0x2638e0),
// ShowTipTextTimed 0x26ffec, ShowDialogueText 0x26ff84 (4.0 from 0x26ffe4), ShowDialogueTextTimed 0x2a6d90, ShowDialogBG
// 0x1bfb64, SetMissionTitle 0x263788, SetMissionSummary 0x27be98, 0x235f30 (duration 0 becomes 4.0). The oracle (agent X,
// state agent-doughnut-title, RPC poll of the object at 0x517ef8) showed durations 3, 3, 4 and 5 s for the four tips of
// 1-1_OnScreenText_Script and an alpha of 0.2 per update in and out. With no argument the real-data part runs on the
// extracted RomFS (FIB NAMES) and exits with 77 when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(assets::Level& level, const std::string& name, std::vector<assets::ScriptCall> calls) {
    assets::LevelScript script;
    script.nameHash = assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void synthetic() {
    assets::Level level;
    add(level, "s", {{"ShowTipTextOnce", {"4094106649"}},
                     {"Wait", {"3"}},
                     {"ShowTipTextTimed", {"4094106651", "1.5"}},
                     {"ShowDialogBG", {"1"}},
                     {"SetMissionTitle", {"1122972713"}},
                     {"SetMissionSummary", {"1442628183"}},
                     {"ShowDialogueTextTimed", {"4060995032", "11", "Player1", "3.5"}},
                     {"ShowDialogueText", {"4060995036", "88", "npc"}}});
    assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("s"), "start");
    const runtime::ScriptTextState& text = runner.text();
    require(text.tip().active && text.tip().key == 4094106649u && text.tip().duration == 5.0f, "tip once: key beyond float precision, 5.0 s");
    require(text.tip().alpha == 0.0f, "no alpha before the first update");
    runner.update(1.0f / 30);
    require(text.tip().alpha > 0.19f && text.tip().alpha < 0.21f, "alpha 0.2 after one update");
    for (int i = 0; i < 4; ++i) runner.update(1.0f / 30);
    require(text.tip().alpha == 1.0f, "alpha 1 after five updates");
    for (int i = 0; i < 90; ++i) runner.update(1.0f / 30);
    require(text.tip().key == 4094106651u && text.tip().duration == 1.5f && text.tip().alpha == 1.0f,
            "the Wait 3 ends, the next tip replaces the text without a fade");
    require(text.dialogueBackground() && text.missionTitle() == 1122972713u && text.missionSummary() == 1442628183u, "background, title, summary");
    require(text.dialogue().active && text.dialogue().key == 4060995036u && text.dialogue().character == 88 && text.dialogue().speaker == "npc" &&
                text.dialogue().duration == 4.0f,
            "ShowDialogueText replaces the timed line and uses 4.0 s");
    for (int i = 0; i < 60; ++i) runner.update(1.0f / 30);
    require(!text.tip().active, "tip over after 1.5 s plus five fade updates");
    for (int i = 0; i < 200 && text.dialogue().active; ++i) runner.update(1.0f / 30);
    require(!text.dialogue().active, "dialogue over");

    assets::Level zero;
    add(zero, "z", {{"ShowTipTextTimed", {"1", "0"}}});
    assets::PlacedObjectState zeroState;
    ScriptRunner zeroRunner(zero, zeroState);
    zeroRunner.start("z");
    require(zeroRunner.text().tip().duration == 4.0f, "duration 0 becomes 4.0 s (0x235f30)");

    for (const char* name : {"ShowTipTextOnce", "ShowTipTextTimed", "ShowDialogueText", "ShowDialogueTextTimed", "ShowDialogBG", "SetMissionTitle",
                             "SetMissionSummary", "ShowIntroText", "ShowLevelText", "ShowCharacterIntro", "ShowTally", "ShowBossHearts",
                             "ShowRaceStart", "InformationBox", "ShowPortraitOverVehicle", "ShowInfoPointer"})
        require(runtime::scriptFunctionSupport(name) == runtime::ScriptFunctionSupport::StandIn, std::string("stand-in ") + name);
}

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level missing");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
    ScriptRunner runner(level, state);
    require(runner.start("1-1_OnScreenText_Script"), "script");
    const runtime::ScriptTextState& text = runner.text();
    // Oracle (state agent-doughnut-title, poll of 0x517ef8): the tips follow at 3 s, 6 s and 10 s, text keys from the level file.
    const std::uint32_t keys[4] = {4094041050u, 4094041051u, 148829677u, 4094041053u};
    int changeAt[4] = {0, 0, 0, 0};
    int seen = 1;
    require(text.tip().key == keys[0], "first tip");
    for (int update = 1; update < 600 && seen < 4; ++update) {
        runner.update(1.0f / 30);
        if (text.tip().key == keys[seen]) changeAt[seen++] = update;
    }
    require(seen == 4, "four tips");
    require(changeAt[1] >= 88 && changeAt[1] <= 92, "second tip after Wait 3");
    require(changeAt[2] - changeAt[1] >= 88 && changeAt[2] - changeAt[1] <= 92, "third tip after Wait 3");
    require(changeAt[3] - changeAt[2] >= 118 && changeAt[3] - changeAt[2] <= 122, "fourth tip after Wait 4");
    require(!runner.unhandledCalls("showtiptextonce") && !runner.unhandledCalls("showdialogbg"), "handled");
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            synthetic();
            return 0;
        }
        if (argc != 3) {
            std::cerr << "usage: script-text-test [FIB NAMES]\n";
            return 2;
        }
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "script-text-test: data not available, skipped\n";
            return 77;
        }
        realData(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "script-text-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
