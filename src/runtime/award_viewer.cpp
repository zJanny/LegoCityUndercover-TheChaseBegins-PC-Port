#include "runtime/award_viewer.hpp"
#include "runtime/script_runner.hpp"

namespace chase::runtime {

void addLevelAwards(AwardCounts& counts, const LevelAwardData& level) {
    for (const unsigned category : kAwardBronzeCategories) counts.bronze = std::uint16_t(counts.bronze + level.collected[category]);
    for (const unsigned category : kAwardBronzeCategories) counts.bronzeTotal = std::uint16_t(counts.bronzeTotal + level.totals[category]);
    counts.silverTotal = std::uint16_t(counts.silverTotal + level.missions - 1);
    counts.goldTotal = std::uint16_t(counts.goldTotal + 1);
    if (!level.done || *level.done == 0) return;
    counts.silver = std::uint16_t(counts.silver + *level.done);
    if (level.missions <= *level.done) {
        counts.silver = counts.silverTotal;
        counts.gold = std::uint16_t(counts.gold + 1);
    }
}

AwardCounts awardCountsOfItem(unsigned item, const std::array<LevelAwardData, kAwardLevels>& levels) {
    AwardCounts counts;
    if (item == kAwardGeneralItem) {
        for (const LevelAwardData& level : levels) addLevelAwards(counts, level);
    } else if (item >= kAwardFirstLevel && item <= kAwardLastLevel) {
        addLevelAwards(counts, levels[item - kAwardFirstLevel]);
    }
    return counts;
}

void ScriptRunner::awardViewerNative() { awardViewerRequested_ = true; }

bool ScriptRunner::takeAwardViewerRequest() {
    const bool requested = awardViewerRequested_;
    awardViewerRequested_ = false;
    return requested;
}

}
