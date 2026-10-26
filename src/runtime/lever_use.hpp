#pragma once
// The player's use of a lever (kind 58), docs/re/switches.md: message 3 of the lever calls 0x2366bc(player, lever, 76, 0), which requests state 76
// "PULL LEVER USE" (descriptor 0x357768: update 0x28dc48, enter 0x26b800, per-update 0x29b400, exit 0x284090, event hook 0x2ccc74) behind the helper
// state 20 "USE LERP" (object 0x58e5e4, vtable 0x363490, update 0x2feadc -> 0x27fecc) that turns and moves the player to the use point 0x207be4 found.
// No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace chase::runtime {

constexpr unsigned kPlayerStateUseLerp = 20;
constexpr unsigned kPlayerStatePullLeverUse = 76;
constexpr unsigned kPlayerStatePullLeverHold = 77;
constexpr unsigned kPlayerStateAfterLeverUse = 1;
constexpr float kUseLerpSeconds = 0.2f;     // 0x2366bc stores it to ctrl+0x290, the divisor of the lerp step of 0x27fecc
constexpr int kUseLerpTurnStep = 3200;      // 0x27fecc: 0x23df04(3200, ctrl+8, ctrl+0x296)
constexpr const char* kLeverUseNode = "char_minifig_use";  // 0x207be4 looks the node up in the lever's model (string 0x207f70)
constexpr float kLeverCancelAnimSpeed = 2.0f;  // 0x28ddcc: a multi-turn use given up plays the lever's use animation back at this speed
constexpr int kLeverClipTypeDefault = 188; // 0x200f90: CharacterSwitchAnim fallback for LeverType 0 (switch_lever_use)
constexpr int kLeverClipKey = 185;          // LeverType 6: key_interact
constexpr int kLeverClipPull = 191;         // LeverType 7: pulllever

// 0x200f90(lever, LeverType): the clip id (tablesanimations sheet 1) the player plays in state 76. LeverType 0 reads CharacterSwitchAnim (fallback 188),
// 1 to 5 play nothing (0), 6 plays 185 and 7 plays 191.
int leverSwitchClip(int leverType, int characterSwitchAnim);

// The player clip of state 76 in seconds: its length (length / rate; 0x22f560 gives kind 6 from there) and the time at which the event track
// 0x98d31b7e rises above 0.5 (the hook 0x2ccc74 sees event type 0 with a value above 0.5); negative without that track.
struct LeverClipTiming {
    float seconds = 0.0f;
    float eventSeconds = -1.0f;
};
LeverClipTiming leverClipTiming(const assets::Animation& clip);

// The use point of 0x207be4 when the lever's model has the node kLeverUseNode: the node's world matrix (node matrix times `objectMatrix`, row vectors)
// gives the position (translation row) and the heading int(atan2(z row x, z row z) x 10430.378) the player turns to (ctrl+0x296). Nothing without the node.
struct UseLerpTarget {
    std::array<float, 3> position{};
    std::int16_t heading = 0;
};
std::optional<UseLerpTarget> leverUseTarget(const assets::Model& model, const std::array<float, 16>& objectMatrix);

// 0x23df04: `facing` moves towards `target` by at most `step` (the shorter way round).
std::int16_t turnTowards(std::int16_t facing, std::int16_t target, int step);

struct LeverUseSetup {
    std::optional<UseLerpTarget> target;  // nothing: the player stays where it is
    LeverClipTiming clip;
    int clipId = 0;
    float numberOfTurns = 1.0f;           // NumberOfTurns (0x241280, fallback 1.0)
    bool holdMe = false;                  // HoldMe: the use ends in state 77 instead of 1
};

// What state 76 does to the lever's use animation (record +0x18) besides the switch's own state change: the enter 0x26b800 and the next turn of
// 0x28dc48 play it forwards with the use sound (more than one turn left), a multi-turn use given up (A let go) plays it backwards from its frame at
// double speed (0x28ddc8..0x28de00).
enum class LeverAnim { None, Forward, Backward };

struct LeverUseStep {
    std::optional<unsigned> entered;  // the player state that starts in this update (20, 76, then 1 or 77)
    bool useBit = false;              // the lever's record +0x24 bit 0 is set in this update (hook 0x2ccc74, or 0x28dc48 for more than one turn)
    LeverAnim leverAnim = LeverAnim::None;
    bool ended = false;               // state 76 was left in this update (exit 0x284090 stops the use sound); the player is free again
};

// The two player states as one sequence. start() runs in the update of the A press and takes the first step of USE LERP, which starts in that
// update; update() runs once per later update after the switches (the hook sets the use bit after the lever's own update). State 76 starts in the
// update after the lerp is done and state 1 in the update after the clip ended; the clip time of the k-th update of state 76 (k = 0 at its start) is
// (k + 1) dt. These orders give RPC 20261025-garage-A-1 at dt 0.033 (state 76 at update 8, the use bit after update 31, state 1 at update 81) and the
// carry states' put-down end at update 27 (docs/re/carrying-animation.md).
class PlayerLeverUse {
public:
    void start(std::size_t lever, const float position[3], std::int16_t facing, const LeverUseSetup& setup, float dt);
    // `actionHeld` is ctrl+0xc bit 2 (pad slot 6, A held), `leverUsed` the lever's state 1 (the hook does nothing then).
    LeverUseStep update(float dt, bool actionHeld, bool leverUsed);
    bool active() const { return state_ != 0 || pending_ != 0; }
    unsigned state() const { return state_; }
    std::size_t lever() const { return lever_; }
    // The clip the player shows: the lever clip in state 76 (0 in USE LERP, whose own clip 0x2e85a8 is not ported).
    int clip() const { return state_ == kPlayerStatePullLeverUse ? setup_.clipId : 0; }
    const std::array<float, 3>& position() const { return position_; }
    std::int16_t facing() const { return facing_; }
    float clipSeconds() const { return clipSeconds_; }

private:
    void stepLerp(float dt);
    LeverUseSetup setup_;
    std::size_t lever_ = 0;
    unsigned state_ = 0;
    unsigned pending_ = 0;
    float lerp_ = 0.0f;
    float turnsLeft_ = 0.0f;
    float clipSeconds_ = 0.0f;
    bool eventFired_ = false;
    std::array<float, 3> start_{};
    std::array<float, 3> position_{};
    std::int16_t facing_ = 0;
};

}
