// The award viewer (src/runtime/award_viewer.cpp, src/frontend/award_viewer.cpp; docs/re/ui-screens.md) with made-up save values. The counting rule is
// 0x1f61f0 and the page rules are 0x304e20 / 0x304d5c / 0x304d70 / 0x304d2c, read with REA (Ghidra 12.1.2) and llvm-objdump by 20261028-uiport-C on
// 2026-10-28. Storyboard lengths and button centres as in pause_menu_test.cpp (main.bxaml: TransitionOn_Round, TransitionOff_Round, Left_Move 0.5;
// centres from request r38-B-1). No game data needed.
#include "frontend/award_viewer.hpp"
#include "runtime/award_viewer.hpp"
#include "runtime/script_runner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;
using chase::runtime::AwardCounts;
using chase::runtime::LevelAwardData;
constexpr float kDt = 1.0f / 30;
using Levels = std::array<LevelAwardData, chase::runtime::kAwardLevels>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool equal(const AwardCounts& c, int bronze, int silver, int gold, int bronzeTotal, int silverTotal, int goldTotal) {
    return c.bronze == bronze && c.silver == silver && c.gold == gold && c.bronzeTotal == bronzeTotal && c.silverTotal == silverTotal && c.goldTotal == goldTotal;
}

std::string show(const AwardCounts& c) {
    return std::to_string(c.bronze) + " " + std::to_string(c.silver) + " " + std::to_string(c.gold) + " / " + std::to_string(c.bronzeTotal) + " " +
           std::to_string(c.silverTotal) + " " + std::to_string(c.goldTotal);
}

LevelAwardData level(std::uint8_t missions, std::optional<std::uint8_t> done) {
    LevelAwardData data;
    for (unsigned c = 0; c < chase::runtime::kAwardCategories; ++c) {
        data.collected[c] = std::uint16_t(c + 1);
        data.totals[c] = 4;
    }
    data.missions = missions;
    data.done = done;
    return data;
}

void oneLevel() {
    AwardCounts counts;
    chase::runtime::addLevelAwards(counts, level(5, 0));
    require(equal(counts, 78, 0, 0, 48, 4, 1), "categories 0..11 count (1 + ... + 12 = 78, 12 x 4 = 48), category 12 not; missions - 1 silver, one gold: " + show(counts));
    counts = {};
    chase::runtime::addLevelAwards(counts, level(5, 3));
    require(equal(counts, 78, 3, 0, 48, 4, 1), "three missions done add three silver: " + show(counts));
    counts = {};
    chase::runtime::addLevelAwards(counts, level(5, 5));
    require(equal(counts, 78, 4, 1, 48, 4, 1), "all missions done: silver is the silver total, one gold: " + show(counts));
    counts = {};
    chase::runtime::addLevelAwards(counts, level(5, 7));
    require(equal(counts, 78, 4, 1, 48, 4, 1), "more than the missions counts as done: " + show(counts));
    counts = {};
    chase::runtime::addLevelAwards(counts, level(5, std::nullopt));
    require(equal(counts, 78, 0, 0, 48, 4, 1), "a level without a block adds no silver or gold: " + show(counts));
    counts = {};
    chase::runtime::addLevelAwards(counts, level(0, 0));
    require(counts.silverTotal == 0xffff, "missions 0 wraps the u16 silver total");
}

void general() {
    Levels levels;
    for (LevelAwardData& data : levels) data = level(5, 0);
    levels[0] = level(5, 2);
    levels[1] = level(6, 6);
    const AwardCounts all = chase::runtime::awardCountsOfItem(chase::runtime::kAwardGeneralItem, levels);
    require(all.goldTotal == 7 && all.gold == 1 && all.bronze == 7 * 78 && all.bronzeTotal == 7 * 48, "item 1 sums levels 2..8: " + show(all));
    require(all.silverTotal == 4 + 5 + 5 * 4, "silver totals of all levels");
    require(all.silver == 9, "a finished level sets silver to the running silver total (4 + 5), not 2 + 6: " + show(all));
    require(equal(chase::runtime::awardCountsOfItem(3, levels), 78, 5, 1, 48, 5, 1), "item 3 counts level 3 alone");
    require(equal(chase::runtime::awardCountsOfItem(9, levels), 0, 0, 0, 0, 0, 0), "no other item");
}

void native() {
    chase::assets::Level levelData;
    chase::assets::LevelScript script;
    script.nameHash = chase::assets::nameHash("Use_AwardViewerScript");
    script.calls = {{"AwardViewer", {"Arg1"}}, {"ResetObject", {"Use_AwardViewer"}}};
    levelData.scripts.push_back(script);
    chase::assets::PlacedObjectState state;
    chase::runtime::ScriptRunner runner(levelData, state);
    require(!runner.takeAwardViewerRequest(), "no request before the script");
    require(runner.start("Use_AwardViewerScript"), "the script starts");
    runner.update(kDt);
    require(runner.takeAwardViewerRequest(), "AwardViewer asks the host to open the viewer");
    require(!runner.takeAwardViewerRequest(), "once");
}

AwardViewerSetup setup() {
    AwardViewerSetup s;
    s.shared.round.transitionOn = 0.5f;
    s.shared.round.transitionOnRound = 0.5f;
    s.shared.round.transitionOffRound = 0.5f;
    s.shared.round.leftMove = s.shared.round.rightMove = 0.5f;
    s.shared.round.loop = 0.2f;
    s.shared.centres = {{{54, 119}, {159, 119}, {264, 119}}};
    s.title = "Awards";
    for (unsigned i = 0; i < chase::runtime::kAwardViewerItems; ++i) s.items[i].text = "item " + std::to_string(i + 1);
    return s;
}

void run(AwardViewer& viewer, int updates, const AwardViewerInput& input = {}) {
    for (int i = 0; i < updates; ++i) viewer.update(kDt, input);
}

AwardViewerInput press(bool AwardViewerInput::*button) {
    AwardViewerInput input;
    input.*button = true;
    return input;
}

void screen() {
    Levels levels;
    for (LevelAwardData& data : levels) data = level(5, 0);
    levels[0] = level(5, 5);
    AwardViewer viewer(setup());
    viewer.open(levels);
    require(viewer.isOpen() && !viewer.pageShown(), "the page is entered by the page manager in an update");
    require(viewer.counts().goldTotal == 7, "the enter counts item 1 before the page");
    viewer.update(kDt, {});
    require(viewer.pageShown() && viewer.round().items().size() == 8 && viewer.round().selected() == 0, "eight items, item 1 selected");
    require(viewer.countTexts()[0] == "546/336" && viewer.countTexts()[1] == "4/28" && viewer.countTexts()[2] == "1/7", "General: " + viewer.countTexts()[1]);
    run(viewer, 20);
    require(viewer.round().state() == RoundMenuState::Active, "the round menu takes input");
    viewer.takeSounds();
    viewer.update(kDt, press(&AwardViewerInput::right));
    require(viewer.round().selected() == 1 && viewer.counts().goldTotal == 1 && viewer.counts().gold == 1, "Right shows the first level (item 2)");
    run(viewer, 20);
    viewer.update(kDt, press(&AwardViewerInput::a));
    require(viewer.takeSounds() == std::vector<int>{0x12}, "A plays no sound (0x304d5c), only the turn's 0x12 was played");
    viewer.update(kDt, press(&AwardViewerInput::right));
    run(viewer, 20);
    viewer.takeSounds();
    viewer.update(kDt, press(&AwardViewerInput::b));
    const std::vector<int> sounds = viewer.takeSounds();
    require(!sounds.empty() && sounds[0] == kAwardSoundBack, "B plays 0x11");
    require(viewer.isOpen(), "the round menu closes first");
    run(viewer, 13);
    require(viewer.isOpen(), "TransitionOff_Round runs 0.5 s");
    run(viewer, 3);
    require(viewer.finished(), "then the mode is popped");
    viewer.open(levels);
    viewer.update(kDt, {});
    require(viewer.round().selected() == 1, "the selection saved by A (+6) comes back, not the one B left");
}

}

int main() {
    try {
        oneLevel();
        general();
        native();
        screen();
    } catch (const std::exception& error) {
        std::cerr << "award-viewer-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "award-viewer-test: ok\n";
    return 0;
}
