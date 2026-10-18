#pragma once
// The idle state of a level character (state function 0x1d5070, docs/re/npc-animation.md): whenever the character's
// animation player has no animation or the current one has finished (kind 6 of 0x22f560), it starts the next idle
// animation; the choice between "idle" and "idle2" is one bit of the game's random generator (0x219614). Each starts in
// mode 0 (play once) from frame 0 to the clip length, speed 1.0, blend 0.1 (0x21941c). PlayCharAnim (0x29e5f0) requests the
// "USER ANIM" state 0x50 (0x51 for PlayCutscene) whose update 0x2a47b0 does not start idles; when the slot kind is 6 it only requests
// state 1, so the finished script animation stays visible to WaitForCurrentCharAnim for one update before an idle replaces it.
// No SDL dependency.
#include "runtime/animation_slots.hpp"
#include "runtime/game_random.hpp"

#include <cstdint>

namespace chase::runtime {

enum class NpcIdleVariant : std::uint8_t { Idle, Idle2 };

constexpr std::uint32_t kIdleRandomBit = 4;
constexpr float kIdleSpeed = 1.0f;
constexpr float kIdleBlend = 0.1f;

class NpcIdle {
public:
    // One update of the idle state at clock `now`. Returns true when it started an animation.
    bool update(AnimationSlots& slots, std::int64_t now, GameRandom& random, const AnimationClipInfo& idle, const AnimationClipInfo& idle2);
    void enterScriptAnimation() { scripted_ = true; }
    bool scripted() const { return scripted_; }
    bool started() const { return started_; }
    NpcIdleVariant variant() const { return variant_; }

private:
    bool scripted_ = false;
    bool started_ = false;
    NpcIdleVariant variant_ = NpcIdleVariant::Idle;
};

}
