// Sound natives PlaySound 0x1b4394 (0x23fa54(word, null, (int)uid, 0)), PlaySoundGO 0x295244 (uid below 0: 0x23ae84, else
// 0x23fb78 position and 0x23fa54 with the uid), PlaySoundAtObject 0x27bd48 (0x23ae84), StopSound 0x1b43ec and StopSoundGO
// 0x1d0d28 (0x235028 / 0x23f75c(bank, id, owner, fade)), registersound 0x1cdbdc (0x281fc4 appends the pair),
// ScaleSoundFrequency 0x2a8afc (0x22f770), SetSoundVolume 0x272dcc (0x219178 -> 0x227ff4), vehiclestopsounds 0x27c148 and
// VehicleEnableDrivingMusic 0x2bf2b4 (bit 2 of component byte +0x312). Values come from llvm-objdump of game.arm.elf
// (docs/re/script-natives-audio.md). The scripts are made up.
#include "runtime/script_runner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::ScriptFunctionSupport;
using chase::runtime::ScriptRunner;
using chase::runtime::SoundRequest;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(script);
}

void positionSource(ScriptRunner& runner) {
    runner.setObjectPositionSource([](const std::string& name, float out[3]) {
        if (name == "crate") {
            out[0] = 1.0f;
            out[1] = 2.0f;
            out[2] = 3.0f;
            return true;
        }
        return false;
    });
}

void playFamily() {
    Level level;
    add(level, "s", {{"PlaySound", {"285", "9"}}, {"PlaySoundGO", {"64", "Crate", "-1"}}, {"PlaySoundGO", {"64", "Crate", "5"}},
                     {"PlaySoundAtObject", {"Crate", "210"}}, {"PlaySoundGO", {"64", "Missing", "5"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    positionSource(runner);
    runner.start("s");
    const std::vector<SoundRequest> requests = runner.soundNatives().takeRequests();
    require(requests.size() == 4, "four plays, the one at an unknown object is dropped");
    require(requests[0].kind == SoundRequest::Kind::Play && requests[0].id == 285 && requests[0].owner == 9 && !requests[0].positional, "PlaySound is flat with the uid as owner");
    require(requests[1].id == 64 && requests[1].positional && requests[1].position[1] == 2.0f && requests[1].owner == chase::runtime::kObjectOwnerBase, "PlaySoundGO with uid -1 plays at the object with the object's owner");
    require(requests[2].positional && requests[2].owner == 5, "PlaySoundGO with uid 5 plays at the object with owner 5");
    require(requests[3].id == 210 && requests[3].positional && requests[3].owner == chase::runtime::kObjectOwnerBase, "PlaySoundAtObject plays the second argument at the first (same owner as the object)");
    require(runner.soundNatives().unresolvedObjects() == 1, "unknown object counted");
}

void stopFamily() {
    Level level;
    add(level, "s", {{"PlaySoundAtObject", {"Crate", "210"}}, {"StopSound", {"285", "9", "-1"}}, {"StopSoundGO", {"210", "Crate", "0.5"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    positionSource(runner);
    runner.start("s");
    const std::vector<SoundRequest> requests = runner.soundNatives().takeRequests();
    require(requests.size() == 3, "play and two stops");
    require(requests[1].kind == SoundRequest::Kind::Stop && requests[1].id == 285 && requests[1].owner == 9 && requests[1].value == -1.0f, "StopSound: id, uid, fade");
    require(requests[2].kind == SoundRequest::Kind::Stop && requests[2].id == 210 && requests[2].owner == requests[0].owner && requests[2].value == 0.5f, "StopSoundGO stops with the owner of the object");
}

void voiceControls() {
    Level level;
    add(level, "s", {{"SetSoundVolume", {"12", "3", "0.5"}}, {"ScaleSoundFrequency", {"12", "3", "2"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const std::vector<SoundRequest> requests = runner.soundNatives().takeRequests();
    require(requests.size() == 2 && requests[0].kind == SoundRequest::Kind::SetVolume && requests[0].id == 12 && requests[0].owner == 3 && requests[0].value == 0.5f, "SetSoundVolume");
    require(requests[1].kind == SoundRequest::Kind::SetFrequency && requests[1].value == 2.0f, "ScaleSoundFrequency");
}

void bookkeeping() {
    Level level;
    add(level, "s", {{"registersound", {"296", "1"}}, {"RegisterSound", {"296", "1"}}, {"RegisterSound", {"296", "257"}}, {"RegisterSound", {"210", "2"}},
                     {"VehicleEnableDrivingMusic", {"Car", "1"}}, {"VehicleEnableDrivingMusic", {"Bus", "1"}}, {"VehicleEnableDrivingMusic", {"Bus", "0"}},
                     {"VehicleStopSounds", {"Car"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto& registered = runner.soundNatives().registered();
    require(registered.size() == 2 && registered[0] == std::make_pair(std::uint16_t(296), std::uint8_t(1)) && registered[1].first == 210,
            "registersound keeps each (id, scene & 255) pair once (0x281fc4 compares both)");
    require(runner.soundNatives().drivingMusicEnabled("car") && !runner.soundNatives().drivingMusicEnabled("bus"), "driving music flag per vehicle");
    require(runner.soundNatives().vehicleSoundStops().size() == 1 && runner.soundNatives().vehicleSoundStops()[0] == "car", "vehiclestopsounds recorded");
    require(runner.soundNatives().takeRequests().empty(), "no audio request from the bookkeeping natives");
}

void support() {
    for (const std::string& name : chase::runtime::soundNativeNames())
        require(chase::runtime::scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name + " counts as a stand-in");
    require(chase::runtime::soundNativeNames().size() == 10, "ten sound natives");
}
}

int main() {
    try {
        playFamily();
        stopFamily();
        voiceControls();
        bookkeeping();
        support();
    } catch (const std::exception& error) {
        std::cerr << "natives-sound: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-sound: ok\n";
    return 0;
}
