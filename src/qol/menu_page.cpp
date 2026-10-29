#include "qol/menu_page.hpp"

#include "frontend/pages.hpp"
#include "frontend/pause_menu.hpp"
#include "qol/menu_list_page.hpp"
#include "runtime/options_page.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace qol {

namespace fe = chase::frontend;

const char* const menuTitle = "PC Settings";
const char* const masterLabel = "Quality of life";

namespace {

constexpr const char* optionsPicture = "UI_StatusScreen_Options.png";
constexpr const char* pageNumberElement = "PageNumber";
constexpr const char* pageSlotsElement = "Pageslots";
constexpr const char* roundElements[] = {"Menu", "Menu_List", "Item_Name"};

Box elementBox(fe::Resources& resources, const std::string& layout, const std::string& path, float x = 0, float y = 0) {
    const auto box = fe::layoutElementBox(resources, layout, path);
    if (!box) throw std::runtime_error("settings menu: layout " + layout + " has no element " + path);
    return {(*box)[0] + x, (*box)[1] + y, (*box)[2], (*box)[3]};
}

std::string gamePicture(std::size_t row) {
    const std::string folder = chase::runtime::kOptionsItemFolder, parent = fe::kPauseItemFolder;
    return folder.substr(parent.size() + 1) + "/" + chase::runtime::kOptionsItemImages[row];
}

std::string plate(const std::string& label, const std::string& value) {
    return label + ": " + value;
}

fe::LayoutScene roundScene(const MenuModel& model, const MenuPageSetup& setup, const std::vector<fe::RoundMenuItem>& items) {
    fe::RoundMenu round(setup.round, setup.centres);
    round.open(fe::kPauseItemFolder, items, int(model.row()));
    round.setTitle(menuTitle);
    round.setSubtitle(tabName(model.tab()));
    round.update(0, {});
    round.update(setup.round.transitionOnRound, {});
    fe::LayoutScene scene = round.scene();
    scene.opacities[pageSlotsElement] = 1;
    fe::TextSlot number;
    number.text = tabPlaceText(model);
    number.align = fe::Align::Centre;
    scene.text[pageNumberElement] = number;
    return scene;
}

}

MenuPageSetup menuPageSetup(fe::Resources& resources, const std::string& gameTextFile, const std::string& systemTextFile) {
    const fe::PauseMenuSetup pause = fe::pauseMenuSetup(resources, gameTextFile, systemTextFile);
    MenuPageSetup setup;
    setup.round = pause.round;
    setup.centres = pause.centres;
    setup.shortcutOn = pause.shortcutOn;
    setup.back = pause.texts.back;
    setup.select = pause.texts.select;
    setup.yes = pause.texts.options.yes;
    setup.no = pause.texts.options.no;
    const chase::runtime::OptionsTexts& options = pause.texts.options;
    setup.gameLabels = {options.sound, options.music, options.invertCamera, options.language};
    setup.on = resources.text(gameTextFile, "ON");
    setup.off = resources.text(gameTextFile, "OFF");
    setup.geometry.slotCentres = pause.centres;
    setup.geometry.slotRadius = fe::kRoundMenuTouchRadius;
    setup.geometry.back = elementBox(resources, fe::kShortcutsLayout, "BackButton_3DS/Button", fe::kShortcutsX, fe::kShortcutsY);
    setup.geometry.select = elementBox(resources, fe::kShortcutsLayout, "SelectButton_3DS/Button", fe::kShortcutsX, fe::kShortcutsY);
    setup.geometry.previousTab = elementBox(resources, fe::kRoundMenuLayout, "Left_Arrow");
    setup.geometry.nextTab = elementBox(resources, fe::kRoundMenuLayout, "Right_Arrow");
    setup.geometry.header = elementBox(resources, fe::kRoundMenuLayout, "Subtitle");
    return setup;
}

std::string rowPlateText(const MenuModel& model, std::size_t row, const MenuPageSetup& setup, const GameOptions* game) {
    const std::vector<Row> rows = model.rows();
    if (row >= rows.size()) return std::string();
    const Settings& settings = model.settings();
    if (model.tab() == Tab::Game && row < gameRowCount) {
        const GameRow gameRow = static_cast<GameRow>(row);
        const std::string& label = setup.gameLabels[row];
        if (!game || gameRow == GameRow::Language) return label;
        if (gameRow == GameRow::InvertCamera) return plate(label, game->invertCamera() ? setup.on : setup.off);
        return plate(label, std::to_string(gameRow == GameRow::SfxVolume ? game->sfxVolume() : game->musicVolume()));
    }
    switch (rows[row].kind) {
    case RowKind::Master: return plate(masterLabel, masterName(settings.master()));
    case RowKind::Feature:
        return plate(featureTable[static_cast<std::size_t>(rows[row].feature)].label,
            settings.enabled(rows[row].feature) ? setup.on : setup.off);
    case RowKind::Display: {
        const std::vector<DisplayRow> display = model.displayRows();
        if (row < display.size()) return plate(display[row].label, display[row].value);
        return rows[row].label;
    }
    case RowKind::Control:
    case RowKind::ResetAll:
    case RowKind::Placeholder: return rows[row].label;
    }
    return rows[row].label;
}

std::string tabPlaceText(const MenuModel& model) {
    return std::to_string(static_cast<std::size_t>(model.tab()) + 1) + "/" + std::to_string(tabCount);
}

std::string rowPicture(const MenuModel& model, std::size_t row) {
    if (model.tab() == Tab::Game && row < gameRowCount) return gamePicture(row);
    return optionsPicture;
}

fe::LayoutScene menuFrameScene(const MenuModel& model, const MenuPageSetup& setup) {
    fe::LayoutScene scene = roundScene(model, setup, {});
    for (const char* element : roundElements) scene.hidden.insert(element);
    return scene;
}

fe::LayoutScene menuPageScene(const MenuModel& model, const MenuPageSetup& setup, const GameOptions* game) {
    std::vector<fe::RoundMenuItem> items;
    const std::size_t count = model.rows().size();
    for (std::size_t row = 0; row < count; ++row) items.push_back({rowPlateText(model, row, setup, game), rowPicture(model, row)});
    return roundScene(model, setup, items);
}

fe::LayoutScene menuShortcutsScene(const MenuPageSetup& setup) {
    fe::ShortcutView back{setup.back, "TransitionOn", setup.shortcutOn, true};
    fe::ShortcutView select{setup.select, "TransitionOn", setup.shortcutOn, true};
    return fe::backSelectShortcutsScene(back, select);
}

ListForm controlsListForm(const MenuPageSetup& setup) {
    return {[](const MenuModel& model) { return listPageScene(model); },
            [](const MenuModel& model) { return listHitAreas(model); },
            [setup](const MenuModel& model) -> std::optional<fe::LayoutScene> {
                if (controlsQuestionShown(model)) return questionScene(model, setup.yes, setup.no);
                if (controlsNoticeShown(model)) return noticeScene(model);
                return std::nullopt;
            }};
}

bool usesListForm(Tab tab) {
    return tab == Tab::Controls;
}

fe::LayoutScene pageScene(const MenuModel& model, const MenuPageSetup& setup, const ListForm& list, const GameOptions* game) {
    if (usesListForm(model.tab()) && list.scene) return list.scene(model);
    return menuPageScene(model, setup, game);
}

std::vector<HitArea> pageHitAreas(const MenuModel& model, const MenuPageSetup& setup, const ListForm& list) {
    if (usesListForm(model.tab()) && list.hits) return list.hits(model);
    return roundHitAreas(model, setup.geometry);
}

void drawMenuPage(fe::Canvas& window, fe::Resources& resources, const MenuModel& model, const MenuPageSetup& setup,
    const ListForm& list, const GameOptions* game) {
    window.clear(0, 0, 0);
    const PageRect rect = menuPageRect(window.width(), window.height());
    const int width = int(std::lround(rect.width)), height = int(std::lround(rect.height));
    if (width <= 0 || height <= 0) return;
    fe::Canvas page(width, height);
    page.clear(0, 0, 0);
    fe::LayoutScene scene = pageScene(model, setup, list, game);
    scene.scaleText = true;
    fe::drawLayout(page, resources, fe::kRoundMenuLayout, scene, 0, 0, rect.scale);
    fe::LayoutScene shortcuts = menuShortcutsScene(setup);
    shortcuts.scaleText = true;
    fe::drawLayout(page, resources, fe::kShortcutsLayout, shortcuts, fe::kShortcutsX * rect.scale, fe::kShortcutsY * rect.scale, rect.scale);
    if (usesListForm(model.tab()) && list.overlay) {
        if (std::optional<fe::LayoutScene> overlay = list.overlay(model)) {
            overlay->scaleText = true;
            fe::drawLayout(page, resources, noticePageLayout, *overlay, 0, 0, rect.scale);
        }
    }
    window.blit(page, int(std::lround(rect.x)), int(std::lround(rect.y)));
}

}
