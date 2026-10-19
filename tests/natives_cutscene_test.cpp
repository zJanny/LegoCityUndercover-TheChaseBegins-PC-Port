// Cutscene natives of batch SCRIPT-29 (src/runtime/natives_cutscene.cpp, ScriptRunner). The rules come from the handlers
// DCamCutSceneLoad 0x2889a4 / 0x196390, DCamCutScenePlay 0x26fa18 / 0x2889e4 / 0x1964dc, DCamCutSceneWait 0x288a40 (returns the
// negated busy test 0x2297b0), DCamCutSceneFree 0x28898c / 0x196144, PlayCutscene 0x1bf9dc (docs/re/cutscenes.md,
// entry on the cutscene natives). The scripts and lengths here are made up.
#include "runtime/script_runner.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::CutsceneNativeState;
using chase::runtime::CutsceneRequest;
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

CutsceneNativeState::LengthSource lengths() {
    return [](const std::string& name) -> std::optional<float> {
        if (name == "short") return 1.0f;
        if (name == "long") return 5.0f;
        return std::nullopt;
    };
}

void slotsAndBusyTest() {
    CutsceneNativeState state;
    state.setLengthSource(lengths());
    state.load("short");
    state.load("short");
    require(state.takeRequests().size() == 1, "a second Load of the same name does nothing");
    state.load("missing");
    require(!state.loaded("missing"), "a cutscene without data leaves the slot empty");
    require(!state.busy("short") && !state.busy(""), "loaded but not played is not busy");
    state.play("short", {false, true, true, true});
    require(state.busy("short") && state.busy("") && !state.busy("long"), "the played slot is busy, others are not");
    state.update(0.5f);
    require(state.busy("short"), "busy before the length");
    state.update(0.5f);
    require(!state.busy("short") && state.playing() == nullptr, "the cutscene ends at its length");
}

void fadesAroundTheCutscene() {
    CutsceneNativeState state;
    state.setLengthSource(lengths());
    require(state.veil() == 0.0f, "no veil before any cutscene");
    state.play("long", {true, true, true, true});
    require(state.playing() != nullptr && !state.shown() && state.veil() == 0.0f, "flag a: the game picture stays during the fade, veil starts clear");
    state.update(chase::runtime::kCutsceneFadeSeconds / 2);
    require(state.veil() == 0.0f && !state.takeModeEntered(), "the fade before the start is the screen fade of the runner, not the veil");
    state.update(chase::runtime::kCutsceneFadeSeconds / 2 + 0.001f);
    require(state.shown() && state.playing()->elapsed == 0.0f, "the cutscene starts after the fade and its time starts at 0");
    require(state.takeModeEntered() && !state.takeModeEntered(), "the cutscene mode is entered once, at the end of the fade");
    state.update(4.0f);
    require(state.veil() == 0.0f, "clear until the last 0.5 s");
    state.update(0.75f);
    require(std::fabs(state.veil() - 0.5f) < 0.01f, "half black 0.25 s before the end");
    state.update(0.3f);
    require(state.playing() == nullptr && state.veil() == 1.0f, "black when the cutscene ends");
    state.update(chase::runtime::kCutsceneBlackSeconds + chase::runtime::kCutsceneFadeInSeconds / 2);
    require(std::fabs(state.veil() - 0.5f) < 0.01f, "the game picture fades in");
    state.update(1.0f);
    require(state.veil() == 0.0f, "clear again");
    state.play("short", {false, true, true, true});
    require(state.shown() && state.takeModeEntered(), "without flag a the cutscene starts at once");
}

// FadeScreen 0 leaves the screen black; DCamCutScenePlay (handler 0x26fa18, flag a 0) fades to black in 0 s and the
// cutscene mode enter 0x2fadb8 fades in over 0.5 s (0x252068), so the cutscene is seen (1-2_Cage_Key_Target_Script,
// gdb trace from m12-13-cage-trigger, docs/re/cutscenes.md).
void cutsceneModeBringsTheScreenBack() {
    Level level;
    add(level, "c", {{"FadeScreen", {"0"}}, {"DCamCutScenePlay", {"\"short\"", "0", "1", "1", "1"}}, {"DCamCutSceneWait", {"\"short\""}},
                     {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengths());
    runner.start("c");
    for (int i = 0; i < 8; ++i) runner.update(0.07f);
    require(runner.fadeLevel() == 0.0f && runner.cutscenes().playing() != nullptr, "black when the cutscene is played");
    runner.update(0.07f);
    require(std::fabs(runner.fadeLevel() - 0.0f) < 1e-6f, "the mode enter starts the fade in from black");
    for (int i = 0; i < 3; ++i) runner.update(0.07f);
    require(std::fabs(runner.fadeLevel() - 0.42f) < 1e-4f, "the fade in takes 0.5 s");
    for (int i = 0; i < 5; ++i) runner.update(0.07f);
    require(runner.fadeLevel() == 1.0f, "the cutscene is seen");
}

// Flag a: 0x26fa18 fades to black over 0.5 s before the cutscene mode starts, then the mode fades in.
void flagAFadesTheScreenOut() {
    Level level;
    add(level, "c", {{"DCamCutScenePlay", {"\"long\"", "1", "1", "1", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengths());
    runner.start("c");
    require(runner.fadeLevel() == 1.0f, "the fade starts from the game picture");
    runner.update(0.25f);
    require(std::fabs(runner.fadeLevel() - 0.5f) < 1e-4f, "half way to black");
    runner.update(0.251f);
    require(runner.cutscenes().shown() && runner.fadeLevel() == 0.0f, "the mode is entered at black");
    runner.update(0.5f);
    require(runner.fadeLevel() == 1.0f, "and fades in");
}

void freeEndsThePlayingCutscene() {
    CutsceneNativeState state;
    state.setLengthSource(lengths());
    state.play("long", {true, true, true, true});
    state.takeRequests();
    state.free("long");
    require(!state.busy("long") && !state.loaded("long"), "Free empties the slot and the busy test no longer finds it");
    const auto requests = state.takeRequests();
    require(requests.size() == 1 && requests[0].kind == CutsceneRequest::Kind::Free, "Free is reported to the host");
}

void waitBlocksForTheLength() {
    Level level;
    add(level, "s", {{"DCamCutSceneLoad", {"\"long\""}},
                     {"DCamCutScenePlay", {"\"long\"", "0", "1", "1", "1"}},
                     {"DCamCutSceneWait", {"\"long\""}},
                     {"DCamCutSceneFree", {"\"long\""}},
                     {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengths());
    require(runner.start("s"), "start");
    require(runner.cutscenes().busy("long") && runner.variable("Done") == 0, "Play does not block; Wait does");
    for (int i = 0; i < 148; ++i) runner.update(1.0f / 30.0f);
    require(runner.variable("Done") == 0, "still waiting before 5 s");
    for (int i = 0; i < 5; ++i) runner.update(1.0f / 30.0f);
    require(runner.variable("Done") == 1 && !runner.cutscenes().loaded("long"), "Wait ends at the length and Free follows");
}

void waitForDCamCountsTheCutscene() {
    Level level;
    add(level, "s", {{"DCamCutScenePlay", {"\"short\"", "0", "0", "0", "0"}}, {"WaitForDCam", {}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengths());
    runner.start("s");
    require(runner.variable("Done") == 0, "0x2297b0 with name 0 counts any playing cutscene");
    for (int i = 0; i < 40; ++i) runner.update(1.0f / 30.0f);
    require(runner.variable("Done") == 1, "WaitForDCam ends with the cutscene");
}

void movieBlocksUntilFinished() {
    Level level;
    add(level, "s", {{"PlayCutscene", {"\"01_GameIntro\""}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setHostPlaysMovies(true);
    runner.start("s");
    require(runner.cutscenes().movie() && runner.cutscenes().movie()->name == "01_GameIntro" && runner.cutscenes().movie()->fadeBackIn,
            "PlayCutscene requests the movie");
    runner.update(0.1f);
    require(runner.variable("Done") == 0, "the script waits while the movie plays");
    runner.cutscenes().finishMovie();
    runner.update(0.1f);
    require(runner.variable("Done") == 1, "the script goes on after the movie");
}

void movieWithoutHostEndsAtOnce() {
    Level level;
    add(level, "s", {{"PlayCutscene", {"\"01_GameIntro\""}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.variable("Done") == 1 && !runner.cutscenes().movieOpen(), "without a host the movie ends at once");
}

void noFadeBackIn() {
    Level level;
    add(level, "s", {{"PlayCutsceneNoFadeBackIn", {"\"x\""}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setHostPlaysMovies(true);
    runner.start("s");
    require(runner.cutscenes().movie() && !runner.cutscenes().movie()->fadeBackIn, "PlayCutsceneNoFadeBackIn clears the fade-back-in flag");
}
}

int main() {
    try {
        slotsAndBusyTest();
        freeEndsThePlayingCutscene();
        fadesAroundTheCutscene();
        waitBlocksForTheLength();
        waitForDCamCountsTheCutscene();
        movieBlocksUntilFinished();
        movieWithoutHostEndsAtOnce();
        noFadeBackIn();
        cutsceneModeBringsTheScreenBack();
        flagAFadesTheScreenOut();
    } catch (const std::exception& error) {
        std::cerr << "natives-cutscene-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-cutscene-test: ok\n";
    return 0;
}
