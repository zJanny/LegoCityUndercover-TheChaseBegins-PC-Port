#pragma once
// The slot of a new game and the level start from a slot (docs/re/save-format-state.md): New Game on an empty slot resets
// the live slot (0x228274(1, slot): zero fill, the slot callback 0x1a25cc, used bit), and the front end then enters the
// level through 0x20a4d4. Links chase_save and chase_assets.
#include "assets/assets.hpp"
#include "save/continue_start.hpp"
#include "save/save.hpp"
#include "save/save_writer.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace chase::save {
constexpr std::uint8_t kSlotOwnPartyBit = 1;            // slot +7 bit 0: 0x20a4d4 keeps the party at +0x5201
constexpr std::size_t kSlotPartyOffset = 0x5201;        // 0x5208b1: 8 character ids
constexpr std::size_t kSlotLevelOneGroupOffset = 0x51b3;
constexpr std::size_t kSlotGroupBitsOffset = 0x51bb;    // 0x52086b
constexpr std::size_t kSlotCharacterBitsOffset = 0x51bc;
constexpr std::size_t kSlotCharacterNewOffset = 0x5211; // 0x5208c1
constexpr std::size_t kSlotAbilityOffset = 0x51ee;      // 0x52089e, 64 bits
constexpr std::size_t kSlotCustomisationOffset = 0x5150; // 0x520800: 8 records of 6 bytes
constexpr unsigned kAbilityBits = 64;
constexpr unsigned kCustomisationRecords = 8;
constexpr std::size_t kCustomisationRecordSize = 6;
constexpr unsigned kCustomisationParts = 4;
constexpr unsigned kNewGameLevel = 2;                   // 0x1a25cc stores 2 at slot +6; 0x20a4d4 turns level 0 into 2

// Byte 0x3741b4[i]: the first character of group i; 0 is searched as character 110.
constexpr std::array<std::uint8_t, kCustomisationRecords> kGroupCharacters = {9, 19, 29, 39, 49, 59, 69, 0};
constexpr std::uint8_t kEmptyGroupCharacter = 110;

// The rows of TablesCharacters sheet 0 (*0x372e10, stride 0x28) the slot callback reads.
struct NewGameCharacter {
    std::uint8_t startGroup = 0;  // +0xc: ids 9..140 with a value below 2 are unlocked
    bool secondBit = false;       // +0x4 (int) is 0: 0x231fc0 also sets bit 1
    bool ownCustomisation = false; // +0x21 is 1: 0x2cd154 clears bit 1 of the record flags
    std::uint8_t linked = 0;      // +0x22: unlocked with it (recursion of 0x231fc0)
    std::uint8_t next = 0;        // +0x23: unlocked after it (loop of 0x231fc0)
};

struct NewGameTables {
    std::array<std::uint8_t, 2> levelOneGroup{};            // TablesLevels row 1 bytes +0x43, +0x44 (*0x375914 + 0x97)
    std::vector<std::array<std::uint8_t, 8>> levelParty;    // TablesLevels rows 0..: bytes +0x43..+0x4a
    std::vector<std::array<std::uint8_t, 2>> levelPartyExtras;  // TablesLevels rows 0..: bytes +0x4b, +0x4c (0x1b1c90 into 0x37590c)
    std::vector<NewGameCharacter> characters;               // TablesCharacters sheet 0 rows 0..140
    std::array<bool, kAbilityBits> abilityLocked{};         // TablesCharacters sheet 2 (*0x372cb8, stride 8) byte +6 != 0
};

// Throws std::runtime_error when a sheet is short or has no stored column at a byte offset the code reads.
NewGameTables newGameTables(const assets::Table& levels, const assets::Table& characters);
NewGameTables newGameTables(assets::FuseArchive& archive);

// 0x231fc0(id): 0x232028(id, 0), 0x232028(id, 1) when the row's +4 is 0, the linked id first, then the next id.
void unlockCharacterChain(std::uint8_t* slot, const NewGameTables& tables, unsigned id);

// STAND-IN SAVE-S6: the part indices of the eight customisation records (+0x5150, bytes 0..3 of each record), which
// 0x2cd154 looks up in four 0xcf30-byte tables read from the stream 0x5145cc (not read), are the bytes of the live slot
// in the oracle state gameplay-start. The id and flag bytes are computed.
constexpr std::array<std::array<std::uint8_t, kCustomisationParts>, kCustomisationRecords> kNewGameCustomisationPartsStandIn = {{
    {0x00, 0x00, 0x00, 0x00}, {0x09, 0x09, 0x0a, 0x01}, {0x12, 0x00, 0x14, 0x0f}, {0x1b, 0x00, 0x1e, 0x14},
    {0x22, 0x00, 0x28, 0x1d}, {0x2c, 0x00, 0x30, 0x24}, {0x31, 0x00, 0x3a, 0x01}, {0x51, 0x07, 0x5d, 0x44},
}};

// 0x228274(1, slotIndex): the zero-filled slot after the callback 0x1a25cc and with the used bit +7 bit 2. The cheat
// flags 0x375961 bit 7 and 0x375962 bits 3 and 4 are clear: no code of the executable sets them (docs/re/save-format-state.md).
Bytes newGameSlot(const NewGameTables& tables);
// The option bytes 0x1a25cc writes for the slot index (0..2): sound volume +4+slot = 10, music volume +6+slot = 8 and
// byte +0x1ec + slot * 0x18 = 0.
void resetSlotOptions(SaveImage& image, unsigned slotIndex);

struct LevelEntry {
    unsigned level = 0;
    std::uint64_t studs = 0;        // slot +0x5148 into 0x593418
    std::uint32_t superBricks = 0;  // slot +0x5190 into 0x593430
};
// 0x20a4d4(slot, level) on the live slot: level 0 becomes 2; the stud count and the super brick count are read from
// the slot; 0x1947a0 sets the unlock bit 2 * level and slot +6 for levels 2..8; without slot +7 bit 0 the party
// +0x5201..+0x5208 is taken from TablesLevels row `level` bytes +0x43..+0x4a. The level load is the caller's.
// Returns the counts; the slot must be kSlotSize bytes (throws std::runtime_error otherwise).
LevelEntry enterLevel(std::vector<std::uint8_t>& slot, unsigned level, const NewGameTables& tables);
}
