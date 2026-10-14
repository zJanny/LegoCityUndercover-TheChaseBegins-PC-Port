#include "frontend/slot_summaries.hpp"
#include "save/continue_start.hpp"
#include "save/save.hpp"
#include "save/save_writer.hpp"

#include <stdexcept>

namespace chase::frontend {

std::vector<SlotSummary> slotSummariesFromSave(const std::string& file, assets::FuseArchive& archive, int language) {
    const save::LoadResult loaded = save::load(file);
    if (!loaded.found) throw std::runtime_error("save file not found: " + file);
    if (loaded.status != save::Status::Ok || !loaded.image) throw std::runtime_error("not a valid save file: " + file);
    const assets::FuseEntry* levelEntry = archive.find("tables/tableslevels.bxls");
    if (!levelEntry) throw std::runtime_error("TablesLevels missing in the archive");
    const assets::Table levels = assets::parseTable(archive.read(*levelEntry));
    std::vector<SlotSummary> summaries;
    for (std::size_t index = 0; index < save::kSlotCount; ++index) {
        const save::SlotView slot = loaded.image->slotView(index);
        SlotSummary summary;
        if (save::slotInUse(slot)) {
            const std::optional<save::SlotRow> row = save::slotRow(slot);
            if (row) {
                summary.used = true;
                summary.time = save::formatPlayTime(row->playTicks);
                summary.percent = save::formatPercent(row->permille, std::uint8_t(language));
                summary.locationKey = save::locationTextHash(levels, save::locationLevel(row->levelByte)).value_or(0);
            }
        }
        summaries.push_back(summary);
    }
    return summaries;
}

}
