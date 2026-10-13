#pragma once
// Stick-to-intent step of the player controller (FUN_0026be04, 0x200354..0x200cbf; docs/re/player.md).
// No SDL dependency.
#include <cstdint>

namespace chase::runtime {

// 0x24765c (tail call into the library atan2f at 0x2473ec): s0 = first argument, s1 = second.
// Replica of the library routine, not libm.
float gameAtan2(float first, float second);

// 0x247ac8 (sinf) and 0x247c90 (cosf) of the game's library, called by the sincos routine 0x249038 (r0 = sin, r1 = cos).
// Arguments of magnitude 12868 or more take the library's slow path (0x132ec8), which is replaced by libm here.
float gameSin(float angle);
float gameCos(float angle);

struct StickChannel {
    float value = 0;
    float deadzone = 0.1f;
};

struct StickIntentInput {
    StickChannel slot14;
    StickChannel slot15;
    std::uint16_t entityType = 0;
    float cameraYaw = 0;
    std::int8_t byte372d44 = 0;
};

struct StickIntent {
    std::int16_t heading = 0;
    std::uint16_t heldSet = 0;
    std::uint16_t heldClear = 0;
    std::uint16_t pressClear = 0;
    bool active = false;
};

constexpr std::uint16_t kHeldMove = 0x1;
constexpr std::uint16_t kHeldSlow = 0x8;

// Result for an entity that passed the player check (0x2003c8) and the 0x235f08 != 3 check (0x2003d8).
// heading is the controller's s16 at +6 after the step (zero when the stick is inactive); heldSet is
// ORed into and heldClear masked out of the u16 at +0xc; pressClear is masked out of the u16 at +0xe.
StickIntent stickIntent(const StickIntentInput& input);

}
