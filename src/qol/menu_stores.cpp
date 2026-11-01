#include "qol/menu_stores.hpp"

#include "qol/active_settings.hpp"

#include <stdexcept>

namespace qol {

void MenuEffects::merge(const MenuEffects& other) {
    settingsChanged = settingsChanged || other.settingsChanged;
    windowChanged = windowChanged || other.windowChanged;
    controlsChanged = controlsChanged || other.controlsChanged;
    gameChanged = gameChanged || other.gameChanged;
    closed = closed || other.closed;
    if (other.language) language = other.language;
}

MenuStores::MenuStores(StoresPorts ports) : ports_(std::move(ports)), settings_(activeSettings()) {}

int MenuStores::startLanguage() const {
    return game_ ? game_->startLanguage() : firstLanguage;
}

void MenuStores::open() {
    settings_ = activeSettings();
    if (!ports_.configured()) return;
    if (!choicesValid(ports_.modes) || !resolutionValid(ports_.desktop))
        throw std::runtime_error("settings menu: the display modes or the desktop size are invalid");
    if (ports_.makeControls && !controls_) controls_ = ports_.makeControls();
    if (ports_.makeGame && !game_) game_ = ports_.makeGame();
}

void MenuStores::commit(const MenuEffects& effects) {
    if (!ports_.configured()) return;
    bool pcControlsSwitched = false;
    if (effects.settingsChanged) {
        pcControlsSwitched = settings_.enabled(Feature::PcControls) != activeSettings().enabled(Feature::PcControls);
        saveSettings(ports_.settingsPath, settings_);
        setActiveSettings(settings_);
    }
    if (pcControlsSwitched && controls_) controls_->reload();
    if (pcControlsSwitched && ports_.controlsChanged) ports_.controlsChanged();
    if (effects.windowChanged && ports_.applyWindow) ports_.applyWindow(settings_);
    if (effects.controlsChanged && ports_.controlsChanged) ports_.controlsChanged();
    if (!game_) return;
    if (effects.language && *effects.language != game_->language()) game_->openLanguages();
    if (effects.gameChanged || effects.language) game_->applied();
    if (effects.closed) game_->left();
}

}
