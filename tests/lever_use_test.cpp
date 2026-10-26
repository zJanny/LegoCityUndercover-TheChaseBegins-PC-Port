// The player's lever use (src/runtime/lever_use.cpp) on made-up values. The rules come from the code read by 20261026-mission13-A on 2026-10-26
// (llvm-objdump of game.arm.elf): 0x200f90 (clip of LeverType), 0x2366bc (request of state 76 behind USE LERP, lerp time 0.2), 0x27fecc (USE LERP:
// turn 3200 per update, lerp, done the update after t reaches 1.0), 0x26b800 / 0x28dc48 / 0x2ccc74 (state 76: turns, the clip end, the hook that
// sets the use bit), 0x207be4 (the node char_minifig_use) and 0x23df04 (turn). docs/re/switches.md.
#include "runtime/lever_use.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

constexpr float kDt = 0.033f;

struct Timeline {
    unsigned use76 = 0;
    unsigned useBit = 0;
    unsigned end = 0;
    unsigned useBits = 0;
    unsigned entered77 = 0;
    std::vector<LeverAnim> anims;
};

Timeline run(const LeverUseSetup& setup, unsigned updates, bool held = false, unsigned releaseAt = 0) {
    PlayerLeverUse use;
    const float position[3] = {0, 0, 0};
    use.start(0, position, 0, setup, kDt);
    Timeline timeline;
    for (unsigned u = 1; u <= updates && use.active(); ++u) {
        const LeverUseStep step = use.update(kDt, held && (releaseAt == 0 || u < releaseAt), false);
        if (step.entered == kPlayerStatePullLeverUse) timeline.use76 = u;
        if (step.entered == kPlayerStatePullLeverHold) timeline.entered77 = u;
        if (step.useBit) {
            if (!timeline.useBit) timeline.useBit = u;
            ++timeline.useBits;
        }
        if (step.leverAnim != LeverAnim::None) timeline.anims.push_back(step.leverAnim);
        if (step.ended) timeline.end = u;
    }
    return timeline;
}

void clips() {
    require(leverSwitchClip(0, 466) == 466, "LeverType 0 plays CharacterSwitchAnim");
    require(leverSwitchClip(0, kLeverClipTypeDefault) == 188, "its fallback is 188 (switch_lever_use)");
    require(leverSwitchClip(6, 466) == 185 && leverSwitchClip(7, 466) == 191, "LeverType 6 plays 185, 7 plays 191");
    for (int type = 1; type <= 5; ++type) require(leverSwitchClip(type, 466) == 0, "LeverType 1 to 5 play nothing");
}

void turning() {
    require(turnTowards(0, 10000, kUseLerpTurnStep) == 3200, "at most 3200 per update");
    require(turnTowards(-31000, 31000, kUseLerpTurnStep) == 31336, "the shorter way round, over the wrap");
    require(turnTowards(100, 300, kUseLerpTurnStep) == 300, "the rest when it is less than the step");
}

void useTarget() {
    chase::assets::Model model;
    model.nodeNames = {"polySurface1", "char_minifig_use"};
    model.nodeMatrices = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1.5f, 1}};
    const std::array<float, 16> object = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 10, 2, 20, 1};
    const std::optional<UseLerpTarget> target = leverUseTarget(model, object);
    require(target.has_value(), "the node char_minifig_use gives the use point");
    require(near(target->position[0], 11.5f) && near(target->position[1], 2) && near(target->position[2], 20), "node translation times the object matrix");
    require(std::abs(target->heading + 16384) <= 1, "heading from the node's world z row: (-1, 0, 0) is -90 degrees, got " + std::to_string(target->heading));
    chase::assets::Model bare;
    bare.nodeNames = {"polySurface1"};
    bare.nodeMatrices = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
    require(!leverUseTarget(bare, object), "no node, no use point");
}

void lerp() {
    LeverUseSetup setup;
    setup.target = UseLerpTarget{{1.65f, 0, 0}, 16384};
    setup.clip = {2.4f, 0.7668f};
    PlayerLeverUse use;
    const float position[3] = {0, 0, 0};
    use.start(0, position, 0, setup, kDt);
    require(use.state() == kPlayerStateUseLerp, "USE LERP starts in the update of the press");
    require(near(use.position()[0], 0.165f * 1.65f) && use.facing() == 3200, "first step: t = dt / 0.2 and a turn of 3200");
    for (unsigned u = 1; u <= 5; ++u) use.update(kDt, false, false);
    require(use.facing() == 16384, "the turn reaches the heading");
    require(!near(use.position()[0], 1.65f), "t is 0.99 after six steps of 0.033");
    use.update(kDt, false, false);
    require(near(use.position()[0], 1.65f), "the seventh step clamps t to 1.0");
    use.update(kDt, false, false);
    require(use.state() == kPlayerStateUseLerp, "the update that sees t == 1.0 ends the lerp");
    const LeverUseStep step = use.update(kDt, false, false);
    require(step.entered == kPlayerStatePullLeverUse && use.clip() == 0, "state 76 starts in the next update; clip id 0 was given");
}

void keySwitch() {
    LeverUseSetup setup;
    setup.clip = {2.4f, 0.319495f * 2.4f};
    setup.clipId = 185;
    const Timeline t = run(setup, 200);
    require(t.use76 == 8, "state 76 at update 8 (RPC 20261025-garage-A-1), got " + std::to_string(t.use76));
    require(t.useBit == 31 && t.useBits == 1, "the hook sets the use bit in update 31 (RPC 20261025-garage-A-1), got " + std::to_string(t.useBit));
    require(t.end == 81, "state 1 at update 81 (RPC 20261025-garage-A-1), got " + std::to_string(t.end));
    require(t.anims.empty(), "one turn: the lever's animation is the switch's own (state change to 1)");
}

void pullSwitch() {
    LeverUseSetup setup;
    setup.clip = {24.0f / 15.0f, 0.00284608f * 23.0f / 15.0f};
    const Timeline t = run(setup, 200);
    require(t.use76 == 8 && t.useBit == 8, "the pull clip's event is at its start: the use bit in the first update of state 76");
    require(t.end == 57, "24 frames at 15 per second: state 1 at update 57, got " + std::to_string(t.end));
}

void usedLeverGetsNoBit() {
    LeverUseSetup setup;
    setup.clip = {2.4f, 0.7668f};
    PlayerLeverUse use;
    const float position[3] = {0, 0, 0};
    use.start(0, position, 0, setup, kDt);
    bool bit = false;
    for (unsigned u = 1; u <= 90; ++u) bit = use.update(kDt, false, true).useBit || bit;
    require(!bit, "0x2ccc74 does nothing while the lever is in state 1");
}

void multiTurn() {
    LeverUseSetup setup;
    setup.clip = {1.0f, 0.5f};
    setup.numberOfTurns = 2.0f;
    const Timeline held = run(setup, 300, true);
    require(held.anims.size() == 2 && held.anims[0] == LeverAnim::Forward && held.anims[1] == LeverAnim::Forward,
            "two turns held: the enter and the next turn play the lever's animation");
    require(held.useBits == 2, "the hook sets the bit in the last turn and 0x28dc48 again at the end (NumberOfTurns above 1)");
    const Timeline released = run(setup, 300, true, 20);
    require(released.anims.size() == 2 && released.anims[1] == LeverAnim::Backward && released.useBits == 0,
            "A let go before the first turn ends: the lever's animation plays back and no bit is set");
    require(released.end > 0, "and the player goes to state 1");
}

void holdMe() {
    LeverUseSetup setup;
    setup.clip = {1.0f, 0.5f};
    setup.holdMe = true;
    const Timeline t = run(setup, 200);
    require(t.entered77 > 0 && t.entered77 == t.end, "HoldMe: the use ends in state 77");
}

}

int main() {
    try {
        clips();
        turning();
        useTarget();
        lerp();
        keySwitch();
        pullSwitch();
        usedLeverGetsNoBit();
        multiTurn();
        holdMe();
    } catch (const std::exception& error) {
        std::cerr << "lever-use-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "lever-use-test: ok\n";
    return 0;
}
