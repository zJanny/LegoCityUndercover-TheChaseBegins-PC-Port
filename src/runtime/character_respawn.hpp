#pragma once
// What happens to a character after message 1 of 0x18e4c8 (docs/re/script-natives-world-death.md, entry 2026-10-19 "The respawn tail"): the AI
// after-death 0x21e218 of a character that stays dead, and the locomotion state DEAD (11) of one that comes back with its spawn counter (record byte
// +0x2ce) of 0x225f5c and the DEAD update 0x1d4ba0. No SDL dependency.
#include <cstdint>
#include <optional>

namespace chase::runtime {

// 0x21e218: with the global bytes 0x3771d7 & 0x377540 or `AIFriendly` the record +0xc8 is freed (0x215988) and either `AIMillAbout` (record +0x315
// bit 7, also set while 0x22b0b4 holds) hands over to 0x22471c or the node goes back to its spawn place (the placement matrix of 0x2364cc, else the
// set-up position record +0xac) with 0x22f03c and the idle 0x2246c0; otherwise 0x21decc takes the character out of the engaged table 0x58deb0 /
// 0x58dec8 (+0xd8 bit 5).
enum class CharacterAfterDeath : std::uint8_t { LeaveEngaged, BackToSpawn, MillAbout };

struct CharacterAfterDeathInput {
    bool friendly = false;           // record +0xdb bit 2 (`AIFriendly`)
    bool millAbout = false;          // record +0x315 bit 7 (`AIMillAbout`, 0x1c6e84)
    bool respawnFlags = false;       // signed bytes 0x3771d7 & 0x377540 (writers not read; STAND-IN SCRIPT-S268: clear)
};

CharacterAfterDeath characterAfterDeath(const CharacterAfterDeathInput& input);

// 0x225f5c counts +0x2ce up to 136 once per character update; the DEAD enter 0x1ab8d4 sets it to 0. While the state is DEAD, the update 0x1d4ba0 of a
// non-player waits for a counter above 64, then raises the event *0x3775ec (when `AIFriendly` and `AIRespawnIfFriendly`) and requests IDLE (1).
// Outside DEAD a counter of 66..136 shows the node (0x23f734 with 1 when its flag bit 5 is set) and, unless the kind is 159, sets the character colour
// 0x18b0c8.
inline constexpr std::uint8_t kComeBackCounterEnd = 136;
inline constexpr std::uint8_t kComeBackDeadWait = 64;
inline constexpr std::uint8_t kComeBackShowAfter = 65;
inline constexpr std::uint8_t kNoComeBackColourKind = 159;

struct CharacterComeBack {
    std::uint8_t counter = 0;   // +0x2ce
    bool dead = true;           // locomotion state 11 (DEAD), the halfword record +0x6c
};

struct CharacterComeBackStep {
    bool leavesDead = false;            // 0x2414b4(entity, ctrl, 1, 1)
    bool raisesEvent = false;           // 0x23cee8(*0x3775ec, entity, entity, 255, 0)
    bool shown = false;                 // the node shown in this update
    std::optional<std::uint32_t> colour;  // 0x18b0c8: bytes r, g, b, a
};

// One update of a non-player character that came back. STAND-IN SCRIPT-S268: 0x225f5c runs before the state update (the order inside the character
// update 0x1835d0 was not read); with the other order IDLE and the event come one update later, the node is shown in the same update.
CharacterComeBackStep characterComeBackStep(CharacterComeBack& state, bool respawnEvent, std::uint8_t kind);
// 0x225fe4..0x2260f8: n = counter & 15 folded to 0..7 (15 - n from 8); colour (255, n x 32 | 31, n x 32 | 31, 255), white at 136.
std::uint32_t characterComeBackColour(std::uint8_t counter);

}
