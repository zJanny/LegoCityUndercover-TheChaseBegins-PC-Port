#pragma once
// Rows of the slot list for a used save file (docs/re/save-format-state.md, builder 0x2cc298): play time, percent and
// the level name key of each slot. No SDL.
#include "assets/assets.hpp"
#include "frontend/flow.hpp"

#include <string>
#include <vector>

namespace chase::frontend {

// Throws when the file is missing or not a valid save.
std::vector<SlotSummary> slotSummariesFromSave(const std::string& file, assets::FuseArchive& archive, int language);

}
