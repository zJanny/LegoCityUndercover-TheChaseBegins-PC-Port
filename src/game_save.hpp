#pragma once
// --save FILE --slot N of chase-game: the progress of a real save slot for the level start (docs/re/save-format.md).
#include "assets/assets.hpp"
#include "runtime/script_runner.hpp"
#include "save/completion_tables.hpp"
#include "save/slot_progress.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace game_save {

constexpr std::size_t kSlotUsedByte = 7;
constexpr std::uint8_t kSlotUsedBit = 4;

inline std::string sceneNameOf(const std::string& levelPath) {
    const std::string folder = levelPath.substr(0, levelPath.find_last_of('/'));
    return folder.substr(folder.find_last_of('/') + 1);
}

inline chase::save::SlotStart load(const std::string& file, unsigned slotIndex, chase::assets::FuseArchive& archive,
                                   const chase::assets::Level& level, const std::string& levelPath) {
    using namespace chase::save;
    const LoadResult loaded = chase::save::load(file);
    if (!loaded.found) throw std::runtime_error("save file not found: " + file);
    if (loaded.status != Status::Ok || !loaded.image) throw std::runtime_error("not a valid save file: " + file);
    if (slotIndex >= kSlotCount) throw std::runtime_error("--slot must be 0 or 1");
    const SlotView slot = loaded.image->slotView(slotIndex);
    if (!(slot.data[kSlotUsedByte] & kSlotUsedBit)) throw std::runtime_error("save slot " + std::to_string(slotIndex) + " is empty");

    const chase::assets::FuseEntry* levelTable = archive.find("tables/tableslevels.bxls");
    if (!levelTable) throw std::runtime_error("TablesLevels missing in the archive");
    const unsigned levelId =
        levelIdForScene(chase::assets::parseTable(archive.read(*levelTable)), sceneNameOf(levelPath)).value_or(0);
    const CompletionTotals totals = completionTotals(archive);
    return slotStart(slot, level, levelId, totals.levelTotals[kGameCompleteLevel - kFirstCounterLevel]);
}

inline void apply(const chase::save::SlotStart& start, chase::runtime::ScriptRunner& runner) {
    chase::runtime::ScriptProgress progress;
    progress.levelUnlocked = start.levelUnlocked;
    progress.gameComplete = start.gameComplete;
    runner.setProgress(progress);
    for (const chase::save::SavedVariable& variable : start.variables) runner.setVariable(variable.hash, variable.value);
}

}
