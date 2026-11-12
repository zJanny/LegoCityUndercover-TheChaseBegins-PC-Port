#pragma once
// The animation player of an entity (docs/re/npc-animation.md): four slots, newest first. Start 0x245b40 (wrapped by
// 0x23afc0), position 0x24cbd4, kind 0x22f560, stop 0x225c5c with the pause 0x24f8fc, start offset 0x22c2fc and the test of
// WaitForCurrentAnim / WaitForCurrentCharAnim (0x2a06a8, 0x263ae4). Times are ticks of the owner's clock. Blending between
// the slots (0x115ecc) is not here (ANIM-3). No SDL dependency.
#include <array>
#include <cstddef>
#include <cstdint>

namespace chase::runtime {

constexpr std::int64_t kAnimationClockTicksPerSecond = 3000;
constexpr std::uint16_t kAnimationFrameDefault = 0xffff;

enum AnimationMode : unsigned {
    kAnimationOnce = 0,
    kAnimationLoop = 1,
    kAnimationReverse = 2,
    kAnimationPingPong = 3,
    kAnimationReverseLoop = 5,
    kAnimationStopped = 6,
};

// What the per-type callbacks of the animation data give the player (type 0 of the table at 0x52ce8c): the length
// (0x1081ec, assets::animationLength), the rate in frames per second (0x108190) and the loop bit (0x107990).
struct AnimationClipInfo {
    std::uint32_t clip = 0;
    std::uint16_t length = 0;
    std::uint16_t rate = 15;
    bool loops = false;
};

// Slot record of 0x58 bytes: +0 animation, +4 mode (low 3 bits) and bit 13 (wrap), +8 start id, +0xc rate, +0x2c first
// frame, +0x2e end frame, +0x30 rate x speed, +0x34 start clock, +0x38 stop clock (-1 while running), +0x3c start clock
// as started, +0x40 blend time.
struct AnimationSlot {
    std::uint32_t clip = 0;
    std::uint16_t length = 0;
    unsigned mode = kAnimationOnce;
    bool wrap = false;
    std::uint32_t startId = 0;
    std::uint16_t rate = 0;
    std::uint16_t first = 0;
    std::uint16_t end = 0;
    float rateSpeed = 0;
    std::int64_t start = 0;
    std::int64_t stop = -1;
    std::int64_t started = 0;
    float blend = 0;
};

struct AnimationPosition {
    float frame = 0;
    bool finished = false;
    std::uint32_t loops = 0;
};

class AnimationSlots {
public:
    static constexpr std::size_t kCapacity = 4;

    // `first` / `end` kAnimationFrameDefault take the clip length.
    const AnimationSlot& start(const AnimationClipInfo& clip, unsigned mode, std::uint16_t first, std::uint16_t end, float speed,
                               float blend, std::int64_t now);
    std::size_t count() const { return count_; }
    const AnimationSlot* current() const { return count_ ? &slots_[0] : nullptr; }
    const AnimationSlot& slot(std::size_t index) const { return slots_[index]; }

    static AnimationPosition position(const AnimationSlot& slot, std::int64_t now, std::int32_t offset = 0);
    unsigned kind(std::int64_t now) const;
    void stop(std::int64_t now);
    // 0x24f8fc on the newest slot: nothing in mode 6; a pause stores the clock in +0x38 when it holds -1, a resume moves the start clock +0x34 by
    // the paused time and sets +0x38 back to -1.
    void pause(bool paused, std::int64_t now);
    void setMode(unsigned mode);
    void setOffset(float frame, std::int64_t now);
    bool currentNearEnd(std::int64_t now) const;

private:
    std::array<AnimationSlot, kCapacity> slots_{};
    std::size_t count_ = 0;
    std::uint32_t starts_ = 0;
};

}
