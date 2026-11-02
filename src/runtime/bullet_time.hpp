#pragma once
// BulletTime (docs/re/scripts-missions-1-4.md): the native 0x1a2df0 passes (scale, duration, flag) with the player object 0x3772b8 to the BulletTimeSystem
// 0x239330, whose update 0x2ec070 blends the time scale of the running process (C+0x1c of the scheduler client [P + 0xc], which 0x23878c returns)
// from the value it had when the call came to the new scale, on its own real-time clock. The scheduler 0x100f44 multiplies the tick count of every
// step by that scale, so the whole game update slows down (Scheduler::setScale). The system plays the sounds 97, 98 and 99 at the owner (0x23ae84)
// and stops 97 and 98 (0x23ae50). No SDL dependency.
#include <cstdint>
#include <vector>

namespace chase::runtime {

constexpr std::uint16_t kBulletTimeSoundStart = 97;
constexpr std::uint16_t kBulletTimeSoundLoop = 98;
constexpr std::uint16_t kBulletTimeSoundEnd = 99;
constexpr std::uint8_t kBulletTimeSlowMode = 255;

// A sound call of the system for its owner: a play through 0x23ae84, or a stop through 0x23ae50 -> 0x23f75c with the duration as the fade.
struct BulletTimeSound {
    bool stop = false;
    std::uint16_t id = 0;
    float fadeSeconds = 0;
    bool operator==(const BulletTimeSound& other) const { return stop == other.stop && id == other.id && fadeSeconds == other.fadeSeconds; }
};

class BulletTimeSystem {
public:
    // 0x239330(system, owner, flag, scale, duration) with the flag clear, as both calls of mission 1-4 pass it (the flag branch calls 0x20395c and
    // 0x2a36b4, not read). Returns false when it did nothing: no blend is running and the scale is 1.0, or the blend already holds 1.0 and the
    // scale is 1.0.
    bool set(float scale, float duration);
    // 0x2ec070 once per update. `realDt` is the real time since the last update: the clock +0x28 is a 3000 Hz tick timer (0x25a1a0) that the time
    // scale does not slow. `loopSoundIdle` is the test of 0x237acc: no voice of 97 runs for the owner and 98 is not playing; then, while the target
    // scale is not 1.0, the system plays 98. Returns true in the update in which a blend reaches its target (progress clamped to 1.0).
    bool update(float realDt, bool loopSoundIdle);
    // 0x2ec044: the scale back to 1.0 and the system off.
    void stop();

    bool active() const { return active_; }
    float targetScale() const { return scale_; }
    float baseScale() const { return base_; }
    float duration() const { return duration_; }
    std::uint8_t mode() const { return mode_; }
    // The scheduler's C+0x1c (1.0 at level load).
    float timeScale() const { return timeScale_; }
    const std::vector<BulletTimeSound>& sounds() const { return sounds_; }
    std::vector<BulletTimeSound> takeSounds();

private:
    bool active_ = false;       // +0x69
    float scale_ = 1.0f;        // +0x5c
    float base_ = 1.0f;         // +0x58: the time scale when the call came
    float start_ = 0.0f;        // +0x60: the clock when the call came
    float duration_ = 0.0f;     // +0x64
    std::uint8_t mode_ = 0;     // +0x68
    bool reached_ = false;
    float clock_ = 0.0f;
    float timeScale_ = 1.0f;
    std::vector<BulletTimeSound> sounds_;
};

}
