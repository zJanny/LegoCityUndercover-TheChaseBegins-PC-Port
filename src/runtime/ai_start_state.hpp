#pragma once
// The StartState table of the AI spawner class (docs/re/scripts-missions-1-4-spawners.md): 16-byte entries at 0x58db48 (BSS) filled by 0x201a3c
// from the class registrations 0x288e14 (entries 0, 1, 2, 3, 6, 7) and 0x26a27c (4, 5, 8, 9). Word +0 is the create (its record is the spawner's
// +0x5c), +4 the post-link, +8 the free and +0xc the spawn, called by the state change 0x268978 with (spawner, character, slot +0x2b, record) once
// the character came out. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/ai_super_jump.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr std::size_t kAiStartStateEntries = 10;
constexpr std::size_t kAiStartStateTargets = 10;           // Runto1 .. Runto10 (post-link 0x2a07b4) or FollowPath1 .. 10 (0x2b0dc4)
constexpr float kAiStartSuperJumpHeight = 3.0f;            // default of SuperjumpHeight, literal 0x40400000 at 0x2c0b30
constexpr std::int32_t kAiStartSuperJumpFrames = 28;       // frames 0x2bae74 passes to 0x213300 (mov r2, #28 at 0x2baec4)
static_assert(kAiStartSuperJumpFrames == kSuperJumpNativeFrames, "entry 6 and AISuperJump pass the same frames to 0x213300");

struct AiStartStateEntry {
    std::uint32_t create = 0, postLink = 0, free = 0, spawn = 0;
};
// The words 0x288e14 and 0x26a27c register; nothing for an index above 9.
std::optional<AiStartStateEntry> aiStartStateEntry(std::uint16_t index);

enum class AiStartTargetKind : std::uint8_t { None, RunTo, FollowPath };
// RunTo for entries 0, 1, 2 and 6 (post-link 0x2a07b4), FollowPath for 8 and 9 (0x2b0dc4).
AiStartTargetKind aiStartTargetKind(std::uint16_t index);

// The record of the entry's create and post-link: the per-slot targets, ForceMove (create 0x2cc73c of entry 0, low byte, bit 0 used) and
// SuperjumpHeight (create 0x2c0af4 of entry 6, record +0x28).
struct AiStartState {
    std::uint16_t index = 0;
    std::array<std::string, kAiStartStateTargets> targets{};  // lower-case object (RunTo) or path name (FollowPath), empty when unset
    bool forceMove = false;
    float superJumpHeight = kAiStartSuperJumpHeight;
};

// `names` maps a reference hash to the lower-case instance (0x24242c with flags 0x04000010 names an object).
AiStartState aiStartStateOf(std::uint16_t index, const assets::LevelObject& object, const assets::Level& cls,
                            const std::map<std::uint32_t, std::string>& names);

// What the spawn word does to the spawned character. Requested numbers are the AI component byte +0x15 (docs/re/ai-natives.md).
enum class AiStartAction : std::uint8_t {
    None,           // no entry (index above 9), or entries 2 / 6 without a target: the original reads through a null pointer
    Idle,           // entry 0 without a target: 0x2246c0, requested state 0
    Move,           // entry 0: 0x22d85c(character, target, avoid = ForceMove bit 0, walk 0, 0): the AIMove front end, running
    RequestState6,  // entry 1: 0x268e88, target to +0xb8, requested state 6
    RequestState7,  // entry 1 without a target: 0x268e88 with null, requested state 7
    RequestState8,  // entry 2: target to +0xb8 and +0xac, 0x218b38 requests state 8
    MillAbout,      // entry 3: 0x22471c (only with +0x315 bit 7: requested state 9)
    PlaceBelow,     // entry 4: 0x2b7fe0, node lowered by 1.1 x (entity +0x64 + +0x70), heading from the node, locomotion state 112
    PlaceBehind,    // entry 5: 0x2aaf40, node moved along its third row by -4.5 x (entity +0x68 + +0x74), heading from the node, locomotion state 112
    SuperJump,      // entry 6: target to +0xb8 and +0xac, 0x213300(character, target, 28, SuperjumpHeight)
    NoState,        // entry 7: 0x241304, requested state 1
    FollowPath,     // entry 8: 0x1fbde4(character, path, 0, 0, +0xdb bit 4)
    PathLocomotion, // entry 9: path +8 to sub-structure +0xe4, locomotion state 398
};
const char* aiStartActionName(AiStartAction action);

struct AiStartOrder {
    AiStartAction action = AiStartAction::None;
    std::string target;
    bool avoid = false;
    float height = 0;
    std::int32_t frames = 0;
};

// The spawn word of `state.index` for the character of slot `slot` (the compacted slot index +0x2b).
AiStartOrder aiStartOrder(const AiStartState& state, std::size_t slot);

class AiCore;
// Gives `order` to the character standing at `position` (the node the release put it at). Idle, Move, MillAbout, SuperJump and NoState run
// through AiCore as the natives' handlers that call the same functions; the others are not ported (STAND-IN SCRIPT-S324). TargetNotFound: AiCore
// cannot locate the target object, so nothing changes.
// STAND-IN SCRIPT-S324: a Move always runs the path finder; the original's gate 0x1f9834 refuses a search without the avoid argument (ForceMove 0)
// when the search byte 0x3771d8 is 0 (0x26aa1c sets it to 1, each search clears it).
enum class AiStartApplied : std::uint8_t { Applied, NotPorted, TargetNotFound };
AiStartApplied applyAiStartOrder(const AiStartOrder& order, const std::string& character, const std::array<float, 3>& position, AiCore& ai);

}
