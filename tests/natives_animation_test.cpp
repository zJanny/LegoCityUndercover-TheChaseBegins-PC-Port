// Synthetic scripts for the animation and attach natives of batch SCRIPT-32 (src/runtime/natives_animation.cpp). The argument
// layout comes from the handlers' disassembly (docs/re/script-natives-animation.md, entry of 2026-10-14): PlayCharAnim 0x29e5f0
// (called through 0x1bf9cc), PlayAnimation 0x268240, LoadAnimation 0x2681a0, UnloadAnimation 0x27e9c4, StopCharAnim 0x1bfb84,
// StopAnim 0x1883d0, WaitForCurrentAnim 0x2a06a8, WaitForCurrentCharAnim 0x263ae4, AttachObject 0x1bf68c, AttachToBone
// 0x1ddb90, AttachToBoneRot 0x1bf718, AttachToWorldRoot 0x2951ec, AttachToRoom 0x1ddc88. The scripts are made up.
#include "runtime/script_runner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::AttachKind;
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

void censusIsStandIn() {
    for (const char* name : {"playcharanim", "playanimation", "loadanimation", "unloadanimation", "stopcharanim", "stopanim",
                             "waitforcurrentanim", "waitforcurrentcharanim", "attachobject", "attachtobone", "attachtobonerot",
                             "attachtoworldroot", "attachtoroom"})
        require(chase::runtime::scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name);
}

void playAndStop() {
    Level level;
    add(level, "s", {{"LoadAnimation", {"Dog_Cage", "\"open\"", "\"\"", "1"}},
                     {"PlayAnimation", {"Dog_Cage", "\"open\"", "1", "0", "0"}},
                     {"PlayCharAnim", {"Dogman", "\"crying_idle_minifig\"", "\"CLASS_Generic\"", "1"}},
                     {"PlayCharAnim", {"Chase", "\"idle_minifig\"", "\"class_generic\"", "0"}},
                     {"WaitForCurrentCharAnim", {"Chase"}},
                     {"UnloadAnimation", {"Dog_Cage", "\"kick_open\""}},
                     {"StopCharAnim", {"Chase"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("s") && runner.threadCount() == 0, "the animation natives never block");
    const auto* cage = runner.animation().current("dog_cage");
    require(cage && cage->name == "open" && !cage->character && cage->speed == 1.0f && !cage->loop && cage->blend == 0.0f, "PlayAnimation");
    require(runner.animation().loaded("dog_cage", "open") && !runner.animation().loaded("dog_cage", "kick_open"), "LoadAnimation");
    const auto* dog = runner.animation().current("dogman");
    require(dog && dog->name == "crying_idle_minifig" && dog->classFolder == "class_generic" && dog->character && dog->loop, "PlayCharAnim loops for 1");
    require(runner.animation().current("chase") == nullptr && runner.animation().stopRequests("chase") == 1, "StopCharAnim");
    require(runner.animation().waitCalls() == 1, "WaitForCurrentCharAnim");
    require(runner.unhandledFunctions().empty(), "all handled");
}

void replacingTheAnimationCountsRequests() {
    Level level;
    add(level, "s", {{"PlayCharAnim", {"A", "\"one\"", "\"c\"", "0"}},
                     {"PlayCharAnim", {"A", "\"two\"", "\"c\"", "1"}},
                     {"UnloadAnimation", {"A", "\"never\""}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto* now = runner.animation().current("a");
    require(now && now->name == "two" && now->requests == 2 && now->loop, "the second request replaces the first");
}

void attachNatives() {
    Level level;
    add(level, "s", {{"AttachObject", {"Mower", "Sfx"}},
                     {"AttachToBone", {"Giver", "Gun", "\"weaponRight\"", "\"handle\""}},
                     {"AttachToBoneRot", {"Man", "Boom", "\"weaponright\"", "60", "0", "-5"}},
                     {"AttachToWorldRoot", {"Crane"}},
                     {"AttachToRoom", {"Crow"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto* mower = runner.animation().attachment("mower");
    require(mower && mower->kind == AttachKind::Object && mower->parent == "sfx", "AttachObject");
    const auto* giver = runner.animation().attachment("giver");
    require(giver && giver->kind == AttachKind::Bone && giver->parent == "gun" && giver->childBone == "weaponright" && giver->parentBone == "handle", "AttachToBone");
    const auto* man = runner.animation().attachment("man");
    require(man && man->kind == AttachKind::BoneRotated && man->rotationDegrees[0] == 60.0f && man->rotationDegrees[2] == -5.0f, "AttachToBoneRot");
    const auto* crane = runner.animation().attachment("crane");
    require(crane && crane->kind == AttachKind::WorldRoot, "AttachToWorldRoot");
    const auto* crow = runner.animation().attachment("crow");
    require(crow && crow->kind == AttachKind::Room, "AttachToRoom");
}

void shortCallsAreIgnored() {
    Level level;
    add(level, "s", {{"PlayCharAnim", {"A", "\"one\""}}, {"AttachToBone", {"A", "B"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.animation().current("a") == nullptr && runner.animation().attachment("a") == nullptr, "calls with missing arguments do nothing");
}
}

int main() {
    try {
        censusIsStandIn();
        playAndStop();
        replacingTheAnimationCountsRequests();
        attachNatives();
        shortCallsAreIgnored();
    } catch (const std::exception& error) {
        std::cerr << "natives-animation-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-animation-test: ok\n";
    return 0;
}
