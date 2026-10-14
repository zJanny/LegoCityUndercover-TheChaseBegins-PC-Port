#pragma once
// Where Continue on a used slot starts (docs/re/save-format.md): the level of slot byte +6 (0x2e7ce8 copies it to the
// front-end structure at 0x3741d4 + 1, the main loop 0x2d5874 passes it to 0x20a4d4, which loads it; 0 becomes level 2),
// entered at the level's own "Player1Start" (spawn 0x2e6264). No SDL dependency.
#include "save/slot_progress.hpp"

#include <string>

namespace chase::save {

constexpr std::size_t kSlotFlagsOffset = 7;
constexpr std::uint8_t kSlotInUseBit = 4;
constexpr unsigned kEmptyLevelFallback = 2;

struct ContinueStart {
    unsigned level = 0;
    std::string scene;
    std::string levelPath;
};

bool slotInUse(SlotView slot);
// Scene name of TablesLevels sheet 0 row `level` (1..8), empty when the row has none.
std::string sceneNameOfLevel(const assets::Table& levels, unsigned level);
std::string levelPathOfScene(const std::string& scene);
// Throws std::runtime_error when the slot is empty or its level has no scene in the table (level 9 and above).
ContinueStart continueStart(SlotView slot, const assets::Table& levels);

}
