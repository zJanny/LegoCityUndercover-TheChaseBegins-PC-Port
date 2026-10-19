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
    state.update(0.5f, false);
    require(state.busy("short"), "busy before the length");
    state.update(0.5f, false);
    require(state.busy("short"), "busy through the end fade and the wait before the exit");
    state.takeFadeRequest();
    state.update(0.1f, false);
    state.update(chase::runtime::kCutsceneExitWaitSeconds, false);
    require(!state.busy("short") && state.playing() == nullptr, "the cutscene ends at the exit after its length");
}

// Update 0x2fae68: once the cutscene has ended (0x2d19cc returns 0) it starts the screen fade to black (0x2520e8, 0.5 s, screen
// byte 1); the mode switch waits until the fade has ended (loop 0x109708) and then two render ticks; the exit 0x2fac7c fades
// in over 0.5 s (0x252068) unless the cutscene is 7_9_A. Provenance: llvm-objdump of those addresses and the ctrace of
// state cutscene-1-1-b (artifacts/oracle/r28E-out/endtrace.txt): 0x256a14 from lr 0x2faf38 (r1 = 1), 0x2fac7c, 0x256a14
// from lr 0x2face8 (r1 = 1), nothing else (docs/re/cutscenes.md).
void endFadesAreReadFromTheCode() {
    using chase::runtime::CutsceneFadeRequest;
    CutsceneNativeState state;
    state.setLengthSource(lengths());
    state.play("long", {true, true, true, true});
    state.update(chase::runtime::kCutsceneFadeSeconds + 0.001f, false);
    require(state.shown() && state.takeModeEntered() && !state.takeModeEntered(), "the cutscene mode is entered once, at the end of the fade");
    require(state.takeFadeRequest() == CutsceneFadeRequest::None, "no fade request while the cutscene plays");
    state.update(4.9f, false);
    require(state.shown() && state.takeFadeRequest() == CutsceneFadeRequest::None, "the cutscene plays up to its length");
    state.update(0.2f, false);
    require(state.shown() && state.playing()->elapsed == 5.0f, "at the end the cutscene is still the picture, held on its last time");
    require(state.takeFadeRequest() == CutsceneFadeRequest::Out && state.takeFadeRequest() == CutsceneFadeRequest::None,
            "the update asks for one fade out");
    state.update(0.25f, true);
    state.update(0.25f, true);
    require(state.shown() && state.busy("long"), "the mode switch waits while the fade runs");
    state.update(1.0f / 60.0f, false);
    state.update(chase::runtime::kCutsceneExitWaitSeconds / 2, false);
    require(state.busy("long") && state.takeFadeRequest() == CutsceneFadeRequest::None, "and two render ticks after it");
    state.update(chase::runtime::kCutsceneExitWaitSeconds / 2, false);
    require(state.playing() == nullptr && !state.busy("long"), "the exit ends the cutscene");
    require(state.takeFadeRequest() == CutsceneFadeRequest::In, "and fades in");
}

void noExitFadeForTheLastCutscene() {
    using chase::runtime::CutsceneFadeRequest;
    CutsceneNativeState state;
    state.setLengthSource([](const std::string&) -> std::optional<float> { return 1.0f; });
    state.play("7_9_a", {false, true, true, true});
    state.update(1.0f, false);
    state.takeFadeRequest();
    state.update(0.1f, false);
    state.update(1.0f, false);
    require(state.playing() == nullptr && state.takeFadeRequest() == CutsceneFadeRequest::None, "0x25ad90 finds the 7_9_A path: no fade in at the exit");
    state.play("short", {false, true, true, true});
    require(state.shown() && state.takeModeEntered(), "without flag a the cutscene starts at once");
}

// The whole sequence on the screen fade of the runner: the end fade takes 0.5 s on the top screen, black until the exit,
// then 0.5 s back; the bottom screen is never covered (docs/re/cutscenes.md: the measured end fade left it clear).
void endSequenceOnTheScreenFade() {
    using chase::runtime::DisplayScreen;
    Level level;
    add(level, "s", {{"DCamCutScenePlay", {"\"short\"", "0", "1", "1", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengths());
    runner.start("s");
    const float step = 1.0f / 60.0f;
    for (int i = 0; i < 40; ++i) runner.update(step);
    require(runner.cutscenes().shown() && runner.screenFade().opacityOn(DisplayScreen::Top) == 0.0f, "clear while the cutscene plays");
    int guard = 0;
    while (!runner.screenFade().running() && ++guard < 100) runner.update(step);
    require(runner.cutscenes().playing() != nullptr && runner.screenFade().running(), "the end fade runs while the cutscene is still the picture");
    require(runner.screenFade().opacityOn(DisplayScreen::Bottom) == 0.0f, "the bottom screen stays clear");
    guard = 0;
    while (runner.cutscenes().playing() && ++guard < 200) runner.update(step);
    require(runner.cutscenes().playing() == nullptr, "the exit came after the fade and the wait");
    require(runner.screenFade().opacityOn(DisplayScreen::Top) == 1.0f, "black when the exit starts the fade in");
    require(runner.screenFade().opacityOn(DisplayScreen::Bottom) == 0.0f, "and the bottom screen is still clear");
    for (int i = 0; i < 31; ++i) runner.update(step);
    require(runner.screenFade().opacityOn(DisplayScreen::Top) == 0.0f, "clear 0.5 s later");
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
    require(runner.variable("Done") == 0, "still waiting through the end fade");
    for (int i = 0; i < 30; ++i) runner.update(1.0f / 30.0f);
    require(runner.variable("Done") == 1 && !runner.cutscenes().loaded("long"), "Wait ends at the exit after the length and Free follows");
}

void waitForDCamCountsTheCutscene() {
    Level level;
    add(level, "s", {{"DCamCutScenePlay", {"\"short\"", "0", "0", "0", "0"}}, {"WaitForDCam", {}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengths());
    runner.start("s");
    require(runner.variable("Done") == 0, "0x2297b0 with name 0 counts any playing cutscene");
    for (int i = 0; i < 60; ++i) runner.update(1.0f / 30.0f);
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
        endFadesAreReadFromTheCode();
        noExitFadeForTheLastCutscene();
        endSequenceOnTheScreenFade();
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
