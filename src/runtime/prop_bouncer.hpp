#pragma once
// The class leprops/leprop_bouncer (object type 19, create 0x18abd0, message handler 0x1988f4) and the player's bounce take-off (state 25 enter 0x2f4e10).
// Landing on the pad's floor makes the player's ground-contact handler send the bouncer message 15 (0x20b78c); the handler requests state 25 and the
// take-off computes the launch speed. Evidence: docs/re/props-wobbly-bouncer.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/player_vertical.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kBouncerClass = "leprops/leprop_bouncer";
constexpr std::uint8_t kMessageBounce = 15;

struct BouncerParams {
    std::string instance;                 // lower case
    float highestTakeOffSpeed = 1.0f;     // HighestTakeOffSpeed, entity +0xa0, made positive
    float lowestTakeOffSpeed = 0.3f;      // LowestTakeOffSpeed, entity +0x9c, made positive
    float jumpSpeedChange = 0.1f;         // JumpSpeedChange, entity +0xa4
    float fallSpeedLoss = 1.0f;           // FallSpeedLoss, entity +0xa8
    bool autoBounce = false;              // AutoBounce, byte +0xb2 bit 0 (read, no reader found)
    bool trickBounce = false;             // TrickBounce, byte +0xb2 bit 1 (copied to ctrl+0x317 bit 0 by the take-off update when the speed is above 0.65)
    std::uint16_t sound = 0;              // ATTR_SFX, played at the take-off (0x237d78)
    std::uint16_t soundHit = 0;           // ATTR_SFXHit, played by the message 0xfc forward
    std::uint16_t particle = 0;           // ATTR_ParticleNum, started at the take-off when non-zero
};

BouncerParams bouncerParams(const assets::LevelObject& object, const assets::Level& cls);

// The bouncers of a level by lower-case instance name.
class BouncerSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    const BouncerParams* find(const std::string& lowerInstance) const;
    std::size_t size() const { return bouncers_.size(); }

private:
    std::vector<BouncerParams> bouncers_;
};

// The player's fields the bounce keeps: ctrl+0x2dc (the speed of the last take-off, 0 before the first) and ctrl+0x24c (set to 1.0 by the message handler
// when FallSpeedLoss is below 1.0, cleared by every take-off; only its equality with 1.0 is tested; 0.635 was read during a jump).
struct BounceMemory {
    float speed = 0;
    float field24c = 0;
};

struct BounceLaunch {
    float speed = 0;       // the take-off speed written back to ctrl+0x2dc
    float fallSpeed = 0;   // ctrl+0x2b8 after the take-off update: -speed plus one rise step
    float rise = 0;        // the node height gained in the take-off update: the update moves it by -ctrl+0x2b8 = speed before the rise step is added
};

// Message 15 with the player falling (0x198a24..0x198a74) followed by the take-off enter (0x2f4e10..0x2f4f00): `fallSpeed` is ctrl+0x2b8 at the
// landing, `jumpHeld` is bit 1 of ctrl+0xc (the B button).
BounceLaunch bounceLaunch(const BouncerParams& bouncer, BounceMemory& memory, float fallSpeed, bool jumpHeld);
// Puts the player into the bounce flight: posture Falling, the speed as the launch left it, no fall distance.
void startBounceFlight(VerticalState& state, const BounceLaunch& launch);

}
