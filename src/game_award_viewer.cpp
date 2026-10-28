// LevelSession glue of the award viewer (frontend/award_viewer.hpp, docs/re/ui-screens.md). The use spot Use_AwardViewer of the police station
// triggers Use_AwardViewerScript, whose native AwardViewer pushes the award mode 0x514030; the level does not update while it is open, and the
// gameplay update's Start test does not run (the mode's update 0x2f26c8 has none), so the pause screen cannot open on top of it.
// STAND-IN UI-S12: a Start press of the update in which the native ran is dropped when the viewer opens (which push wins in the original is not read).
#include "game_hud.hpp"
#include "game_level.hpp"
#include "save/completion_tables.hpp"
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;
namespace fe = chase::frontend;
namespace save = chase::save;

namespace {
constexpr const char* kGameText = "localisation/gametext_uk.loc";
constexpr const char* kSystemText = "localisation/systemtext_uk.loc";
constexpr std::size_t kSlotDpadLeft = 2, kSlotDpadRight = 3;

std::array<runtime::LevelAwardData, runtime::kAwardLevels> levelAwards(const save::Bytes& slotBytes, const save::CompletionTotals& totals) {
    const save::SlotView slot{slotBytes.data(), slotBytes.size()};
    std::array<runtime::LevelAwardData, runtime::kAwardLevels> levels{};
    for (unsigned level = runtime::kAwardFirstLevel; level <= runtime::kAwardLastLevel; ++level) {
        runtime::LevelAwardData& data = levels[level - runtime::kAwardFirstLevel];
        for (unsigned category = 0; category < runtime::kAwardCategories; ++category) {
            data.collected[category] = save::categoryCounter(slot, category, level).value_or(0);
            data.totals[category] = totals.categoryTotals[category][level - save::kFirstCounterLevel];
        }
        data.missions = totals.levelTotals[level - save::kFirstCounterLevel];
        data.done = save::levelCollected(slot, level);
    }
    return levels;
}
}

bool LevelSession::holdForAwardViewer(float dt, const runtime::PadState& padInput) {
    if (awardViewer && awardViewer->isOpen()) {
        runtime::stickToDpad(pad, pauseStickDirection);
        fe::AwardViewerInput input;
        input.left = pad.slot(kSlotDpadLeft).press > 0;
        input.right = pad.slot(kSlotDpadRight).press > 0;
        input.a = pad.slot(kSlotConfirmButton).press > 0;
        input.b = pad.slot(kSlotCancelButton).press > 0;
        input.touch = {padInput.touch, float(padInput.touchX), float(padInput.touchY)};
        awardViewer->update(dt, input);
        game_save::playTick(levelScripts.progressNatives());
        for (const int id : awardViewer->takeSounds())
            if (shared.audio) shared.audio->playUi(unsigned(id));
        if (awardViewer->finished()) std::cout << "award viewer closed at update " << updates << '\n';
    } else if (levelScripts.takeAwardViewerRequest()) {
        pauseRequested = false;
        if (!awardViewer) awardViewer.emplace(fe::awardViewerSetup(hudResources, kGameText, kSystemText));
        const save::CompletionTotals totals = save::completionTotals(hudResources.archive());
        awardViewer->open(levelAwards(levelScripts.progressNatives().slotImage(), totals));
        const std::array<std::string, 3> counts = awardViewer->countTexts();
        std::cout << "award viewer opened at update " << updates << ", bronze " << counts[0] << ", silver " << counts[1] << ", gold " << counts[2] << '\n';
    } else {
        return false;
    }
    ++updates;
    ++updatesThisSecond;
    return true;
}

bool LevelSession::drawAwardViewer(int width, int height) {
    if (!awardViewer || !awardViewer->isOpen()) return false;
    const game_hud::Screens screens = game_hud::screensFor(width, height);
    chase::render::setScissorTest(false);
    chase::render::setViewport({0, 0, width, height});
    chase::render::clearWindow({0, 0, 0, 1}, true);
    awardViewer->drawTop(hudCanvas, hudResources);
    awardViewer->drawBottom(mapCanvas, hudResources);
    overlay->draw(game_hud::pixelsOf(hudCanvas), game_hud::pixelsOf(mapCanvas), screens.top, screens.bottom, width, height);
    return true;
}

}
