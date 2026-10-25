#pragma once
// WaitForPlatform 0x263b84 (docs/re/props-rocking.md): the script waits until the platform record of the object (ObjectType 64, record at
// entity+0x7c) reports its travel value R+0x7c at 1.0 or above, or its state R+2 at 5. The platform class (create 0x199adc, update 0x2807c8,
// state change 0x19a590, handler 0x1a8d74) is ported only as far as that record needs it (SCRIPT-44). No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace chase::runtime {

constexpr std::uint8_t kPlatformKind = 64;
constexpr std::uint16_t kPlatformMovingState = 2;
constexpr std::uint16_t kPlatformArrivedState = 5;
constexpr float kPlatformSpeedDefault = 0.01f;   // `speed` default of the create (literal 0x3c23d70a at 0x199e6c), stored at R+0x74
constexpr float kPlatformStepRate = 30.0f;       // literal 0x41f00000 at 0x280ae8

struct PlatformRecord {
    float value = 0;              // R+0x7c, read by 0x1de878
    std::uint16_t state = 0;      // R+2
    std::uint16_t requested = 0;  // R+4
    float speed = kPlatformSpeedDefault;  // R+0x74
};

// 0x263b84: the bit pattern of the value equal to 1.0, or not below it as a signed integer (a float at or above 1.0), or the state 5.
bool platformReached(const PlatformRecord& record);

class PlatformSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    PlatformRecord* find(const std::string& lowerInstance);
    const PlatformRecord* find(const std::string& lowerInstance) const;
    // STAND-IN SCRIPT-S296: a trigger message (0xff) requests state 2 (the requested state R+4 that 0x2150bc writes, as the oracle showed for
    // the crane platform); the other branches of the handler 0x1a8d74 and of 0x2150bc (types, ObjectTo, the way back) are not ported.
    void triggerStandIn(const std::string& lowerInstance);
    // The entity update after the script pass: in state 2 the travel step of 0x2807c8 (0x280980) adds dt * 30 * speed to the value; STAND-IN
    // SCRIPT-S296: at 1.0 or above the value is set to 1.0 and state 5 requested (measured, the code after 0x280984 is not read); then the
    // requested state becomes the state (0x19a590). The platform's node does not move.
    void update(float dt);

private:
    std::map<std::string, PlatformRecord> records_;
};

}
