#include "qol/menu_stores.hpp"

#include "qol/active_settings.hpp"

#include <stdexcept>

namespace qol {

bool changesSettings(Result result) {
    switch (result) {
    case Result::MasterCycled:
    case Result::FeatureToggled:
    case Result::SwitchedToCustom:
    case Result::DisplayChanged:
        return true;
    default:
        return false;
    }
}

MenuStores::MenuStores(StoresPorts ports) : ports_(std::move(ports)), settings_(activeSettings()) {}

void MenuStores::open(MenuModel& model) {
    settings_ = activeSettings();
    if (!ports_.configured()) return;
    if (!model.setDisplayModes(ports_.modes, ports_.desktop))
        throw std::runtime_error("settings menu: the display modes or the desktop size are invalid");
    if (ports_.makeControls) {
        if (!controls_) controls_ = ports_.makeControls();
        model.setControls(controls_.get());
    }
}

void MenuStores::commit(Result result) {
    if (!ports_.configured()) return;
    if (changesSettings(result)) {
        saveSettings(ports_.settingsPath, settings_);
        setActiveSettings(settings_);
        if (result == Result::DisplayChanged && ports_.applyWindow) ports_.applyWindow(settings_);
    }
    if (result == Result::ControlsChanged && ports_.controlsChanged) ports_.controlsChanged();
}

}
