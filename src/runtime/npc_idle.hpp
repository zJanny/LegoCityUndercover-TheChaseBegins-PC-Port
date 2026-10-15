#pragma once
// The idle state of a level character (state function 0x1d5070, docs/re/npc-animation.md): whenever the character has no
// animation or the current one has finished, it starts the next idle animation; the choice between "idle" and "idle2" is
// one bit of the game's random generator (0x219614). Each animation plays once at its stored rate (slot mode 0 of 0x24cbd4)
// and holds its last position until the next update starts the next one. No SDL dependency.
#include "runtime/game_random.hpp"

#include <cstdint>

namespace chase::runtime {

enum class NpcIdleVariant : std::uint8_t { Idle, Idle2 };

constexpr std::uint32_t kIdleRandomBit = 4;

struct NpcIdleClip {
    double frames = 0;
    double framesPerSecond = 0;
};

struct NpcIdlePose {
    NpcIdleVariant variant = NpcIdleVariant::Idle;
    double position = 0;  // animation position in frames, 0 to the clip's frame count (the original's slot value)
};

class NpcIdle {
public:
    // One update of the idle state with the time since the previous update. The first call starts the first animation.
    void update(double dt, GameRandom& random, const NpcIdleClip& idle, const NpcIdleClip& idle2);
    bool started() const { return started_; }
    NpcIdlePose pose(const NpcIdleClip& idle, const NpcIdleClip& idle2) const;
    NpcIdleVariant variant() const { return variant_; }
    double elapsedSeconds() const { return elapsed_; }

private:
    static const NpcIdleClip& clipOf(NpcIdleVariant variant, const NpcIdleClip& idle, const NpcIdleClip& idle2) {
        return variant == NpcIdleVariant::Idle ? idle : idle2;
    }
    bool started_ = false;
    NpcIdleVariant variant_ = NpcIdleVariant::Idle;
    double elapsed_ = 0;
};

}
