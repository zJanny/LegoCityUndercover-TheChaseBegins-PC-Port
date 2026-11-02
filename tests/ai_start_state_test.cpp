// The StartState table of the AI spawner (src/runtime/ai_start_state.*) on made-up spawners. The table words are those 0x288e14 and 0x26a27c pass to
// 0x201a3c, the orders are hand-evaluated from the spawn words they name (llvm-objdump of game.arm.elf, 20261102-mission14b-A, 2026-11-02); none is
// measured in the original.
#include "runtime/ai.hpp"
#include "runtime/ai_start_state.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::AiStartAction;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void table() {
    const std::map<std::uint16_t, runtime::AiStartStateEntry> expected = {
        {0, {0x2cc73c, 0x2a07b4, 0x2bae68, 0x2cb968}}, {1, {0x2a8bec, 0x2a07b4, 0x2bae68, 0x2a8b60}}, {2, {0x2a8bec, 0x2a07b4, 0x2bae68, 0x295538}},
        {3, {0, 0, 0, 0x2bae6c}},                       {4, {0, 0, 0, 0x2b7fe0}},                       {5, {0, 0, 0, 0x2aaf40}},
        {6, {0x2c0af4, 0x2a07b4, 0x2bae68, 0x2bae74}}, {7, {0, 0, 0, 0x2afc90}},                       {8, {0x2b7fd0, 0x2b0dc4, 0x2bae68, 0x2c9ca8}},
        {9, {0x2b7fd0, 0x2b0dc4, 0x2bae68, 0x298bd4}},
    };
    for (const auto& [index, words] : expected) {
        const auto entry = runtime::aiStartStateEntry(index);
        require(entry && entry->create == words.create && entry->postLink == words.postLink && entry->free == words.free && entry->spawn == words.spawn,
                "the words of entry " + std::to_string(index));
    }
    require(!runtime::aiStartStateEntry(10), "no entry above 9");
    for (std::uint16_t index : {0, 1, 2, 6}) require(runtime::aiStartTargetKind(index) == runtime::AiStartTargetKind::RunTo, "Runto targets");
    for (std::uint16_t index : {8, 9}) require(runtime::aiStartTargetKind(index) == runtime::AiStartTargetKind::FollowPath, "FollowPath targets");
    for (std::uint16_t index : {3, 4, 5, 7, 10}) require(runtime::aiStartTargetKind(index) == runtime::AiStartTargetKind::None, "no targets");
}

assets::LevelParam param(const std::string& name, std::uint16_t type, std::uint32_t value) {
    assets::LevelParam p;
    p.hash = assets::nameHash(name);
    p.flags = 2;
    p.type = type;
    p.value = value;
    return p;
}

assets::LevelParam reference(const std::string& name, const std::string& target) {
    assets::LevelParam p = param(name, 4, 0);
    const std::uint32_t hash = assets::nameHash(target);
    p.blocks.push_back({0, 0, 0, 0, std::uint8_t(hash), std::uint8_t(hash >> 8), std::uint8_t(hash >> 16), std::uint8_t(hash >> 24)});
    return p;
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

void recordOfTheCreateAndPostLink() {
    assets::Level cls;
    for (const char* name : {"ForceMove", "SuperjumpHeight", "Runto1", "Runto2", "Runto3", "FollowPath1"}) cls.defaults.push_back(param(name, 0, 0));
    cls.defaults[1].value = bitsOf(3.0f);
    assets::LevelObject object;
    object.params = {param("ForceMove", 5, 3), param("SuperjumpHeight", 0, bitsOf(5.0f)), reference("Runto1", "Goon_GoTo"),
                     reference("Runto3", "Nowhere")};
    const std::map<std::uint32_t, std::string> names = {{assets::nameHash("Goon_GoTo"), "goon_goto"}};
    const runtime::AiStartState jump = runtime::aiStartStateOf(6, object, cls, names);
    require(jump.superJumpHeight == 5.0f && !jump.forceMove, "entry 6 reads SuperjumpHeight (create 0x2c0af4), not ForceMove");
    require(jump.targets[0] == "goon_goto" && jump.targets[1].empty() && jump.targets[2].empty(),
            "post-link 0x2a07b4: Runto1 .. 10 by slot; a reference naming no object stays empty");
    const runtime::AiStartState move = runtime::aiStartStateOf(0, object, cls, names);
    require(move.forceMove && move.superJumpHeight == runtime::kAiStartSuperJumpHeight, "entry 0 reads ForceMove bit 0 (create 0x2cc73c)");
    object.params.erase(object.params.begin() + 1);
    require(runtime::aiStartStateOf(6, object, cls, names).superJumpHeight == 3.0f, "the class value of SuperjumpHeight");
    const runtime::AiStartState none = runtime::aiStartStateOf(7, object, cls, names);
    require(none.targets[0].empty(), "entry 7 has no post-link");
}

void orders() {
    runtime::AiStartState state;
    state.targets[0] = "goto_a";
    state.superJumpHeight = 5.0f;
    state.forceMove = true;
    const auto order = [&](std::uint16_t index, std::size_t slot) {
        state.index = index;
        return runtime::aiStartOrder(state, slot);
    };
    require(order(0, 0).action == AiStartAction::Move && order(0, 0).target == "goto_a" && order(0, 0).avoid,
            "0x2cb968: 0x22d85c to the slot's Runto target, avoid argument = ForceMove");
    require(order(0, 1).action == AiStartAction::Idle, "0x2cb968 without a target: 0x2246c0");
    require(order(1, 0).action == AiStartAction::RequestState6 && order(1, 1).action == AiStartAction::RequestState7, "0x2a8b60: 0x268e88");
    require(order(2, 0).action == AiStartAction::RequestState8 && order(2, 1).action == AiStartAction::None, "0x295538: state 8, no null test");
    require(order(3, 0).action == AiStartAction::MillAbout && order(4, 0).action == AiStartAction::PlaceBelow &&
                order(5, 0).action == AiStartAction::PlaceBehind && order(7, 0).action == AiStartAction::NoState &&
                order(8, 0).action == AiStartAction::FollowPath && order(9, 0).action == AiStartAction::PathLocomotion,
            "the entries without Runto");
    const runtime::AiStartOrder jump = order(6, 0);
    require(jump.action == AiStartAction::SuperJump && jump.target == "goto_a" && jump.height == 5.0f && jump.frames == 28,
            "0x2bae74: 0x213300(character, target, 28, SuperjumpHeight)");
    require(order(6, 1).action == AiStartAction::None && order(10, 0).action == AiStartAction::None, "no target for entry 6, no entry 10");
}

void applied() {
    runtime::AiCore ai;
    ai.setPositionLookup([](const std::string& name, float out[3]) {
        if (name != "goto_a") return false;
        out[0] = 10.0f, out[1] = 2.0f, out[2] = 0.0f;
        return true;
    });
    runtime::AiStartOrder jump;
    jump.action = AiStartAction::SuperJump;
    jump.target = "goto_a";
    jump.height = 5.0f;
    jump.frames = 28;
    require(runtime::applyAiStartOrder(jump, "goon", {0.0f, 0.0f, 0.0f}, ai) == runtime::AiStartApplied::Applied, "the super jump runs through AiCore");
    const runtime::AiCharacter* goon = ai.character("goon");
    require(goon && goon->state == runtime::AiState::GoToTarget && goon->superJump, "state 11 after 0x213300");
    const runtime::SuperJumpPlan plan = runtime::planSuperJump({0.0f, 0.0f, 0.0f}, {10.0f, 2.0f, 0.0f}, 28, 5.0f);
    require(goon->superJump->plan.frames == plan.frames && goon->superJump->plan.climb == 7.0f && goon->superJump->start[0] == 0.0f,
            "the plan of 0x213300 from the release point: climb 2 + 5");
    runtime::AiStartOrder down = jump;
    down.target = "low";
    runtime::AiCore fall;
    fall.setPositionLookup([](const std::string& name, float out[3]) {
        if (name != "low") return false;
        out[0] = 4.0f, out[1] = 0.0f, out[2] = 0.0f;
        return true;
    });
    require(runtime::applyAiStartOrder(down, "jumper", {0.0f, 8.0f, 0.0f}, fall) == runtime::AiStartApplied::Applied, "a jump down");
    for (int i = 0; i < 60 && fall.character("jumper")->state != runtime::AiState::Idle; ++i) fall.update(1.0f / 30.0f);
    const runtime::AiCharacter* jumper = fall.character("jumper");
    require(jumper->state == runtime::AiState::Idle && jumper->superJump && jumper->superJump->released && (*jumper->position)[1] > 0.0f,
            "STAND-IN SCRIPT-S318: state 11 arrives above the target, the flight goes on as a released flight until the host lands it");
    runtime::AiStartOrder move;
    move.action = AiStartAction::Move;
    move.target = "goto_a";
    require(runtime::applyAiStartOrder(move, "runner", {0.0f, 0.0f, 0.0f}, ai) == runtime::AiStartApplied::Applied, "the move runs through AiCore");
    const runtime::AiCharacter* runner = ai.character("runner");
    require(runner && runner->move && runner->move->run && runner->state == runtime::AiState::MoveTo, "0x22d85c with walk 0: a running move");
    runtime::AiStartOrder idle;
    idle.action = AiStartAction::Idle;
    require(runtime::applyAiStartOrder(idle, "runner", {0.0f, 0.0f, 0.0f}, ai) == runtime::AiStartApplied::Applied && !ai.character("runner")->move &&
                ai.character("runner")->state == runtime::AiState::Idle,
            "0x2246c0 ends the move");
    runtime::AiStartOrder none;
    none.action = AiStartAction::NoState;
    require(runtime::applyAiStartOrder(none, "runner", {0.0f, 0.0f, 0.0f}, ai) == runtime::AiStartApplied::Applied && ai.character("runner")->state == runtime::AiState::NoState,
            "0x241304: state 1");
    runtime::AiStartOrder later;
    later.action = AiStartAction::RequestState8;
    require(runtime::applyAiStartOrder(later, "other", {0.0f, 0.0f, 0.0f}, ai) == runtime::AiStartApplied::NotPorted, "state 8 is not ported");
    jump.target = "missing";
    require(runtime::applyAiStartOrder(jump, "other", {0.0f, 0.0f, 0.0f}, ai) == runtime::AiStartApplied::TargetNotFound && !ai.character("other")->superJump,
            "a target AiCore cannot locate changes nothing");
}

}

int main() {
    try {
        table();
        recordOfTheCreateAndPostLink();
        orders();
        applied();
    } catch (const std::exception& error) {
        std::cerr << "ai-start-state-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-start-state-test: ok\n";
    return 0;
}
