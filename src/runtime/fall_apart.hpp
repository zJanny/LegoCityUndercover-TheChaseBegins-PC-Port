#pragma once
// The fall-apart body of a minifig character (docs/re/script-natives-world-death.md, entry 2026-10-19T13:21:05): the pool 0x374208 of four slots (0x15ce3c
// allocates 608 bytes, 152 per slot), each with the five part objects BrokenHead0N, BrokenBody0N, BrokenArm0N, BrokenLegs0N and BrokenHips0N of
// levels/charbits.blvl; the start 0x1d3b50, the update 0x16c100 -> 0x231a08 with the part motion 0x22b1f8 -> 0x1d27f4, and the release 0x182348.
// Matrices are row major with the translation in row 3 (row vectors); velocities and spins are per update. No SDL dependency.
#include "runtime/debris.hpp"
#include "runtime/game_random.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace chase::runtime {

using FallApartMatrix = std::array<float, 16>;

inline constexpr unsigned kFallApartSlots = 4;
enum class FallApartPart : std::uint8_t { Head = 0, Body = 1, Arm = 2, Legs = 3, Hips = 4 };
inline constexpr unsigned kFallApartParts = 5;
// The pool entry +0x94 (0x15cf58): the sound 0x1d3f34 plays at the character.
inline constexpr std::uint16_t kFallApartSound = 548;
// +0x1 counts the updates of each phase (0x1d3e48, 0x231b70).
inline constexpr std::uint8_t kFallApartPhaseUpdates = 32;
// 0x37716c: 0.075 from the static initialiser 0x350708 and again from the level setup 0x2e65f0 through 0x18cc90; 0x231bf4: 0.5.
inline constexpr float kFallApartGravity = 0.075f;
inline constexpr float kFallApartBounce = 0.5f;

// node +0xa0 / +0xac of a part: the model's mesh bounds (centre and half extents); the motion reads half.x as the radius.
struct FallApartBounds {
    DebrisVec3 centre{};
    DebrisVec3 half{};
};

struct FallApartBody {
    bool enabled = false;
    FallApartMatrix matrix{};
    DebrisVec3 velocity{};  // entry +0x1c + 12 part
    DebrisVec3 spin{};      // entry +0x58 + 12 part: Euler angles per update (0x237c48)
};

enum class FallApartPhase : std::uint8_t { Flying = 1, Blinking = 2 };

struct FallApartSlot {
    bool inUse = false;                   // +0
    std::uint8_t counter = 0;             // +1
    FallApartPhase phase = FallApartPhase::Flying;  // +2
    FallApartPart lowerPart = FallApartPart::Legs;  // +3: 3 + a draw modulo 2
    std::string owner;                    // +4: the character
    // The bounds the head part moves with: those of its child node, the character's head (0x231a7c), else of the dummy itself.
    FallApartBounds headBounds;
    std::array<FallApartBody, kFallApartParts> parts{};
};

// 0x1d27f4 for one part: the line test 0x298274 from the bounds centre in the world to that point plus the unit velocity times the radius plus the
// velocity. Without a hit the part moves by its velocity; with a hit it moves to the contact, the velocity is reflected and halved (0x27c514), and it
// rests for this update (returns true) when its speed was below 2.0 and the new velocity's x and z are below 0.125; otherwise it moves on by the
// rest of its travel and gets a new spin. Then the gravity is taken off the velocity's y and the part turns by its spin about the bounds centre.
bool fallApartStep(FallApartBody& body, const FallApartBounds& bounds, float bounce, float gravity, const DebrisLineTest& test, GameRandom& random);

// STAND-IN SCRIPT-S265: 0x1d3c28 hangs the character's head node (record +0x16c) under the head part with 0x22690c / 0x2268f4 (not read); the
// port keeps the head's offset from the character's world matrix at the start: childWorld * inverse(oldParent) * newParent (rigid matrices).
FallApartMatrix fallApartCarried(const FallApartMatrix& childWorld, const FallApartMatrix& oldParent, const FallApartMatrix& newParent);

class FallApartPool {
public:
    // `partBounds` are the bounds of the five part models (the head dummy's are zero).
    FallApartPool(GameRandom& random, const std::array<FallApartBounds, kFallApartParts>& partBounds) : random_(random), partBounds_(partBounds) {}

    // 0x1d3b50 from the first free slot: the parts at the character's world matrix (the head 0.6 lower, the others 0.8 higher), enabled, with a
    // velocity and a spin from the random draws. Nothing when all four slots are in use (the original then only disables the character).
    std::optional<unsigned> start(const std::string& owner, const FallApartMatrix& world, const std::optional<FallApartBounds>& headBounds);
    // 0x16c100 (called by the game update 0x2e5470): 0x231a08 for every slot in use. `clock` is the value of 0x241098 the blink tests (bit 0
    // clear toggles the parts).
    void update(const DebrisLineTest& test, std::uint32_t clock);

    const std::array<FallApartSlot, kFallApartSlots>& slots() const { return slots_; }
    static bool partUsed(const FallApartSlot& slot, unsigned part) { return part < 3 || part == unsigned(slot.lowerPart); }

private:
    // 0x182348: the slot is free and its parts disabled.
    static void release(FallApartSlot& slot);

    GameRandom& random_;
    std::array<FallApartBounds, kFallApartParts> partBounds_;
    std::array<FallApartSlot, kFallApartSlots> slots_{};
};

}
