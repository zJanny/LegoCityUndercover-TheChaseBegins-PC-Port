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
#include <cmath>
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
    require(text.dialogBgFlag() && text.missionTitle() == 1122972713u && text.missionSummary() == 1442628183u, "background, title, summary");
    require(text.dialogue().active && text.dialogue().key == 4060995036u && text.dialogue().character == 88 && text.dialogue().speaker == "npc" &&
                text.dialogue().duration == 4.0f,
            "ShowDialogueText replaces the timed line and uses 4.0 s");
    for (int i = 0; i < 60; ++i) runner.update(1.0f / 30);
    require(!text.tip().active, "tip over after 1.5 s plus five fade updates");
    for (int i = 0; i < 200 && text.dialogue().active; ++i) runner.update(1.0f / 30);
    require(!text.dialogue().active, "dialogue over");

    runtime::ScriptTextState fade;
    fade.showDialogue(1, 0, "", 1.0f);
    fade.update(0.04f);
    require(std::fabs(fade.dialogue().alpha - 0.2f) < 1e-5f, "dialogue alpha rises by dt * 5.0 (0x1cc878, code of 0x1cc550)");
    for (int i = 0; i < 40 && !fade.dialogue().fadingOut; ++i) fade.update(0.04f);
    require(fade.dialogue().fadingOut && fade.dialogue().alpha == 1.0f, "dialogue reaches 1.0 and holds");
    fade.update(0.125f);
    require(std::fabs(fade.dialogue().alpha - 0.5f) < 1e-5f, "dialogue fade-out is linear over 0.25 s (+0x18, 0x235e2c)");

    // Oracle poll of the tip object state (0x517f08) and the alpha (0x51a2d4), state 3 -> 0 at 27.460 s -> 27.726 s wall: alpha stays 1.0 through
    // state 3 (0.25 s) and then falls 0.8 .. 0.0 by 0.2 per update (writer 0x2de284, evidence docs/re/script-text.md).
    runtime::ScriptTextState tipOut;
    tipOut.showTip(1, 1.0f);
    for (int i = 0; i < 5; ++i) tipOut.update(1.0f / 30);
    require(tipOut.tip().alpha == 1.0f && !tipOut.tip().fadingOut, "tip holds");
    for (int i = 0; i < 40 && !tipOut.tip().fadingOut; ++i) tipOut.update(1.0f / 30);
    require(tipOut.tip().fadingOut && tipOut.tip().alpha == 1.0f, "state 3 starts at full alpha");
    for (int i = 0; i < 40 && !tipOut.tip().released; ++i) {
        require(tipOut.tip().alpha == 1.0f, "alpha stays 1.0 in state 3");
        tipOut.update(1.0f / 30);
    }
    require(tipOut.tip().released && tipOut.tip().active, "state 0 after 0.25 s");
    tipOut.update(1.0f / 30);
    require(std::fabs(tipOut.tip().alpha - 0.8f) < 1e-5f && tipOut.tip().active, "falls by 0.2 per update");
    for (int i = 0; i < 4; ++i) tipOut.update(1.0f / 30);
    require(int(tipOut.tip().alpha * 255.0f) == 0, "alpha * 255 is 0 after five updates (0x15b564, float residue 2.4e-8)");
    tipOut.update(1.0f / 30);
    require(tipOut.tip().alpha == 0.0f && !tipOut.tip().active, "clamped to 0 by the sixth update");

    // Heading object 0x517ec8 on the oracle: +0x14 = +0x18 = 0.5, +0x1c = 5.0 (read over RPC, state 1 -> 2 -> 3 -> 0, state 2 lasts 5.0 s of game clock).
    runtime::ScriptTextState heading;
    heading.showHeading(7);
    require(heading.heading().active && heading.heading().key == 7 && heading.heading().alpha == 0.0f, "heading starts invisible");
    heading.update(0.25f);
    require(std::fabs(heading.heading().alpha - 0.5f) < 1e-5f, "heading fades in linearly over 0.5 s");
    heading.update(0.25f);
    heading.update(2.0f);
    require(heading.heading().alpha == 1.0f, "heading holds");
    heading.update(3.0f);
    heading.update(0.25f);
    require(std::fabs(heading.heading().alpha - 0.5f) < 1e-5f && heading.heading().active, "heading fades out linearly over 0.5 s");
    heading.update(0.25f);
    require(!heading.heading().active, "heading over after 6.0 s");

    // ShowIntroText 0x1cdc2c: the key goes through the hint text table [0x37596c] into the same heading object 0x517ec8 as ShowLevelText (0x20a4a8).
    assets::Level intro;
    add(intro, "i", {{"ShowIntroText", {"1122972714"}}});
    assets::PlacedObjectState introState;
    ScriptRunner introRunner(intro, introState);
    introRunner.start("i");
    require(introRunner.text().heading().active && introRunner.text().heading().hintText && introRunner.text().heading().key == 1122972714u,
            "ShowIntroText starts the heading with a hint text key");

    // ShowCharacterIntro 0x2c2af8 -> 0x1be0ac(id, 4.0): the dialogue object with the character id, shown for the literal 4.0 (0x2c2b50) with no hint text key.
    assets::Level characterIntro;
    add(characterIntro, "c", {{"ShowCharacterIntro", {"21"}}});
    assets::PlacedObjectState characterState;
    ScriptRunner characterRunner(characterIntro, characterState);
    characterRunner.start("c");
    require(characterRunner.text().dialogue().active && characterRunner.text().dialogue().characterIntro && characterRunner.text().dialogue().character == 21 &&
                characterRunner.text().dialogue().duration == 4.0f,
            "ShowCharacterIntro starts the dialogue object with the character");

    // InformationBox 0x1d9d4c: opens the dialog mode with the script's key, "OK" 0xd12f028b and the empty 0xdf3cad3b; same flow as WarningDialogBox (docs/re/scripts-runtime.md
    // §2026-10-14T10:54), blocks until the answer, writes no variable.
    assets::Level info;
    add(info, "b", {{"InformationBox", {"2082939282"}}, {"ShowTipTextTimed", {"5", "1"}}});
    assets::PlacedObjectState infoState;
    ScriptRunner infoRunner(info, infoState);
    infoRunner.start("b");
    require(infoRunner.dialogOpen() && infoRunner.dialog().bodyKey == 2082939282u && infoRunner.dialog().cancelKey == 0xdf3cad3bu, "InformationBox opens the dialog");
    infoRunner.update(1.0f / 30);
    require(infoRunner.dialogOpen() && !infoRunner.text().tip().active, "InformationBox blocks");
    infoRunner.answerDialog(runtime::kDialogCancel);
    require(infoRunner.dialogOpen(), "no cancel button on the information box");
    infoRunner.answerDialog(runtime::kDialogConfirm);
    infoRunner.update(1.0f / 30);
    require(!infoRunner.dialogOpen() && infoRunner.text().tip().active, "InformationBox continues after A");

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
