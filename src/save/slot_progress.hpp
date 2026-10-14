#pragma once
// What a level start needs from a slot: the saved level state block, the level unlock bits, the completion test of
// GetGameComplete and the saved script variables (docs/re/save-format.md). Links chase_save and chase_assets.
#include "assets/assets.hpp"
#include "save/save.hpp"

#include <optional>
#include <string>
#include <vector>

namespace chase::save {
constexpr std::size_t kLevelStateLengthOffset = 0x1a;
constexpr std::size_t kLevelStoryProgressOffset = 0x1c;
constexpr std::size_t kLevelStateSavedOffset = 0x1e;
constexpr std::size_t kLevelStateDataOffset = 0x1f;
constexpr std::size_t kLevelStateCapacity = 0x800;
constexpr std::size_t kSlotCurrentLevelOffset = 6;
constexpr std::size_t kLevelUnlockBitsOffset = 0x51e0;
constexpr std::size_t kObjectRecordHeader = 8;
constexpr unsigned kUnlockLevels = 9;
constexpr unsigned kGameCompleteLevel = 8;
constexpr std::size_t kLevelNameColumn = 21;  // TablesLevels sheet 0, the string at row +0x30

struct LevelStateView {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::uint16_t storyProgress = 0;
};

// Level block 1..8 of 0x203464. Empty when the level has no saved state (byte +0x1e is 0), the length exceeds the
// 0x800 data area or the slot is short.
std::optional<LevelStateView> levelState(SlotView slot, unsigned level);
// 0x21d450 without the cheat flag: bit 2 * level + which of the set at +0x51e0, level 0..8; which 0 = unlocked, 1 = second state.
std::optional<bool> levelUnlockBit(SlotView slot, unsigned level, unsigned which);
// 0x2635d0: the collected byte of level 8 reaches `finalLevelTotal` (TablesLevels row 8, byte +0x40).
std::optional<bool> gameComplete(SlotView slot, std::uint8_t finalLevelTotal);
// Row of TablesLevels sheet 0 whose scene name equals `sceneName` (case-insensitive), 1..8.
std::optional<unsigned> levelIdForScene(const assets::Table& levels, const std::string& sceneName);

struct SavedVariable {
    std::uint32_t hash = 0;
    float value = 0;
};
// The level-data record (name hash 0) of the saved state: the default parameters of `level` with record flag bit 1,
// in file order (restore 0x1c2400). Types 0, 5 and 7 give variables; strings and vectors are skipped. Empty when the
// state holds no such record or its size does not match the parameter table.
std::optional<std::vector<SavedVariable>> levelVariables(const assets::Level& level, const LevelStateView& state);

struct ObjectRecord {
    std::uint32_t hash = 0;
    std::vector<std::uint8_t> payload;
};
struct ObjectRecords {
    std::vector<ObjectRecord> records;
    std::size_t endBitSetBytes = 0;
    bool endBitSetZero = true;
};
// The object records of the saved state (restore 0x1c2400): every record but the level-data record (hash 0) up to the
// end marker, then the zero padding of the end bit set. Empty when the data has no end marker or a record overruns it.
std::optional<ObjectRecords> objectRecords(const LevelStateView& state);
// Slot byte +6: the current level id written when a level is entered (0x1947e0, 0x2f68ec) and read by the front end (0x2b1100).
std::optional<unsigned> slotCurrentLevel(SlotView slot);
// Restore callback of the class with a 4-byte record (object type 0xfc, 0x192804 save, 0x1927a8 restore): the value of
// the scene variable "TASKS_COMPLETE". The classes with 1-byte records (types 0x15, 0x43, 0x62) are not applied.
constexpr std::size_t kTasksCompleteRecordSize = 4;
std::vector<SavedVariable> tasksCompleteVariables(const ObjectRecords& records);

struct SlotStart {
    unsigned level = 0;
    bool stateSaved = false;
    std::uint16_t storyProgress = 0;
    std::uint32_t levelUnlocked = 0;  // bit n: level n unlocked
    bool gameComplete = false;
    std::vector<SavedVariable> variables;
    std::vector<ObjectRecord> objectRecords;
    std::optional<unsigned> currentLevel;
    std::vector<std::uint8_t> slotBytes;  // the whole slot block, for the progress natives (natives_progress.hpp)
};
// Throws std::runtime_error when the slot is too short or a saved state does not match `level`.
SlotStart slotStart(SlotView slot, const assets::Level& level, unsigned levelId, std::uint8_t finalLevelTotal);
}
