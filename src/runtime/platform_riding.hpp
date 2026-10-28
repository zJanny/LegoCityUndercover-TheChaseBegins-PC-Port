#pragma once
// Riding moving platforms (docs/re/props-rocking-platforms.md): the natives of the lift scripts, SetPlatformSpeed 0x26fe08 (0x1b5d38 with no time),
// SetPlatformOrigin 0x27bfbc (0x1c416c), SetPlatformTarget 0x27bfd8 (0x1c41a4) and MovePlatform 0x1bf958 (message 26), the reset message 8 of the platform
// handler 0x1a8d74, and the ground entity of a character: the ground contact 0x2b24bc -> 0x20b78c stores the platform and its translation (ctrl+0x174,
// ctrl+0x178), and the mover 0x20cd30 adds the move of that translation (0x2b01e4) to the character's movement. No SDL dependency.
#include "runtime/platform_node_move.hpp"

#include <cstdint>
#include <string>

namespace chase::runtime {

constexpr std::uint8_t kPlatformMessageReset = 8;   // ResetObject 0x1b25f8 sends it; handler 0x1a8d74 case 8
constexpr std::uint8_t kPlatformMessageMove = 26;   // MovePlatform 0x1bf958
constexpr float kPlatformSpeedMinimum = 0x1p-23f;   // 0x34000000 at 0x1b5da8: a speed whose bits are below it (negative ones too) becomes this
constexpr float kGroundEntityRideLimitSquared = 25.0f;  // literal 0x41c80000 at 0x2b0400: a squared move of 25.0 or more (or 0) is not ridden

// SetPlatformSpeed: the native passes no time (s1 = 0.0 at 0x26fe10), so 0x1b5d38 stores the speed at R+0x74 at once.
void setPlatformSpeed(PlatformRecord& record, float speed);
// SetPlatformOrigin / SetPlatformTarget: R+0x18 / R+0x24 = the translation of the other object's node world matrix (0x2429d0, 0x221a1c). STAND-IN SCRIPT-S309:
// the quaternions R+0x30 / R+0x40 that `orient` platforms turn by are not set.
void setPlatformOrigin(PlatformRecord& record, const PlatformVector& translation);
void setPlatformTarget(PlatformRecord& record, const PlatformVector& translation);
// MovePlatform(platform, progress, backwards): message 26 calls 0x2150bc(entity, backwards = argument 2 != 0, progress = argument 1).
void movePlatform(PlatformRecord& record, float progress, bool backwards);
// Message 8 (case 8 of 0x1a8d74): the requested state becomes 2 for type 0 and 0 for types 1 to 6, the value 0.0, and the node goes back to the origin R+0x18,
// which is returned. STAND-IN SCRIPT-S309: the node's rotation (R+0x30 for `orient`, `initialorientation` for a path) and the reference step 0x23211c are not ported.
PlatformVector platformReset(PlatformRecord& record);

// ctrl+0x174 (the ground entity) and ctrl+0x178 (its translation at the last contact, or for an `orient` platform the character's position in the
// platform's frame), ctrl+0x184 (for an `orient` platform the character's forward axis in that frame). `owner` is empty when the character stands on no platform.
struct GroundEntityLink {
    std::string owner;
    PlatformVector remembered{};
    bool oriented = false;
    PlatformVector localForward{};
};
// The platform node's world matrix: rotation rows (right, up, forward) and translation.
struct PlatformFrame {
    PathRotation rows = kIdentityPathRotation;
    PlatformVector translation{};
};
constexpr float kRadiansToHeading = 10430.378f;  // literal at 0x2b03fc: 65536 / 2 pi
// 0x20b78c for a platform (kind 64) without `orient`: the ground entity becomes the platform and ctrl+0x178 its node translation.
void groundEntityContact(GroundEntityLink& link, const std::string& lowerPlatform, const PlatformVector& translation);
// 0x20b78c for an `orient` platform (0x1de884): ctrl+0x178 = the position in the platform's frame (0x2536c0), ctrl+0x184 = the forward axis in it (0x20c7f0).
void groundEntityContactOriented(GroundEntityLink& link, const std::string& lowerPlatform, const PlatformFrame& frame, const PlatformVector& position,
                                 const PlatformVector& forward);
// 0x2b01e4 for an `orient` platform: the remembered local position taken back to the world by the frame (0x250da8) minus the position, with y 0, and
// the turn int16(int32((atan2 of the remembered local forward - atan2 of the current one) x 65536 / 2 pi)) that is added to ctrl+8 and ctrl+0xa.
struct GroundEntityTurn {
    PlatformVector move{};
    std::int16_t turn = 0;
};
GroundEntityTurn groundEntityRideOriented(const GroundEntityLink& link, const PlatformFrame& frame, const PlatformVector& position, const PlatformVector& forward);
// 0x2b01e4 for a platform without `orient`: the translation now minus the remembered one, or zero unless
// 0 < squared distance < 25 (0x24ef70: unfused products of remembered - now).
PlatformVector groundEntityRide(const GroundEntityLink& link, const PlatformVector& now);

}
