#pragma once
// The mover of a level character and the throw fall of the grab and takedown states (docs/re/scripts-missions-1-4-roof.md, entry 2026-11-12 "The
// character mover and the throw fall"): 0x215804 -> 0x20cd30 with its ground probe 0x20b48c and the character's answers to the events 0x26 (fall) and
// 0x24 (ground contact), the head and tail that the MELEE PUTDOWN update 0x27726c and the takedown update 0x283b10 share (sub +0x131 bits 4 and 5,
// the flight vector ctrl+0x2f8), the fall-allowed track of the animation event handler (0x1d6a64), the enter 0x28cbf8's clear of the bits and the
// FALL ARREST HIT request of the ground contact 0x2b24bc. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/collision.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace chase::runtime {

using AirVector = std::array<float, 3>;

// The animation event kind and track of the handler branch 0x1d6a64 (the hash is the negated literal 0x37898b26 at 0x1d6c74).
inline constexpr std::uint32_t kFallAllowedAspect = 0x0c39ee40;
inline constexpr std::uint32_t kFallAllowedTrack = 0xc87674da;
// 0x2772c4 / 0x283b74: the flight vector's y drops by this every update (not scaled by dt).
inline constexpr float kThrownFallStep = 0.05f;
// *0x3771ac, compared with |node y - ctrl+0x244| before a fall starts (0x2774a4, 0x283bf0).
inline constexpr float kThrownFallStartDrop = 0.05f;
inline constexpr std::uint16_t kFallArrestHitState = 0x136;  // 310 FALL ARREST HIT, requested by 0x2b2558

struct CharacterAir {
    bool fallAllowed = false;            // sub +0x131 bit 4
    bool thrown = false;                 // sub +0x131 bit 5
    bool onGround = true;                // ctrl+0x314 bit 6
    std::optional<float> groundBelow;    // ctrl+0x244 (-FLT_MAX: none)
    float fallSpeed = 0;                 // ctrl+0x2b8, world units per update, positive downwards
    float fallDistance = 0;              // ctrl+0x2b4
    AirVector velocity{};                // ctrl+0x2f8, the flight vector
    AirVector lastMove{};                // ctrl+0x204: node position after the last move minus before it
};

// The arguments of 0x20cd30 the callers here pass: r3 (the vector, zero for none), flags bit 0 (set: entity +0xc bit 5 cleared, no fall speed in the
// movement and no gravity), and what 0x2c9770 adds (the clip's root displacement of this update in world space).
struct CharacterMove {
    AirVector vector{};
    AirVector root{};
    bool gravity = false;
};

struct CharacterMoveResult {
    bool probed = false;           // 0x20b48c ran
    bool landed = false;           // the probe found ground after the character was in the air (event 0x24 -> 0x2b24bc)
    bool fallEvent = false;        // the probe found no ground: event 0x26
    bool fallStateRequest = false; // in the gravity mode the handler 0x18f0e4 asks for FALL (5) unless the state is excluded (not ported here)
};

// 0x20cd30 for a level character at `position` whose node faces (forwardX, forwardZ): the horizontal part slides along the walls (0x21a73c), then a
// rise moves the node up and leaves the ground (0x20d464), a standing character with no movement is not probed, and every other move probes the
// ground below (0x20b48c): ground sets the node on it, no ground moves the node down by ctrl+0x2b8 (with the gravity step 0x18f1dc in the gravity
// mode). Last, ctrl+0x204 = the node's move (0x20d72c).
CharacterMoveResult moveCharacter(CharacterAir& air, AirVector& position, float forwardX, float forwardZ, const CharacterMove& move,
                                  const CollisionWorld& world, float dt);

enum class ThrownFallStep { Landed, Flying };
struct ThrownFall {
    ThrownFallStep step = ThrownFallStep::Flying;  // Landed: 0x1fdad0(entity, ctrl+0x2bc, 0) resumes the clip
    CharacterMoveResult move;
};

// The head of 0x27726c / 0x283b10 with bit 5 set: on the ground bit 5 is cleared and the clip resumes; in the air the flight vector's y drops by
// kThrownFallStep and 0x215804(entity, ctrl, 1, ctrl+0x2f8) moves the character. Nothing when bit 5 is clear.
std::optional<ThrownFall> thrownFlight(CharacterAir& air, AirVector& position, float forwardX, float forwardZ, const CollisionWorld& world, float dt);

// The tail of 0x27726c / 0x283b10: with bit 4 set, bit 5 clear, the character in the air and no ground height or one farther than
// kThrownFallStartDrop below or above the node, the flight vector becomes the last move, the clip pauses (0x1fdad0(entity, ctrl+0x2bc, 1)) and bit 5 is
// set. True when it started.
bool startThrownFall(CharacterAir& air, float nodeY);

// The end of the takedown enter 0x28cbf8: bit 5 cleared; a gun HIT state entered in the air starts the fall as above (true: the clip pauses);
// bit 4 cleared.
bool takedownEnterAir(CharacterAir& air, bool gunHitInAir);

// The branch 0x1d6a64 of the animation event handler for one playing clip: a clip with the track kFallAllowedTrack in a sub-animation of the kind
// kFallAllowedAspect sets bit 4 when the track's value at the normalised time is 1.0 and clears it otherwise; other clips leave it. The value is
// the last key at or before the time (the step sampling of the footstep track).
void sampleFallAllowed(CharacterAir& air, const assets::Animation& clip, float normalisedTime);

// 0x2b24bc's head on a ground contact: a character with the auto-arrest mark (sub +0x131 bits 6..7 = 2) requests FALL ARREST HIT unless 0x28b46c
// holds for the state +0x6e (227..231, 307, 325, 409).
bool fallArrestHitOnLanding(bool marked, std::uint16_t state);

// Which update of a state moves the character here: 0x27726c of MELEE PUTDOWN (227), or 0x283b10 of the takedown states registered at 0x279da0
// (0x122..0x12f, 0x133..0x136, 0x139..0x13e).
enum class TakedownMover { None, Putdown, Takedown };
TakedownMover takedownMover(std::uint16_t state);

}
