// Cutscene natives of batch SCRIPT-29 (src/runtime/natives_cutscene.cpp, ScriptRunner). The rules come from the handlers
// DCamCutSceneLoad 0x2889a4 / 0x196390, DCamCutScenePlay 0x2889e4 / 0x1964dc, DCamCutSceneWait 0x288a40 (returns the
// negated busy test 0x2297b0), DCamCutSceneFree 0x28898c / 0x196144, PlayCutscene 0x1bf9dc (docs/re/cutscenes.md,
// entry on the cutscene natives). The scripts and lengths here are made up.
#include "runtime/script_runner.hpp"
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
        waitBlocksForTheLength();
        waitForDCamCountsTheCutscene();
        movieBlocksUntilFinished();
        movieWithoutHostEndsAtOnce();
        noFadeBackIn();
    } catch (const std::exception& error) {
        std::cerr << "natives-cutscene-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-cutscene-test: ok\n";
    return 0;
}
