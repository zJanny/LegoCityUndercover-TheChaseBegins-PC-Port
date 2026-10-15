// Music natives OverrideMusic 0x1cdb28 (-> 0x240ec8(id, 1, 1, 0.5)), AutomaticMusic 0x1d9c70 (0x1c29ac clears byte 0x376b31) and
// StopMusic 0x195054 (0.0: 0x21cc04(1); otherwise 0x240f6c(1 / (30 * seconds)) with the 0.02 fallback of 0x240f8c). Values come
// from llvm-objdump of game.arm.elf (docs/re/script-natives-audio.md). The scripts are made up.
#include "runtime/script_runner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::MusicRequest;
using chase::runtime::ScriptFunctionSupport;
using chase::runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(script);
}

void overrideAndAutomatic() {
    Level level;
    add(level, "s", {{"OverrideMusic", {"4"}}, {"OverrideMusic", {"65540"}}, {"AutomaticMusic", {}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const std::vector<MusicRequest> requests = runner.musicNatives().takeRequests();
    require(requests.size() == 2, "two OverrideMusic starts");
    for (const MusicRequest& request : requests)
        require(request.kind == MusicRequest::Kind::Start && request.track == 4 && request.loop && request.crossfade && request.fade == 0.5f,
                "OverrideMusic starts the row looping with crossfade and fade 0.5 (u16 id)");
    require(!runner.musicNatives().overrideActive(), "AutomaticMusic clears the override byte");

    Level only;
    add(only, "s", {{"OverrideMusic", {"4"}}});
    ScriptRunner overridden(only, state);
    overridden.start("s");
    require(overridden.musicNatives().overrideActive(), "OverrideMusic sets the override byte");
}

void stopMusic() {
    Level level;
    add(level, "s", {{"StopMusic", {"0"}}, {"StopMusic", {"2"}}, {"StopMusic", {"-1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const std::vector<MusicRequest> requests = runner.musicNatives().takeRequests();
    require(requests.size() == 3, "three StopMusic requests");
    require(requests[0].kind == MusicRequest::Kind::StopNow, "0 stops now");
    require(requests[1].kind == MusicRequest::Kind::FadeOut && requests[1].step == 1.0f / 60.0f, "2 seconds: step 1 / 60");
    require(requests[2].kind == MusicRequest::Kind::FadeOut && requests[2].step == 0.02f, "a negative time uses the 0.02 fallback");
}

void support() {
    for (const char* name : {"OverrideMusic", "AutomaticMusic", "StopMusic"})
        require(chase::runtime::scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name);
    chase::runtime::MusicNativeState bounded;
    bounded.setTrackCount(39);
    bounded.run(chase::runtime::MusicNativeKind::OverrideMusic, {39.0f});
    require(bounded.outOfRangeTracks() == 1 && bounded.takeRequests().empty() && !bounded.overrideActive(), "a track past the table is ignored");
}
}

int main() {
    try {
        overrideAndAutomatic();
        stopMusic();
        support();
    } catch (const std::exception& error) {
        std::cerr << "natives-music: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-music: ok\n";
    return 0;
}
