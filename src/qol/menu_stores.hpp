#pragma once

#include "qol/controls_port.hpp"
#include "qol/display_tab.hpp"
#include "qol/game_port.hpp"
#include "qol/settings.hpp"

#include <functional>
#include <memory>
#include <optional>
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
    bool quitTab = false;

    bool configured() const { return !settingsPath.empty(); }
};

struct MenuEffects {
    bool settingsChanged = false;
    bool windowChanged = false;
    bool controlsChanged = false;
    bool gameChanged = false;
    bool closed = false;
    bool quitGame = false;
    std::optional<int> language;

    void merge(const MenuEffects& other);
};

class MenuStores {
public:
    explicit MenuStores(StoresPorts ports);

    Settings& settings() { return settings_; }
    const Settings& settings() const { return settings_; }
    GamePort* game() const { return game_.get(); }
    ControlsPort* controls() const { return controls_.get(); }
    bool hasDisplay() const { return ports_.configured(); }
    bool quitTab() const { return ports_.quitTab; }
    const ResolutionChoices& modes() const { return ports_.modes; }
    Resolution desktop() const { return ports_.desktop; }
    int startLanguage() const;

    void open();
    void commit(const MenuEffects& effects);

private:
    StoresPorts ports_;
    Settings settings_;
    std::unique_ptr<ControlsPort> controls_;
    std::unique_ptr<GamePort> game_;
};

}
