#include "save/completion_tables.hpp"
#include <stdexcept>
#include <string>

namespace chase::save {
namespace {
const assets::TableSheet& firstSheet(const assets::Table& table, const char* name, unsigned minRows,
                                     std::size_t minColumns) {
    if (table.sheets.empty()) throw std::runtime_error(std::string(name) + " has no sheet");
    const assets::TableSheet& sheet = table.sheets[0];
    if (sheet.rows < minRows || sheet.columns.size() < minColumns)
        throw std::runtime_error(std::string(name) + " sheet 0 is too small");
    return sheet;
}

std::uint8_t byteCell(const assets::TableSheet& sheet, unsigned row, std::size_t column, const char* name) {
    assets::TableValue value = assets::tableValue(sheet, row, column);
    if (!value.present || value.bits > 0xff) throw std::runtime_error(std::string(name) + " cell is not a stored byte");
    return std::uint8_t(value.bits);
}

assets::Table loadTable(assets::FuseArchive& archive, const std::string& path) {
    const assets::FuseEntry* entry = archive.find(path);
    if (!entry) throw std::runtime_error("not in archive: " + path);
    return assets::parseTable(archive.read(*entry));
}
}

CompletionTotals completionTotals(const assets::Table& levels, const assets::Table& superCounters,
                                  const assets::Table& characters) {
    CompletionTotals totals;
    const assets::TableSheet& levelSheet =
        firstSheet(levels, "TablesLevels", kLastCounterLevel + 1, kLevelTotalColumn + 1);
    for (unsigned level = kFirstCounterLevel; level <= kLastCounterLevel; ++level)
        totals.levelTotals[level - kFirstCounterLevel] = byteCell(levelSheet, level, kLevelTotalColumn, "TablesLevels");

    const assets::TableSheet& counterSheet =
        firstSheet(superCounters, "TablesSuperCounters", kLevelCategories, kFirstCounterColumn + kCounterLevels);
    for (unsigned category = 0; category < kLevelCategories; ++category)
        for (unsigned index = 0; index < kCounterLevels; ++index)
            totals.categoryTotals[category][index] =
                byteCell(counterSheet, category, kFirstCounterColumn + index, "TablesSuperCounters");

    const assets::TableSheet& characterSheet =
        firstSheet(characters, "TablesCharacters", kCharacterIdLimit, kExclusionColumn + 1);
    for (unsigned id = kFirstCompletionCharacter; id < kCharacterIdLimit; ++id)
        if (byteCell(characterSheet, id, kExclusionColumn, "TablesCharacters") == 1) totals.excludedCharacters.push_back(std::uint8_t(id));
    return totals;
}

CompletionTotals completionTotals(assets::FuseArchive& archive) {
    return completionTotals(loadTable(archive, "tables/tableslevels.bxls"),
                            loadTable(archive, "tables/tablessupercounters.bxls"),
                            loadTable(archive, "tables/tablescharacters.bxls"));
}
}
