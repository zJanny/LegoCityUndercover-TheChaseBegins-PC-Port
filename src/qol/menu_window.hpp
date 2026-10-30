#pragma once

#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include "qol/game_tab.hpp"
#include "qol/menu_hit.hpp"
#include "qol/menu_model.hpp"
#include "qol/menu_page.hpp"
#include "qol/menu_redraw.hpp"

#include <optional>
#include <string>

namespace qol {

std::optional<Key> menuKeyForName(const std::string& keyName);
std::optional<Key> menuKeyForPad(const std::string& padName);

inline constexpr int leftMouseButton = 1;

class MenuWindow {
public:
    MenuWindow(MenuModel& model, MenuPageSetup setup, ListForm list);

    const MenuModel& model() const { return model_; }
    bool isOpen() const { return model_.isOpen(); }
    void open();
    void setGameOptions(const GameOptions* game) { game_ = game; }

    Result keyDown(const std::string& keyName);
    Result padDown(const std::string& padName);
    Result mouseDown(int button, int windowWidth, int windowHeight, float x, float y);
    Result mouseMoved(int windowWidth, int windowHeight, float x, float y);
    Result wheel(int steps);

    void draw(chase::frontend::Canvas& window, chase::frontend::Resources& resources) const;
    RedrawnPage page(int windowWidth, int windowHeight, chase::frontend::Resources& resources);
    const MenuRedraw& redraw() const { return redraw_; }

private:
    bool assigning() const;
    Result handle(const std::optional<Event>& event);
    std::optional<std::array<float, 2>> pageAt(int windowWidth, int windowHeight, float x, float y) const;

    MenuModel& model_;
    MenuPageSetup setup_;
    ListForm list_;
    const GameOptions* game_ = nullptr;
    MenuPointer pointer_;
    MenuRedraw redraw_;
};

}
