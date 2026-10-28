#pragma once
// The award viewer of the police station (docs/re/ui-screens.md): the script native AwardViewer 0x1b23e4 pushes mode 0x514030 (0x175790);
// the mode's enter 0x2f2184 and the page 0x514058 (vtable 0x363a58) count the awards of the save slot with 0x1f61f0. No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>

namespace chase::runtime {

constexpr unsigned kAwardCategories = 13;
// 0x1f61f0 sums the collected counts 0x20a630 and the totals 0x20a7fc of these twelve categories, in this order; category 12 is not counted.
constexpr std::array<unsigned, 12> kAwardBronzeCategories = {7, 8, 9, 10, 11, 0, 1, 5, 4, 3, 6, 2};
constexpr unsigned kAwardFirstLevel = 2, kAwardLastLevel = 8;
constexpr unsigned kAwardLevels = kAwardLastLevel - kAwardFirstLevel + 1;
// Page enter 0x304e20: the round menu lists the items 1..8; item 1 is "General" (all levels), items 2..8 are the levels of TablesLevels rows 2..8.
constexpr unsigned kAwardGeneralItem = 1;
constexpr unsigned kAwardViewerItems = 8;
constexpr unsigned awardViewerItem(unsigned index) { return index + 1; }

// The six u16 counts of the mode object +0x6c..+0x76 (0x1f61f0 param [0]..[5]); the top screen draws bronze, silver and gold from left to right.
struct AwardCounts {
    std::uint16_t bronze = 0, silver = 0, gold = 0;
    std::uint16_t bronzeTotal = 0, silverTotal = 0, goldTotal = 0;
};

// What 0x1f61f0 reads for one level.
struct LevelAwardData {
    std::array<std::uint16_t, kAwardCategories> collected{};  // 0x20a630(category, level)
    std::array<std::uint8_t, kAwardCategories> totals{};      // 0x20a7fc(category, level): TablesSuperCounters
    std::uint8_t missions = 0;                                // TablesLevels row byte +0x40
    std::optional<std::uint8_t> done;                         // byte +0x81f of the level block 0x203464; empty without a block
};

// 0x1f61f0(counts, level): adds one level, with the game's u16 wrap.
void addLevelAwards(AwardCounts& counts, const LevelAwardData& level);
// The counts of a round-menu item: 0x2f2184 and page 0x304d70 sum levels 2..8 for item 1 and count only that level for items 2..8.
// levels[i] is level kAwardFirstLevel + i.
AwardCounts awardCountsOfItem(unsigned item, const std::array<LevelAwardData, kAwardLevels>& levels);

}
