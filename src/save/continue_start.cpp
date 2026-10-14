#include "save/continue_start.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace chase::save {

bool slotInUse(SlotView slot) { return slot.size > kSlotFlagsOffset && (slot.data[kSlotFlagsOffset] & kSlotInUseBit) != 0; }

std::string sceneNameOfLevel(const assets::Table& levels, unsigned level) {
    if (levels.sheets.empty()) return {};
    const assets::TableSheet& sheet = levels.sheets[0];
    if (level < 1 || level > kLastCounterLevel || sheet.rows <= level || sheet.columns.size() <= kLevelNameColumn) return {};
    const assets::TableValue name = assets::tableValue(sheet, level, kLevelNameColumn);
    return name.present ? name.text : std::string();
}

std::string levelPathOfScene(const std::string& scene) {
    std::string folder = scene;
    std::transform(folder.begin(), folder.end(), folder.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return "levels/levels/" + folder + "/" + folder + "_main.blvl";
}

ContinueStart continueStart(SlotView slot, const assets::Table& levels) {
    if (!slotInUse(slot)) throw std::runtime_error("the save slot is empty, there is nothing to continue");
    const std::optional<unsigned> stored = slotCurrentLevel(slot);
    if (!stored) throw std::runtime_error("the save slot is too short to hold its current level");
    ContinueStart start;
    start.level = *stored == 0 ? kEmptyLevelFallback : *stored;
    start.scene = sceneNameOfLevel(levels, start.level);
    if (start.scene.empty())
        throw std::runtime_error("the slot's current level " + std::to_string(start.level) + " has no scene in TablesLevels (levels 1 to 8 are known)");
    start.levelPath = levelPathOfScene(start.scene);
    return start;
}

}
