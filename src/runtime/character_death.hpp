#pragma once
// Message 1 (destroy) of the character handler 0x18e4c8 at 0x18e93c, kinds 27, 121 and 242 (docs/re/script-natives-world-death.md, entry
// 2026-10-19T13:21:05): the death mode `DisableFallapart`, the fall-apart body 0x1d3b50 (runtime/fall_apart.hpp), the bricks 0x23aebc and the
// tail shared by every mode (respawn of the player and of friendly AI, the stud drop 0x21e2e4, the disable, the AI after-death 0x21e218).
// No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/debris.hpp"
#include "runtime/stud_drop.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace chase::runtime {

// `DisableFallapart` truncated to two bits (0x237010: bits 4 and 5 of the sub-structure byte +0x12f). Mode 3 reaches no branch of 0x18e998.
enum class CharacterDeathMode : std::uint8_t { FallApart = 0, Smash = 1, Vanish = 2, None = 3 };

// 0x23aebc(entity, +0xb8, +0xbc, 2, 1, 1): two bricks unless the class names DebrisAmount (0x20c938).
inline constexpr unsigned kCharacterSmashBricks = 2;
// 0x18eb94 / 0x18ec6c: the stud drop 0x21e2e4 of a character that stays dead.
enum class CharacterStudDrop : std::uint8_t { None, Dropped, Thirty };
inline constexpr unsigned kCharacterStudDropThirty = 30;

struct CharacterDeathInput {
    CharacterDeathMode mode = CharacterDeathMode::FallApart;
    bool minifig = true;  // 0x22716c
    // SmashDebrisColour into +0xb8 and SmashDebrisColour2 (else the first colour) into +0xbc, with byte +0x12f bit 1 (0x237044..0x23720c).
    std::optional<std::pair<std::uint32_t, std::uint32_t>> smashColours;
    // Sub-structure byte +0x12e bit 5 (0x18e94c): no store that sets it was found, so it is clear for every character (STAND-IN SCRIPT-S264).
    bool skipsDeathBranch = false;
    bool player = false;              // the entity is the player's (*0x3772c0)
    bool respawnEveryone = false;     // 0x22b0b4: byte 0x5163e8 + 0x25 equals 1 (writer not read; the port keeps it clear, STAND-IN SCRIPT-S264)
    bool friendly = false;            // record byte +0xdb bit 2: `AIFriendly` (0x1c6f04)
    bool respawnIfFriendly = true;    // `AIRespawnIfFriendly` (0x2451d0 default 1)
    bool millAbout = false;           // `AIMillAbout` (0x1c6e84 into record +0x315 bit 7, default 0)
};

struct CharacterDeathSteps {
    bool fallApart = false;           // 0x1d3b50: the fall-apart body, which also hides the entity and plays its sounds
    bool smashBricks = false;         // 0x23aebc with the colours of the input
    bool dropsBodyParts = false;      // 0x21e854 (weapons 0x22af80 / 0x22ae28 and the 12 attachment entities) outside the fall-apart
    bool deathSound = false;          // 0x21e734 (the character's rate-limited sound, record +0x2c2) outside the fall-apart
    bool comesBack = false;           // 0x18eb48: enabled again, node flags 0x23f734, state 11 (0x2414b4)
    CharacterStudDrop studDrop = CharacterStudDrop::None;
    bool hidden = false;              // the entity ends disabled (0x2428c0 in a branch or in the tail)
    bool afterDeathAi = false;        // 0x21e218
};

CharacterDeathSteps characterDeathSteps(const CharacterDeathInput& input);

// 0x18ec60 / 0x18eb94: 0x21e2e4(s0 = s1 = 1.0, mode 1, count 0, the node position, 0x56d400, stack 0, 0, 0, 30, 0, 0), or with byte +0x12e bit 5
// mode 0, count 30 (stack 1, 0, 0, 30, 0, 0). Count 0 reaches no record of 0x19913c: only the two random draws of 0x21e2e4 remain.
std::optional<StudDropRequest> characterStudDropRequest(CharacterStudDrop drop, const DebrisVec3& nodePosition);

// The input as the class parameters of a placed character give it: `DisableFallapart` (0x236d74 at 0x23700c, default 0), the smash colours,
// `AIFriendly` and `AIRespawnIfFriendly`. 0x22716c: for kinds 27 and 149 and for a class with `Boss_IsBoss` (bit 14 of +0xc) the character byte
// +0x2cc (`CharacterEnum`, default 192, 0x236d98) is not 145; for the kinds of the list 0x3743ec (121, 159 and 242, created through 0x23e394) the
// byte +0x12e bit 7, which the create of 121 sets from `IsMinifig` (0x16c220) and the create of 242 clears (0x15a428); false for other kinds.
CharacterDeathInput characterDeathInput(std::uint8_t kind, const assets::LevelObject& object, const assets::Level& cls);

// 0x20c938 for 0x23aebc(entity, +0xb8, +0xbc, 2, 1, 1): the class's DebrisColour / DebrisColour2 / DebrisAmount win over the smash colours.
DebrisBurst characterSmashBurst(const std::pair<std::uint32_t, std::uint32_t>& colours, const assets::LevelObject& object, const assets::Level& cls,
                                const DebrisVec3& position);

}
