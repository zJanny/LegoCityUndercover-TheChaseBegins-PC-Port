// LevelSession glue of the pause screen (frontend/pause_menu.hpp, docs/re/ui-screens.md). Start in a gameplay update pauses the game
// (0x2e5158 -> 0x2e6a60(gm, 0)); the next update opens the pause screen (0x188500 pushes mode 0x527358) instead of running the game, and
// the game goes on in the update after the screen has closed (0x2e6a60(gm, 1)).
// STAND-IN UI-S7: the gates of the Start test (0x25207c, 0x171364, 0x22acc4, game state 0x377508 != 3, gm+0x234) are replaced by
// "no cutscene plays" (the port's other holds return earlier); the mode fades (0x25d26c 0 / 0.5 s, 0x22b6d4 0.5 / 0.5 s), the paused sounds
// and animation slots of 0x2e6a60 are not reproduced.
#include "game_hud.hpp"
#include "game_level.hpp"
#include "frontend/script_text_pages.hpp"
#include "qol/pause_entry.hpp"
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

namespace {
constexpr const char* kGameText = "localisation/gametext_uk.loc";
constexpr const char* kHintText = "localisation/hinttext_uk.loc";
constexpr const char* kSystemText = "localisation/systemtext_uk.loc";
constexpr std::size_t kSlotStart = 10, kSlotDpadUp = 0, kSlotDpadDown = 1, kSlotDpadLeft = 2, kSlotDpadRight = 3;
const char* const kEntryNames[] = {"Resume", "Options", "Credits", "Exit"};
}

bool LevelSession::holdForPause(float dt, const runtime::PadState& padInput) {
    if (pauseMenu && pauseMenu->isOpen()) {
        if (qol::pauseEntryHolds(pauseSettings, *pauseMenu)) return true;
        runtime::stickToDpad(pad, pauseStickDirection);
        fe::PauseInput input;
        input.start = pad.slot(kSlotStart).press > 0;
        input.left = pad.slot(kSlotDpadLeft).press > 0;
        input.right = pad.slot(kSlotDpadRight).press > 0;
        input.up = pad.slot(kSlotDpadUp).press > 0;
        input.down = pad.slot(kSlotDpadDown).press > 0;
        input.a = pad.slot(kSlotConfirmButton).press > 0;
        input.b = pad.slot(kSlotCancelButton).press > 0;
        input.touch = {padInput.touch, float(padInput.touchX), float(padInput.touchY)};
        pauseMenu->update(dt, input);
        game_save::playTick(levelScripts.progressNatives());
        for (const int id : pauseMenu->takeSounds())
            if (shared.audio) shared.audio->playUi(unsigned(id));
        for (const fe::PauseEntry entry : pauseMenu->takeUnported())
            std::cout << "pause menu: " << kEntryNames[int(entry)] << " is not ported (does nothing) at update " << updates << '\n';
        const fe::PauseGains gains = pauseMenu->takeGains();
        if (shared.audio && (gains.sound || gains.music)) shared.audio->setOptionGains(gains.sound, gains.music);
        if (const std::optional<int> language = pauseMenu->takeLanguageChange()) {
            std::cout << "pause menu: language " << *language << " chosen (the texts and pictures of the other languages are not loaded) at update " << updates << '\n';
        }
        if (pauseMenu->takeSaveRequest()) {
            const std::uint8_t* block = pauseMenu->options();
            const std::size_t slot = runtime::optionsSlot(block).value_or(0);
            const bool stored = shared.progress.writer.storeOptions(block);
            std::cout << "pause menu: options saved" << (stored ? " to " + shared.progress.writer.path() : std::string(" (writing disabled)"))
                      << ", slot " << slot << " sound volume " << unsigned(block[4 + slot]) << ", music volume " << unsigned(block[6 + slot])
                      << ", invert camera " << ((block[3] & runtime::kInvertCameraBit) ? "on" : "off") << " at update " << updates << '\n';
            pauseMenu->saveDone();
        }
        if (pauseMenu->finished()) {
            const chase::save::Bytes& slot = levelScripts.progressNatives().slotImage();
            std::cout << "pause menu closed at update " << updates << ", play ticks " << chase::save::playTicks({slot.data(), slot.size()}) << '\n';
        }
    } else if (pauseRequested) {
        pauseRequested = false;
        if (!pauseMenu) pauseMenu.emplace(qol::withPauseEntry(fe::pauseMenuSetup(hudResources, kGameText, kSystemText), pauseSettings, hudResources, shared.qolStores));
        if (shared.progress.options.size() == chase::save::kOptionsSize)
            pauseMenu->bindOptions(shared.progress.options.data(), &shared.progress.volumes, int(shared.options.language));
        pauseMenu->open();
        const runtime::ScriptTextState& missionText = levelScripts.text();
        pauseMenu->setTickerText(runtime::pauseTickerText(hudResources.textForKey(kGameText, fe::levelHeadingKey(hudResources, levelName)),
                                                          missionText.missionTitle(), hudResources.textForKey(kHintText, missionText.missionTitle()),
                                                          missionText.missionSummary(), hudResources.textForKey(kHintText, missionText.missionSummary())));
        std::cout << "pause menu opened at update " << updates << '\n';
    } else {
        if (pad.slot(kSlotStart).press > 0 && !cutscenePlaying()) pauseRequested = true;
        return false;
    }
    ++updates;
    ++updatesThisSecond;
    return true;
}

bool LevelSession::pauseSettingsConsume(const SDL_Event& event, SDL_Window* window) {
    return qol::pauseEntryConsumes(pauseSettings, event, window);
}

bool LevelSession::drawPause(int width, int height) {
    if (!pauseMenu || !pauseMenu->isOpen()) return false;
    if (qol::pauseEntryPresented(pauseSettings, width, height)) return true;
    const game_hud::Screens screens = game_hud::screensFor(width, height);
    chase::render::setScissorTest(false);
    chase::render::setViewport({0, 0, width, height});
    chase::render::clearWindow({0, 0, 0, 1}, true);
    pauseMenu->drawTop(hudCanvas, hudResources);
    pauseMenu->drawBottom(mapCanvas, hudResources);
    overlay->draw(game_hud::pixelsOf(hudCanvas), game_hud::pixelsOf(mapCanvas), screens.top, screens.bottom, width, height);
    return true;
}

}
