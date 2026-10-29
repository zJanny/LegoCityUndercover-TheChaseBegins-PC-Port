#pragma once

#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/resources.hpp"
#include "frontend/round_menu.hpp"
#include "qol/game_tab.hpp"
#include "qol/menu_hit.hpp"
#include "qol/menu_model.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qol {

extern const char* const menuTitle;
extern const char* const masterLabel;

struct MenuPageSetup {
    chase::frontend::RoundMenuTimings round;
    std::array<std::array<float, 2>, 3> centres{};
    float shortcutOn = 0;
    std::string on, off, back, select, yes, no;
    std::array<std::string, gameRowCount> gameLabels;
    MenuGeometry geometry;
};

MenuPageSetup menuPageSetup(chase::frontend::Resources& resources, const std::string& gameTextFile,
    const std::string& systemTextFile);

std::string rowPlateText(const MenuModel& model, std::size_t row, const MenuPageSetup& setup, const GameOptions* game);
std::string tabPlaceText(const MenuModel& model);
std::string rowPicture(const MenuModel& model, std::size_t row);

chase::frontend::LayoutScene menuFrameScene(const MenuModel& model, const MenuPageSetup& setup);
chase::frontend::LayoutScene menuPageScene(const MenuModel& model, const MenuPageSetup& setup, const GameOptions* game = nullptr);
chase::frontend::LayoutScene menuShortcutsScene(const MenuPageSetup& setup);

struct ListForm {
    std::function<chase::frontend::LayoutScene(const MenuModel&)> scene;
    std::function<std::vector<HitArea>(const MenuModel&)> hits;
    std::function<std::optional<chase::frontend::LayoutScene>(const MenuModel&)> overlay;
};

ListForm controlsListForm(const MenuPageSetup& setup);
bool usesListForm(Tab tab);

chase::frontend::LayoutScene pageScene(const MenuModel& model, const MenuPageSetup& setup, const ListForm& list,
    const GameOptions* game = nullptr);
std::vector<HitArea> pageHitAreas(const MenuModel& model, const MenuPageSetup& setup, const ListForm& list);

void drawMenuPage(chase::frontend::Canvas& window, chase::frontend::Resources& resources, const MenuModel& model,
    const MenuPageSetup& setup, const ListForm& list, const GameOptions* game = nullptr);

}
