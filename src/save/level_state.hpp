#pragma once
// The level state refresh of a save request (0x192f60 with the serialiser 0x1c26dc, docs/re/save-format-state.md):
// the records of the level's objects, the end marker and the end bit set go into the level block of the slot.
// Links chase_save and chase_assets.
#include "assets/assets.hpp"
#include "save/slot_progress.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::save {

struct LevelStateInput {
    // The live value of a level variable by name hash (empty: the default of the parameter table).
    std::function<std::optional<float>(std::uint32_t)> variable;
    // STAND-IN SAVE-S5: the object records the port does not produce are the records of the slot's own saved state,
    // written back in their order (a 4-byte record with the live TASKS_COMPLETE); the end bit set keeps the length of
    // that state and is empty for a level without one (the count at scene + 0x12f4 is not read).
    std::vector<ObjectRecord> keptRecords;
    std::size_t endBitSetBytes = 0;
};

// 0x1c26dc in mode 2 (0x376aac): the level-data record (name hash 0) holds the parameters of the level's table with
// flag bit 1 in table order (types 0 float, 5 and 7 integer: 4 bytes; type 1: the string and its NUL; type 2: 12
// bytes), each record a header {u32 name hash, u8 size, 3 zero bytes} and its payload, then 0xffffffff and the end bit
// set (all zero in mode 2). Empty when the data would not fit into the 0x800 bytes of the block. STAND-IN SAVE-S5:
// strings and vectors are the table's defaults (the port keeps no live string or vector variables).
std::optional<std::vector<std::uint8_t>> levelStateData(const assets::Level& level, const LevelStateInput& input);

// 0x192f60 on level block `levelId` (1..8): the data at +0x1f, its length at +0x1a, +0x1e = 1 and, when the level has a
// StoryProgress parameter, its value as an integer (0x2451d0: types 5 / 7 as stored, type 0 truncated) at +0x1c.
// False when the slot is short, the level has no block or the data does not fit.
bool writeLevelState(std::vector<std::uint8_t>& slot, unsigned levelId, const std::vector<std::uint8_t>& data,
                     std::optional<std::int32_t> storyProgress);

// The StoryProgress value 0x2451d0 returns for the level: empty when the level has no such parameter.
std::optional<std::int32_t> storyProgressOf(const assets::Level& level, const LevelStateInput& input);
}
