// AI core and the AI natives (src/runtime/ai.cpp, docs/re/ai-natives.md). The flag bits, the requested state numbers and
// the argument handling come from the disassembly of the handlers (addresses next to each check); the scripts and
// positions are made up. The walk speed is the one measured in the oracle at the police entry (docs/re/scripts.md, 0.11
// per update of 1/30 s).
#include "runtime/ai.hpp"
#include "runtime/ai_go_to_target.hpp"
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
    require(core.character("npc")->state == AiState::RunAway, "AIRunAway 0x228c70 requests state 25 (0x228c78), AIStalkPlayer 0x282b88 state 27 (0x282b94)");
    core.run(AiNativeKind::AIStalkPlayer, {"npc"}, {4.0f});
    require(core.character("npc")->state == AiState::Stalk, "AIStalkPlayer requests state 27");
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

NavWorld navCorridor() {
    chase::assets::NavGrid grid;
    grid.width = 4;
    grid.depth = 2;
    grid.heightExtent = 512;
    grid.cells.assign(8, 0);
    grid.linkSlots.fill(0xffff);
    grid.flags = 1;
    grid.extras = {{0, 0, 0, 0}};
    chase::assets::NavPath box;
    box.position[1] = 1;
    box.extent[0] = box.extent[1] = box.extent[2] = 100;
    box.indices = {0};
    chase::assets::NavMesh mesh;
    mesh.grids.push_back(grid);
    mesh.paths.push_back(box);
    chase::assets::NavRoom descriptor;
    descriptor.heightUnit = 1;
    descriptor.cellSize = 1;
    descriptor.end = 1;
    return NavWorld::fromRooms({descriptor}, {mesh});
}

AiCore navCore(float targetX) {
    AiCore core;
    core.setPositionLookup([targetX](const std::string& name, float out[3]) {
        if (name == "npc") { out[0] = 0.5f; out[1] = 1; out[2] = 0.5f; return true; }
        if (name == "target") { out[0] = targetX; out[1] = 1; out[2] = 0.5f; return true; }
        return false;
    });
    std::vector<NavWorld> worlds;
    worlds.push_back(navCorridor());
    core.setNavigation(std::move(worlds));
    return core;
}

void aiMoveFollowsTheNavGrid() {
    AiCore core = navCore(3.5f);
    core.run(AiNativeKind::AIMove, {"npc", "target"}, {0.0f});
    require(core.moving("npc") && core.character("npc")->move->nav, "the finder 0x204284 accepts a position in the same grid: a nav order (0x22d85c)");
    core.update(0.1f);
    const float step = kAiWalkSpeed * 0.1f;
    require(near((*core.character("npc")->position)[0], 0.5f + step) && near((*core.character("npc")->position)[2], 0.5f),
            "the follower returns code 4 toward the centre of the exit cell 3: heading 64 (+x), stand-in speed");
    int updates = 1;
    while (core.moving("npc") && updates < 100) { core.update(0.1f); ++updates; }
    const std::vector<AiMoveEnd> ended = core.takeMovesEnded();
    require(ended.size() == 1 && ended[0].character == "npc" && ended[0].argument == 0, "arrival raises the AIMove event once (0x2c630c)");
    // The follower returns 0 as soon as the position is in cell 3 (x >= 3): the character stops there, not on the target.
    const float x = (*core.character("npc")->position)[0];
    require(x >= 3.0f && x < 3.0f + step + 1e-3f, "arrival happens on entering the exit cell");
    require(core.character("npc")->state == AiState::Idle, "idle after the event (0x2246c0)");
}

NavWorld navTwoGrids() {
    chase::assets::NavGrid left, right;
    for (chase::assets::NavGrid* grid : {&left, &right}) {
        grid->width = 4;
        grid->depth = 2;
        grid->heightExtent = 512;
        grid->cells.assign(8, 0);
        grid->linkSlots.fill(0xffff);
        grid->flags = 2;
    }
    right.x = 4;
    left.linkSlots[0] = 101;
    right.linkSlots[0] = 100;
    left.extras = {{0, 0, 1, 0}, {0, 0, 1, 0}};
    right.extras = {{0, 0, 0xff, 0}, {0, 0, 0xff, 0}};
    left.cells[3] = 4;
    left.cells[7] = 12;
    right.cells[0] = 4;
    right.cells[4] = 12;
    chase::assets::NavPath box;
    box.position[0] = box.position[2] = 50;
    box.position[1] = 1;
    box.extent[0] = box.extent[1] = box.extent[2] = 100;
    box.indices = {0, 1};
    chase::assets::NavMesh mesh;
    mesh.grids = {left, right};
    mesh.paths.push_back(box);
    chase::assets::NavRoom descriptor;
    descriptor.heightUnit = 1;
    descriptor.cellSize = 1;
    descriptor.base = 100;
    descriptor.end = 102;
    return NavWorld::fromRooms({descriptor}, {mesh});
}

void aiMoveCrossesToTheNextGrid() {
    AiCore core;
    core.setPositionLookup([](const std::string& name, float out[3]) {
        if (name == "npc") { out[0] = 0.5f; out[1] = 1; out[2] = 0.5f; return true; }
        if (name == "target") { out[0] = 7.5f; out[1] = 1; out[2] = 1.5f; return true; }
        return false;
    });
    std::vector<NavWorld> worlds;
    worlds.push_back(navTwoGrids());
    core.setNavigation(std::move(worlds));
    core.run(AiNativeKind::AIMove, {"npc", "target"}, {1.0f});
    require(core.moving("npc") && core.character("npc")->move->nav, "a path across two grids is a nav order (finder result 3, phase 2)");
    float highest = 0;
    int updates = 0;
    while (core.moving("npc") && updates < 400) {
        core.update(1.0f / 30.0f);
        highest = std::max(highest, (*core.character("npc")->position)[0]);
        ++updates;
    }
    const std::vector<AiMoveEnd> ended = core.takeMovesEnded();
    require(ended.size() == 1 && ended[0].character == "npc", "arrival in the goal grid raises the AIMove event once");
    require(core.character("npc")->move == std::nullopt && (*core.character("npc")->position)[0] >= 7.0f, "the character crossed from grid 100 into grid 101 and ended in cell 7");
}

void aiMoveToAnUnreachableTargetChangesNothing() {
    AiCore core = navCore(30.5f);
    core.run(AiNativeKind::AIMove, {"npc", "target"}, {0.0f});
    require(!core.moving("npc") && core.character("npc")->state == AiState::Idle, "result 1: the front end returns without a state change");
    require(core.cannotGetThere().size() == 1 && core.cannotGetThere()[0] == "npc", "\"I can't get there\" is recorded");
}

void aiMoveInTheSameCellEndsAtOnce() {
    AiCore core = navCore(0.7f);
    core.run(AiNativeKind::AIMove, {"npc", "target"}, {0.0f});
    core.update(1.0f / 30.0f);
    require(core.takeMovesEnded().size() == 1, "phase 0: the follower returns 0 on the first tick (0x1faf2c)");
}

void superJumpPlanAndFlight() {
    const SuperJumpPlan level = planSuperJump({0, 0, 0}, {3, 0, 4}, kSuperJumpNativeFrames, 2.0f);
    require(level.heading == 6711 && level.frames == 28 && near(level.horizontalPerFrame, 5.0f / 28.0f, 1e-6f) && level.climb == 2.0f,
            "0x213300 with a target at the same height keeps the 28 frames: speed 5 / 28, climb = height (hand evaluation of 0x213300..0x2133f0)");
    const SuperJumpPlan up = planSuperJump({0, 0, 0}, {0, 3, 4}, kSuperJumpNativeFrames, 2.0f);
    require(up.heading == 0 && up.frames == 22 && near(up.horizontalPerFrame, 4.0f / 22.0f, 1e-6f) && up.climb == 5.0f,
            "a target 3 above: frames = int((pi - asin(3 / 5)) * 8.912677) = 22 (hand evaluation, python), climb = 3 + 2");

    const chase::runtime::SuperJumpStep step = chase::runtime::stepSuperJumpHorizontal(level, 6711, 1.0f / 30.0f);
    require(near(step.x, 0.6f * 5.0f / 28.0f, 2e-5f) && near(step.z, 0.8f * 5.0f / 28.0f, 2e-5f),
            "0x203d18 state 73: +0x308 * 30 * dt along the facing 6711 (sin 0.6, cos 0.8; hand evaluation, 20261025-garage-C)");
    const chase::runtime::SuperJumpStep slow = chase::runtime::stepSuperJumpHorizontal(level, 16384, 1.0f / 15.0f);
    require(near(slow.x, 2.0f * 5.0f / 28.0f, 2e-5f) && near(slow.z, 0.0f, 1e-6f), "the step scales with dt (0x23602c); facing 16384 is +x");

    AiCore core = withPositions();
    core.run(AiNativeKind::AISuperJump, {"npc", "target"}, {2.0f});
    const AiCharacter* npc = core.character("npc");
    require(npc->state == AiState::GoToTarget && npc->heading && *npc->heading == 6711, "AISuperJump requests state 11 (0x2133f8) and sets the heading");
    int ticks = 0;
    while (core.character("npc")->state == AiState::GoToTarget && ticks < 100) {
        core.update(1.0f / 30.0f);
        ++ticks;
        require(core.movedLastUpdate("npc"), "the flight moves the character");
    }
    const AiCharacter* landed = core.character("npc");
    const float rest = std::sqrt((3 - (*landed->position)[0]) * (3 - (*landed->position)[0]) + (4 - (*landed->position)[2]) * (4 - (*landed->position)[2]));
    require(landed->state == AiState::Idle && ticks >= 25 && ticks <= 28 && rest < kAiGoToMinDistance, "state 11 ends when ATTR_RunSpeed (0.23) exceeds the horizontal distance (0x2c3c20)");
    const std::vector<AiMoveEnd> ended = core.takeMovesEnded();
    require(ended.size() == 1 && ended[0].character == "npc" && ended[0].argument == 0, "arrival raises the AIMove event (0x2c3c68)");
}

// RPC 20261025-garage-C-1 (20261025-garage-servant, 2026-10-25, recorded from a save state on the generator trigger spot of mission 1-3, stepped,
// the game's dt 0.033): AISuperJump(1-3_AI_Girder_Guy, 1-3_girder_Jump_2, 2) at u193 with the guy at (947.881, 17.617, 1142.456) gave
// ctrl+0x308 0.2415, ctrl+0x30c 2.0; from u194 ctrl+6 = ctrl+0xa = 44393 and the guy moved (x, z) per update as below; at u209 the script's
// AIFaceTarget (state 10) stopped the horizontal move at (944.661, 1140.876).
void superJumpTraceReplay() {
    AiCore core;
    core.setPositionLookup([](const std::string& name, float out[3]) {
        if (name == "guy") { out[0] = 947.881f; out[1] = 17.617f; out[2] = 1142.456f; return true; }
        if (name == "jump") { out[0] = 941.8098f; out[1] = 17.3054f; out[2] = 1139.4764f; return true; }
        if (name == "player1") { out[0] = 945.0f; out[1] = 17.0f; out[2] = 1150.0f; return true; }
        return false;
    });
    core.run(AiNativeKind::AISuperJump, {"guy", "jump"}, {2.0f});
    const AiCharacter* guy = core.character("guy");
    require(near(guy->superJump->plan.horizontalPerFrame, 0.2415f, 1e-4f) && guy->superJump->plan.climb == 2.0f && guy->superJump->plan.frames == 28,
            "the plan of 0x213300 equals ctrl+0x308 / +0x30c of the trace");
    const float trace[15][2] = {{947.666f, 1142.351f}, {947.452f, 1142.246f}, {947.237f, 1142.140f}, {947.022f, 1142.035f}, {946.808f, 1141.930f},
                                {946.593f, 1141.824f}, {946.378f, 1141.719f}, {946.164f, 1141.614f}, {945.949f, 1141.508f}, {945.734f, 1141.403f},
                                {945.520f, 1141.297f}, {945.305f, 1141.192f}, {945.090f, 1141.087f}, {944.876f, 1140.981f}, {944.661f, 1140.876f}};
    for (int u = 0; u < 15; ++u) {
        core.update(0.033f);
        const std::array<float, 3>& at = *core.character("guy")->position;
        require(near(at[0], trace[u][0], 2e-3f) && near(at[2], trace[u][1], 2e-3f), "flight update u" + std::to_string(194 + u) + " matches the trace");
        require(std::abs(int(*core.character("guy")->heading) - 44393) <= 1, "the heading of state 11 (and the facing ctrl+0xa) stays 44393 within one unit (the trace positions are rounded to 1e-3)");
    }
    core.run(AiNativeKind::AIFaceTarget, {"guy", "player1"}, {});
    core.update(0.033f);
    const std::array<float, 3>& held = *core.character("guy")->position;
    require(near(held[0], 944.661f, 2e-3f) && near(held[2], 1140.876f, 2e-3f), "u209: state 10 holds x and z where the jump was");
    require(core.character("guy")->superJump && core.character("guy")->superJump->released, "20261101-mission-B, SCRIPT-S318: the replaced flight is released and keeps its height going");
    core.land("guy", 16.8266f);
    require(!core.character("guy")->superJump && (*core.character("guy")->position)[1] == 16.8266f, "the host lands a released flight on the ground (oracle u217: y 16.8266)");
}

// 20261101-mission-B, SCRIPT-S318: the released flight of the girder guy (oracle u209..u216: y 19.202 falling to 17.016, x and z held) falls in the port too
// (the parabola of SCRIPT-S103 continued), keeps x and z, and AiCore::land ignores a flight that is still the character's current order.
void releasedFlightFalls() {
    AiCore core;
    core.setPositionLookup([](const std::string& name, float out[3]) {
        if (name == "guy") { out[0] = 947.881f; out[1] = 17.617f; out[2] = 1142.456f; return true; }
        if (name == "jump") { out[0] = 941.8098f; out[1] = 17.3054f; out[2] = 1139.4764f; return true; }
        if (name == "player1") { out[0] = 945.0f; out[1] = 17.0f; out[2] = 1150.0f; return true; }
        return false;
    });
    core.run(AiNativeKind::AISuperJump, {"guy", "jump"}, {2.0f});
    for (int u = 0; u < 15; ++u) core.update(0.033f);
    core.land("guy", 16.8266f);
    require(core.character("guy")->superJump && !core.character("guy")->superJump->released, "land leaves a flight that is not released");
    core.run(AiNativeKind::AIFaceTarget, {"guy", "player1"}, {});
    const std::array<float, 3> held = *core.character("guy")->position;
    float previous = held[1];
    for (int u = 0; u < 8; ++u) {
        core.update(0.033f);
        const std::array<float, 3>& at = *core.character("guy")->position;
        require(near(at[0], held[0], 1e-6f) && near(at[2], held[2], 1e-6f), "a released flight keeps x and z");
        require(u == 0 || at[1] < previous, "a released flight falls once it is past its apex");
        previous = at[1];
    }
    require(previous < 17.617f + 2.0f, "the fall stays below the apex");
}

// STAND-IN SCRIPT-S103: a jump whose step is longer than the 0.23 arrival window (40 units in 28 frames, 1.43 per frame) is cut at the target and ends.
void longSuperJumpEnds() {
    for (const float dt : {0.033f, 0.0367f}) {
        AiCore core;
        core.setPositionLookup([](const std::string& name, float out[3]) {
            if (name == "npc") { out[0] = 0; out[1] = 0; out[2] = 0; return true; }
            if (name == "far") { out[0] = 0; out[1] = 0; out[2] = 40; return true; }
            return false;
        });
        core.run(AiNativeKind::AISuperJump, {"npc", "far"}, {2.0f});
        int updates = 0;
        while (core.character("npc")->superJump && updates < 300) {
            core.update(dt);
            ++updates;
        }
        const std::vector<AiMoveEnd> ended = core.takeMovesEnded();
        require(!core.character("npc")->superJump && core.character("npc")->state == AiState::Idle && updates <= 30, "a 40-unit jump ends");
        require(ended.size() == 1 && ended[0].character == "npc", "and raises the AIMove event");
        require(near((*core.character("npc")->position)[2], 40.0f, 1e-3f), "on the target");
    }
}

void faceTargetState() {
    AiCore core = withPositions();
    core.run(AiNativeKind::AIFaceTarget, {"npc", "target"}, {});
    core.update(1.0f / 30.0f);
    require(core.character("npc")->heading && *core.character("npc")->heading == 6711, "state 10 (0x2c5bc4) sets the heading toward the target every tick: atan2(3, 4) * 10430.378");
    const float moved[3] = {9.0f, 0.0f, 9.0f};
    core.setPosition("target", moved);
    core.update(1.0f / 30.0f);
    require(*core.character("npc")->heading == 8192, "the heading follows the target (atan2(9, 9) = pi / 4 = 8192 units)");
    const FaceTargetResult pinned = stepFaceTarget({1, 7, 2}, {0, 0, 0}, nullptr, 77);
    require(pinned.position[0] == 0 && pinned.position[1] == 7 && pinned.position[2] == 0 && pinned.heading == 77 && !pinned.facing,
            "x and z return to the home position (0x2c5bd8..0x2c5be8), y stays; without a target the heading stays");
}

void goToTargetState() {
    GoToTarget order;
    order.target = {3, 5, 4};
    std::uint16_t heading = 0;
    const std::array<float, 3> origin = {0, 0, 0};
    order.minDistance = 5.5f;
    require(stepGoToTarget(order, origin, heading) == GoToTargetResult::Arrived, "MinDistance 5.5 > horizontal distance 5 arrives (0x2c3c20 bgt)");
    order.minDistance = 5.0f;
    require(stepGoToTarget(order, origin, heading) == GoToTargetResult::Moving, "5.0 is not greater than 5.0");
    require(heading == 6711, "heading = trunc(atan2(3, 4) * 10430.378) = trunc(6711.96) (0x2c3cc0)");
    order.minDistance = -1.0f;
    order.arriveBelowTarget = false;
    require(stepGoToTarget(order, origin, heading) == GoToTargetResult::Moving, "the default MinDistance -1.0 never arrives by distance, below the target without bit 6 neither");
    order.arriveBelowTarget = true;
    require(stepGoToTarget(order, origin, heading) == GoToTargetResult::Arrived, "height 0 below the target height 5 with bit 6 of +0x314: arrived (0x2c3c34)");
    require(stepGoToTarget(order, {0, 5, 0}, heading) == GoToTargetResult::Moving, "at the target height (bhs) the y test does not arrive");
    order.target = {-3, 0, -4};
    stepGoToTarget(order, {0, 0, 0}, heading);
    require(heading == 39480, "atan2(-3, -4) * 10430.378 = -26056.03 truncates to -26056, stored as u16 39480");
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
        aiMoveFollowsTheNavGrid();
        aiMoveCrossesToTheNextGrid();
        aiMoveToAnUnreachableTargetChangesNothing();
        aiMoveInTheSameCellEndsAtOnce();
        goToTargetState();
        superJumpPlanAndFlight();
        superJumpTraceReplay();
        releasedFlightFalls();
        longSuperJumpEnds();
        faceTargetState();
    } catch (const std::exception& error) {
        std::cerr << "ai-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-test: ok\n";
    return 0;
}
