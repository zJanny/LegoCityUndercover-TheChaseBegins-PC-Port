// What an active director camera does to the characters (src/runtime/director_hold.cpp, docs/re/dcam.md): the control routines
// 0x26be04 and 0x29bf30 read from the disassembly, and the AI gate that stops a held character.
#include "runtime/ai.hpp"
#include "runtime/dcam.hpp"
#include "runtime/director_hold.hpp"
#include "runtime/script_runner.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::runtime::AiCore;
using chase::runtime::AiNativeKind;
using chase::runtime::CharacterControl;
using chase::runtime::DirectorCamera;
using chase::runtime::characterControl;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void normalControl() {
    // Provenance: llvm-objdump of 0x26be04 (docs/re/dcam.md): the player reads the pad unless bit 4 of +0xd8 is set; anybody else runs 0x200c30.
    require(characterControl(false, true, false, false) == CharacterControl::Pad, "player reads the pad");
    require(characterControl(false, true, true, false) == CharacterControl::Ai, "player under AI control runs the AI");
    require(characterControl(false, false, false, false) == CharacterControl::Ai, "other characters run the AI");
}

void directorControl() {
    // Provenance: llvm-objdump of 0x29bf30: the player without AI control gets nothing even with DCamActive; everybody else
    // runs 0x200c30 only with bit 1 of +0xd9.
    require(characterControl(true, true, false, false) == CharacterControl::None, "player holds still");
    require(characterControl(true, true, false, true) == CharacterControl::None, "DCamActive does not free a player on the pad");
    require(characterControl(true, true, true, false) == CharacterControl::None, "AI player without DCamActive holds");
    require(characterControl(true, true, true, true) == CharacterControl::Ai, "AI player with DCamActive runs the AI");
    require(characterControl(true, false, false, false) == CharacterControl::None, "other character holds");
    require(characterControl(true, false, false, true) == CharacterControl::Ai, "other character with DCamActive runs the AI");
}

void aiGateStopsAMove() {
    AiCore ai;
    const float start[3] = {0, 0, 0};
    ai.setPosition("walker", start);
    ai.setPosition("bystander", start);
    ai.setPositionLookup([](const std::string&, float out[3]) {
        out[0] = 10;
        out[1] = 0;
        out[2] = 0;
        return true;
    });
    ai.run(AiNativeKind::AIMove, {"walker", "target"}, {});
    ai.run(AiNativeKind::AIMove, {"bystander", "target"}, {});
    bool walkerRuns = false;
    ai.setUpdateGate([&](const std::string& name) { return name == "bystander" || walkerRuns; });
    ai.update(0.033f);
    require(ai.movedLastUpdate("bystander") && !ai.movedLastUpdate("walker"), "a gated character does not move");
    walkerRuns = true;
    ai.update(0.033f);
    require(ai.movedLastUpdate("walker"), "a character moves once the gate opens");
}

chase::assets::LevelPath path(const std::string& name, std::vector<std::array<float, 3>> points) {
    chase::assets::LevelPath p;
    p.nameHash = chase::assets::nameHash(name);
    p.points = std::move(points);
    return p;
}

void directorHoldsWhileActive() {
    DirectorCamera director;
    require(!director.holdsControl(), "no hold without a director camera");
    const chase::assets::LevelPath eye = path("eye", {{0, 0, 0}});
    const chase::assets::LevelPath look = path("look", {{0, 0, 5}});
    director.startPaths(&eye, &look, 0.1f, 0);
    require(!director.holdsControl(), "the characters of the update that starts the camera are not held yet");
    director.update(0.033f, nullptr);
    require(director.holdsControl(), "held from the update after the start");
    int heldUpdates = 1;
    while (director.busy()) {
        director.update(0.033f, nullptr);
        heldUpdates += director.holdsControl();
    }
    require(director.holdsControl(), "held in the update after the end test");
    director.update(0.033f, nullptr);
    require(!director.holdsControl() && heldUpdates > 1, "free two updates after the end test");
}

void runnerHoldsThePlayerAndTheCharactersDuringAScriptedCamera() {
    // Provenance: StartDCam sets the camera slot 0x37689c (0x189548) and the end test 0x196784 switches it back, the character
    // routine 0x1835d0 reads it; oracle: the save state in front of the police car of mission 1-1, controls zeroed from 3 updates after Drive begins (docs/re/dcam.md).
    chase::assets::Level level;
    level.paths.push_back(path("camera", {{0, 0, 0}, {10, 0, 0}}));
    level.paths.push_back(path("target", {{0, 0, 5}}));
    chase::assets::LevelScript script;
    script.nameHash = chase::assets::nameHash("s");
    script.calls = {{"DCamActive", {"Free", "1"}}, {"StartDCam", {"Camera", "Target", "0", "0.1", "0"}}, {"WaitForDCam", {}}};
    level.scripts.push_back(script);
    chase::assets::PlacedObjectState state;
    chase::runtime::ScriptRunner runner(level, state);
    runner.ai().setPositionLookup([](const std::string&, float out[3]) {
        out[0] = 10;
        out[1] = 0;
        out[2] = 0;
        return true;
    });
    const float origin[3] = {0, 0, 0};
    runner.ai().setPosition("free", origin);
    runner.ai().setPosition("held", origin);
    runner.ai().run(AiNativeKind::AIMove, {"free", "goal"}, {});
    runner.ai().run(AiNativeKind::AIMove, {"held", "goal"}, {});
    require(!runner.playerControlTaken(), "free before the script");
    runner.start("s");
    require(!runner.playerControlTaken(), "not held in the update that starts the camera");
    runner.director().update(0.033f, nullptr);
    require(runner.playerControlTaken(), "the player is held from the next update");
    runner.update(0.033f);
    require(runner.ai().movedLastUpdate("free") && !runner.ai().movedLastUpdate("held"), "only the DCamActive character keeps moving");
    int updates = 0;
    while (runner.director().busy() && updates++ < 100) {
        runner.director().update(0.033f, nullptr);
        runner.update(0.033f);
    }
    require(runner.playerControlTaken(), "still held in the update after the end test");
    runner.director().update(0.033f, nullptr);
    runner.update(0.033f);
    require(!runner.playerControlTaken(), "the player is free after that");
    require(runner.ai().movedLastUpdate("held"), "the other character moves again");
}
}

int main() {
    try {
        normalControl();
        directorControl();
        aiGateStopsAMove();
        directorHoldsWhileActive();
        runnerHoldsThePlayerAndTheCharactersDuringAScriptedCamera();
    } catch (const std::exception& error) {
        std::cerr << "director-hold-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "director-hold-test: ok\n";
    return 0;
}
