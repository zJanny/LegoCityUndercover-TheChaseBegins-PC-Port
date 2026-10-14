// AI core and the AI natives (src/runtime/ai.cpp, docs/re/ai-natives.md). The flag bits, the requested state numbers and
// the argument handling come from the disassembly of the handlers (addresses next to each check); the scripts and
// positions are made up. The walk speed is the one measured in the oracle at the police entry (docs/re/scripts.md, 0.11
// per update of 1/30 s).
#include "runtime/ai.hpp"
#include "runtime/script_runner.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;
using chase::assets::Level;
using chase::assets::LevelPath;
using chase::assets::LevelScript;
using chase::assets::LevelTrigger;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-3f) { return std::fabs(a - b) <= tolerance; }

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

AiCore withPositions() {
    AiCore core;
    core.setPositionLookup([](const std::string& name, float out[3]) {
        if (name == "npc") { out[0] = 0; out[1] = 0; out[2] = 0; return true; }
        if (name == "target") { out[0] = 3; out[1] = 0; out[2] = 4; return true; }
        if (name == "player1") { out[0] = 1; out[1] = 0; out[2] = 1; return true; }
        return false;
    });
    return core;
}

void supportClasses() {
    for (const std::string& name : aiNativeNames()) require(scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name);
    require(aiNativeNames().size() == 16, "16 natives here, LockCharacter is a progress native");
    require(scriptFunctionSupport("LockCharacter") == ScriptFunctionSupport::StandIn, "LockCharacter");
}

void flagNatives() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AIEnable, {"npc"}, {1.0f});
    const AiCharacter* npc = core.character("npc");
    require(npc && npc->enabled == true && npc->state == AiState::NoState, "AIEnable 0x1881d4: +0xd8 bit 4 set, state 1 requested (0x241304)");
    require(npc->homePosition && (*npc->homePosition)[0] == 0, "AIEnable stores the node position at +0xb8");
    core.run(AiNativeKind::AIEnable, {"npc"}, {0.0f});
    require(core.character("npc")->enabled == false, "AIEnable 0 clears the bit");
    core.run(AiNativeKind::AIIgnorePlayer, {"npc"}, {2.0f});
    require(core.character("npc")->ignorePlayer == true, "AIIgnorePlayer 0x1d9c38: bit 0 of +0xd9 for any non-zero value");
    core.run(AiNativeKind::CharacterInvulnerable, {"npc"}, {1.0f});
    require(core.character("npc")->invulnerable == true, "CharacterInvulnerable 0x2a6cfc: bit 6 of +0xd8");
    core.run(AiNativeKind::CharacterInvulnerable, {"npc"}, {2.0f});
    require(core.character("npc")->invulnerable == false, "the argument is truncated and masked with 1 (and r1, r3, r1 lsl 6)");
    core.run(AiNativeKind::AICantAttack, {"npc"}, {0.0f});
    require(core.character("npc")->canAttack == true, "AICantAttack 0x1bf528 sets bit 7 of +0xdb for an argument of 0");
    core.run(AiNativeKind::AICantAttack, {"npc"}, {1.0f});
    require(core.character("npc")->canAttack == false, "and clears it otherwise");
}

void millAboutAndActive() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AIActive, {"npc"}, {});
    require(core.character("npc")->state == AiState::Idle, "AIActive 0x1881a4 without the mill bit requests state 0 (0x2246c0)");
    core.run(AiNativeKind::AIMillAbout, {"npc"}, {1.0f});
    require(core.character("npc")->millAbout == true && core.character("npc")->state == AiState::MillAbout, "AIMillAbout 1 sets bit 7 of +0x315 and requests state 9 (0x22471c)");
    core.run(AiNativeKind::AIActive, {"npc"}, {});
    require(core.character("npc")->state == AiState::MillAbout, "AIActive with the bit set requests state 9");
    core.run(AiNativeKind::AIMillAbout, {"npc"}, {0.0f});
    require(core.character("npc")->millAbout == false && core.character("npc")->state == AiState::Idle, "AIMillAbout 0 clears the bit and requests state 0");
    core.run(AiNativeKind::AIMillAbout, {"npc"}, {-1.0f});
    require(core.character("npc")->millAbout == false, "the test is greater than 0 (orrgt / bicle)");
}

void states() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AINoState, {"npc"}, {});
    require(core.character("npc")->state == AiState::NoState, "AINoState 0x194e0c requests state 1");
    core.run(AiNativeKind::AIPursuit, {"npc", "node"}, {1.0f});
    require(core.character("npc")->state == AiState::Takedown && core.character("npc")->pursuit->takedown, "AIPursuit 0x17cbf0 with the takedown argument passes state 35 to 0x1d4688");
    core.run(AiNativeKind::AIPursuit, {"npc", "node"}, {0.0f});
    require(core.character("npc")->state == AiState::Pursuit && !core.character("npc")->pursuit->takedown, "without it state 36");
    core.run(AiNativeKind::AIResetState, {"npc"}, {});
    require(core.character("npc")->state == AiState::Idle && core.character("npc")->stateResets == 1, "AIResetState 0x1bf650 resets to idle");
    core.run(AiNativeKind::AIStalkPlayer, {"npc"}, {4.0f});
    core.run(AiNativeKind::AIRunAway, {"npc"}, {});
    require(core.character("npc")->stalkDistance == 4.0f && core.character("npc")->runningAway, "AIStalkPlayer and AIRunAway record their request");
    core.run(AiNativeKind::AISuperJump, {"npc", "target"}, {2.0f});
    require(core.character("npc")->superJump && core.character("npc")->superJump->height == 2.0f && core.character("npc")->superJump->target[2] == 4.0f, "AISuperJump 0x1b2344 hands the target position and the height to 0x213300");
}

void faceTarget() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AIFaceTarget, {"npc", "target"}, {});
    require(core.character("npc")->state == AiState::FaceTarget && core.character("npc")->faceTarget == "target", "AIFaceTarget 0x1bf570 on a character that is not the player requests state 10 (0x211110)");
    core.run(AiNativeKind::AIFaceTarget, {"player1", "target"}, {});
    require(core.character("player1")->state == AiState::Idle && core.character("player1")->heading, "a player whose AI is not enabled gets the heading directly (+10)");
    const float expected = std::atan2(2.0f, 3.0f) * 10430.378f;
    require(*core.character("player1")->heading == static_cast<std::uint16_t>(static_cast<std::int32_t>(expected)), "heading = atan2(dx, dz) * 65536 / 2 pi (literal 0x4622f983)");
}

void moveWalksAtTheMeasuredSpeedAndEnds() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AIMove, {"npc", "target"}, {0.0f});
    require(core.moving("npc") && core.character("npc")->state == AiState::MoveTo, "AIMove starts a move");
    core.update(1.0f / 30.0f);
    const AiCharacter* npc = core.character("npc");
    require(near((*npc->position)[0], 0.11f * 0.6f) && near((*npc->position)[2], 0.11f * 0.8f), "0.11 per update towards the target (oracle)");
    require(core.takeMovesEnded().empty(), "not at the target yet");
    for (int i = 0; i < 60; ++i) core.update(1.0f / 30.0f);
    const std::vector<AiMoveEnd> ended = core.takeMovesEnded();
    require(ended.size() == 1 && ended[0].character == "npc" && ended[0].argument == 0 && !core.moving("npc"), "the move ends once at the target");
    require(core.character("npc")->state == AiState::Idle && near((*core.character("npc")->position)[2], 4.0f), "idle again, standing on the target");
}

void runIsFaster() {
    AiCore walking = withPositions(), running = withPositions();
    walking.run(AiNativeKind::AIMove, {"npc", "target"}, {0.0f});
    running.run(AiNativeKind::AIMoveNoAvoid, {"npc", "target"}, {1.0f});
    walking.update(0.1f);
    running.update(0.1f);
    require((*running.character("npc")->position)[2] > (*walking.character("npc")->position)[2], "run argument 1");
    require(running.character("npc")->move->noAvoid && !walking.character("npc")->move->noAvoid, "AIMoveNoAvoid records the avoid flag");
}

SimplePathFollow route(bool closed) {
    SimplePathFollow follow;
    follow.nodes = {{0, 0, 1}, {0, 0, 2}, {0, 0, 3}};
    follow.closed = closed;
    return follow;
}

void simplePathIndexRules() {
    std::uint16_t next = 99;
    require(nextSimplePathIndex(0, 3, false, false, next) && next == 1, "forward: index + 1 (0x2cbecc)");
    require(!nextSimplePathIndex(2, 3, false, false, next), "an open path ends after its last node (0x2cbee0 branches to 0x2cbef8)");
    require(nextSimplePathIndex(2, 3, false, true, next) && next == 0, "a closed path wraps forward to index + 1 - count");
    require(nextSimplePathIndex(2, 3, true, false, next) && next == 1, "reverse: index - 1 (0x2cbea4)");
    require(!nextSimplePathIndex(0, 3, true, false, next), "an open path ends before node 0 in reverse");
    require(nextSimplePathIndex(0, 3, true, true, next) && next == 1, "a closed path wraps in reverse to count - 2 (sub 1, sub 1, add count)");
}

void simplePathStepRules() {
    SimplePathFollow follow = route(false);
    std::uint16_t heading = 0;
    require(stepSimplePath(follow, {0, 0, 0}, heading) == SimplePathResult::Moving && heading == 0 && follow.index == 0, "node 0 is 1 away: moving, heading atan2(0, 1) = 0");
    require(stepSimplePath(follow, {1, 0, 0}, heading) == SimplePathResult::Moving && heading == static_cast<std::uint16_t>(static_cast<std::int32_t>(std::atan2(-1.0f, 1.0f) * 10430.378f)), "heading toward the node, u16 of the truncated angle (+6)");
    require(stepSimplePath(follow, {0, 5, 0.5f}, heading) == SimplePathResult::Moving, "exactly 0.5 away is not arrival (cmp r0, #0x3f000000; bge keeps moving)");
}

void simplePathAdvancesAndEnds() {
    SimplePathFollow follow = route(false);
    std::uint16_t heading = 0;
    require(stepSimplePath(follow, {0, 0, 0.6f}, heading) == SimplePathResult::Advanced && follow.index == 1, "0.4 from node 0: advance to node 1");
    require(stepSimplePath(follow, {0, 0, 1.51f}, heading) == SimplePathResult::Advanced && follow.index == 2, "0.49 from node 1: advance to node 2");
    require(stepSimplePath(follow, {0, 0, 2.6f}, heading) == SimplePathResult::Ended, "0.4 from the last node of an open path: ended");
}

void followSimplePath() {
    AiCore core = withPositions();
    core.setPathLookup([&](const std::string& name, SimplePathFollow& out) {
        if (name == "route") { out = route(false); return true; }
        if (name == "loop") { out = route(true); return true; }
        return false;
    });
    core.run(AiNativeKind::AIFollowSimplePath, {"npc", "route"}, {-1.0f, 0.0f, 1.0f});
    require(core.character("npc")->state == AiState::FollowPath && core.character("npc")->move->follow->index == 1 && core.character("npc")->move->run, "AIFollowSimplePath 0x285d18: start node -1 of 3 points is node 1 (count - 1 + node); run 1");
    core.run(AiNativeKind::AIFollowSimplePath, {"npc", "route"}, {1.0f, 1.0f, 0.0f});
    require(core.character("npc")->move->follow->index == 1 && core.character("npc")->move->follow->reverse && !core.character("npc")->move->run, "reverse flag, walk");
    core.run(AiNativeKind::AIFollowSimplePath, {"npc", "route"}, {0.0f, 0.0f, 0.0f});
    for (int i = 0; i < 300 && core.moving("npc"); ++i) core.update(1.0f / 30.0f);
    const std::vector<AiMoveEnd> ended = core.takeMovesEnded();
    require(ended.size() == 1 && ended[0].character == "npc" && ended[0].argument == 255, "the end of an open path raises the AIMove event with argument 255 (0x2cbf10)");
    require(core.character("npc")->state == AiState::Idle, "and requests idle (0x218b38 with state 0)");
    const float z = (*core.character("npc")->position)[2];
    require(z >= 2.5f - 0.001f && z < 3.0f, "the character stops within 0.5 of the last node, not on it (walk step 0.11: first position below 0.5)");
    core.run(AiNativeKind::AIFollowSimplePath, {"npc", "loop"}, {0.0f, 0.0f, 1.0f});
    for (int i = 0; i < 600; ++i) core.update(1.0f / 30.0f);
    require(core.moving("npc") && core.takeMovesEnded().empty(), "a closed path never ends");
}

void unknownTargetIsRecorded() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AIMove, {"npc", "nowhere"}, {0.0f});
    require(!core.moving("npc") && core.unresolvedTargets().size() == 1, "an unresolved target starts no move and is listed");
}

void aiMoveFiresTheTriggerOfTheCharacter() {
    Level level;
    LevelTrigger first, second, other;
    first.event = "AIMove";
    first.objectName = "NPC";
    first.scriptName = "OnArrive";
    second = first;
    second.scriptName = "OnArriveAgain";
    other = first;
    other.objectName = "someone";
    other.scriptName = "Wrong";
    level.triggers = {other, first, second};
    add(level, "go", {{"AIMove", {"NPC", "Target", "0"}}});
    add(level, "OnArrive", {{"=", {"Arrived", "1"}}});
    add(level, "OnArriveAgain", {{"=", {"ArrivedAgain", "1"}}});
    add(level, "Wrong", {{"=", {"WrongFired", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.setObjectPositionSource([](const std::string& name, float out[3]) {
        out[0] = out[1] = 0;
        out[2] = name == "target" ? 0.2f : 0.0f;
        return name == "target" || name == "npc";
    });
    const float player[3] = {100, 0, 100};
    runner.start("go");
    require(runner.ai().moving("npc"), "the script call starts the move and continues at once (the handler returns 1)");
    for (int i = 0; i < 4; ++i) {
        runner.update(1.0f / 30.0f);
        runner.checkTriggers(player);
    }
    runner.update(1.0f / 30.0f);
    require(runner.variable("Arrived") == 1.0f, "the AIMove trigger of the character started its script (0x23cee8 -> 0x21a560)");
    require(runner.variable("ArrivedAgain") == 0.0f && runner.variable("WrongFired") == 0.0f, "only the first enabled trigger of that type and owner fires, and no other owner's");
    const float home[3] = {0, 0, 0};
    runner.setTriggerEnabled(1, false);
    runner.ai().setPosition("npc", home);
    runner.start("go");
    for (int i = 0; i < 4; ++i) {
        runner.update(1.0f / 30.0f);
        runner.checkTriggers(player);
    }
    runner.update(1.0f / 30.0f);
    require(runner.variable("ArrivedAgain") == 1.0f, "a disabled trigger is skipped and the next one of the owner fires");
}
}

int main() {
    try {
        supportClasses();
        flagNatives();
        millAboutAndActive();
        states();
        faceTarget();
        moveWalksAtTheMeasuredSpeedAndEnds();
        runIsFaster();
        simplePathIndexRules();
        simplePathStepRules();
        simplePathAdvancesAndEnds();
        followSimplePath();
        unknownTargetIsRecorded();
        aiMoveFiresTheTriggerOfTheCharacter();
    } catch (const std::exception& error) {
        std::cerr << "ai-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-test: ok\n";
    return 0;
}
