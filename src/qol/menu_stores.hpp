#pragma once

#include "qol/controls_port.hpp"
#include "qol/game_port.hpp"
#include "qol/menu_model.hpp"
#include "qol/settings.hpp"

#include <functional>
#include <memory>
#include <string>

namespace qol {

struct StoresPorts {
    std::string settingsPath;
    ResolutionChoices modes;
    Resolution desktop{0, 0};
    std::function<void(const Settings&)> applyWindow;
    std::function<std::unique_ptr<ControlsPort>()> makeControls;
    std::function<void()> controlsChanged;
    std::function<std::unique_ptr<GamePort>()> makeGame;

    bool configured() const { return !settingsPath.empty(); }
};

bool changesSettings(Result result);

class MenuStores {
public:
    explicit MenuStores(StoresPorts ports);

    Settings& settings() { return settings_; }
    const Settings& settings() const { return settings_; }
    const GameOptions* game() const { return game_.get(); }
    void open(MenuModel& model);
    void commit(Result result);

private:
    StoresPorts ports_;
    Settings settings_;
    std::unique_ptr<ControlsPort> controls_;
    std::unique_ptr<GamePort> game_;
};

}
