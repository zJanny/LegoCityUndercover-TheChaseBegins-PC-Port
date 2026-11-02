#pragma once
// BulletTime (docs/re/scripts-missions-1-4.md): the native 0x1a2df0 passes (scale, duration, flag) with the player object 0x3772b8 to the BulletTimeSystem
// 0x239330, whose update 0x2ec070 blends the scale of the current scene (scene +0x1c, also the global 0x376d08 through 0x2a0744) from the value it had
// when the call came to the new scale. The readers of the scene scale and the receivers of the messages 97, 98 and 99 are not read: the blend is
// kept here and nothing applies it (STAND-IN SCRIPT-S323). No SDL dependency.
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint16_t kBulletTimeMessageStart = 97;
constexpr std::uint16_t kBulletTimeMessageRestoring = 98;
constexpr std::uint16_t kBulletTimeMessageEnd = 99;
constexpr std::uint8_t kBulletTimeSlowMode = 255;

// What 0x239330 sends to the player object with the message ids 0x61 (0x23ae84 without a value, 0x23ae50 with the duration) and 0x62, 0x63.
struct BulletTimeMessage {
    std::uint16_t id = 0;
    std::optional<float> value;
    bool operator==(const BulletTimeMessage& other) const { return id == other.id && value == other.value; }
};

class BulletTimeSystem {
public:
    // 0x239330(system, owner, flag, scale, duration) with the flag clear, as both calls of mission 1-4 pass it (the flag branch calls 0x20395c and
    // 0x2a36b4, not read). Returns false when it did nothing: no blend is running and the scale is 1.0, or the blend already holds 1.0 and the
    // scale is 1.0.
    bool set(float scale, float duration);
    // 0x2ec070 once per update; `dt` stands in for the clock 0x2528b4 that the original reads (STAND-IN SCRIPT-S323).
    void update(float dt);
    // 0x2ec044: the scene scale back to 1.0 and the system off.
    void stop();

    bool active() const { return active_; }
    float targetScale() const { return scale_; }
    float baseScale() const { return base_; }
    float duration() const { return duration_; }
    std::uint8_t mode() const { return mode_; }
    float worldScale() const { return worldScale_; }
    const std::vector<BulletTimeMessage>& messages() const { return messages_; }
    std::vector<BulletTimeMessage> takeMessages();

private:
    bool active_ = false;       // +0x69
    float scale_ = 1.0f;        // +0x5c
    float base_ = 1.0f;         // +0x58: the scene scale when the call came
    float start_ = 0.0f;        // +0x60: the clock when the call came
    float duration_ = 0.0f;     // +0x64
    std::uint8_t mode_ = 0;     // +0x68
    float clock_ = 0.0f;
    float worldScale_ = 1.0f;   // scene +0x1c, 1.0 at level load
    std::vector<BulletTimeMessage> messages_;
};

}
