#pragma once
// Builds the completion totals of docs/re/save-format.md (09:19, 09:25) from the shipped tables.
// Links chase_save and chase_assets; neither library depends on the other.
#include "assets/assets.hpp"
#include "save/save.hpp"

namespace chase::save {
constexpr std::size_t kLevelTotalColumn = 26;      // TablesLevels sheet 0, row byte +0x40
constexpr std::size_t kFirstCounterColumn = 1;     // TablesSuperCounters sheet 0, columns 1..7
constexpr std::size_t kExclusionColumn = 4;        // TablesCharacters sheet 0, row byte +0xc

// Throws std::runtime_error when a sheet is missing, short or holds a value that is not a stored byte.
CompletionTotals completionTotals(const assets::Table& levels, const assets::Table& superCounters,
                                  const assets::Table& characters);
// Reads tables/tableslevels.bxls, tablessupercounters.bxls and tablescharacters.bxls from the archive.
CompletionTotals completionTotals(assets::FuseArchive& archive);
}
