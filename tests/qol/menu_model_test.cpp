// Spec: docs/design/qol-settings.md entries 2026-10-28T09:32:07 (tabs, input),
// 09:37:29 (master cycles Original, Improved, Custom; tabs always active),
// 09:40:40 (Custom skipped while no mix), 09:53:33 (switches apply only in Custom).
// Cases written by 20261028-qolmenu-A on 2026-10-28.
// Answers of the user, 2026-10-28 (docs/design/qol-settings.md, QOLSET-16 and QOLSET-17): feature rows
// outside Custom copy the mode's switches into an own mix and switch to Custom; rows wrap; Left/Right ignored.
#include "qol/menu_model.hpp"

#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

std::string written(const qol::Settings& settings) {
    std::ostringstream out;
    qol::writeSettings(out, settings);
    return out.str();
}

using qol::Event;
using qol::EventKind;
using qol::Result;
using qol::Tab;

void goToQualityTab(qol::MenuModel& menu) {
    menu.handle(qol::tabClick(static_cast<std::size_t>(Tab::QualityOfLife)));
}

void originalModeFirst() {
    qol::Settings settings;
    const std::string before = written(settings);
    qol::MenuModel menu(settings);
    require(!menu.isOpen(), "menu starts closed");
    require(menu.handle(qol::eventForKey(qol::Key::Enter)) == Result::Ignored, "closed menu ignores events");
    menu.open();
    require(menu.isOpen(), "open");
    require(menu.tab() == Tab::Game, "opens on Game");
    require(settings.master() == qol::Master::Original, "fresh store is Original");
    require(!settings.enabled(qol::Feature::WidenedView), "fresh store has the feature off");
    menu.handle(qol::eventForKey(qol::Key::Escape));
    require(!menu.isOpen(), "back closes");
    require(written(settings) == before, "opening and closing changes nothing");
    require(!settings.mixSet(), "no mix after opening and closing");

    menu.open();
    for (qol::Key key : {qol::Key::Down, qol::Key::Enter, qol::Key::Right, qol::Key::PageDown,
                         qol::Key::PageUp, qol::Key::Up, qol::Key::Enter})
        menu.handle(qol::eventForKey(key));
    menu.handle(qol::rowClick(1));
    menu.handle(qol::tabClick(2));
    menu.handle(qol::rowClick(0));
    require(written(settings) == before, "the placeholder tabs change no setting");
}

void structure() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    for (std::size_t i = 0; i < qol::tabCount; ++i)
        require(menu.tabActive(static_cast<Tab>(i)), "all tabs active");
    require(std::string(qol::tabName(Tab::Game)) == "Game", "tab Game");
    require(std::string(qol::tabName(Tab::Display)) == "Display", "tab Display");
    require(std::string(qol::tabName(Tab::Controls)) == "Controls", "tab Controls");
    require(std::string(qol::tabName(Tab::QualityOfLife)) == "Quality of life", "tab Quality of life");

    auto game = qol::rowsOf(Tab::Game);
    require(game.size() == 4 && game[0].label == "SFX volume" && game[1].label == "Music volume" &&
                game[2].label == "Invert camera" && game[3].label == "Language",
            "Game rows");
    auto display = qol::rowsOf(Tab::Display);
    require(display.size() == 2 && display[0].label == "Window mode" && display[1].label == "Resolution" &&
                display[0].kind == qol::RowKind::Display && display[1].kind == qol::RowKind::Display,
            "Display rows");
    require(qol::rowsOf(Tab::Controls).size() == 1 && qol::rowsOf(Tab::Controls)[0].label == "Key bindings",
            "Controls rows");
    for (Tab tab : {Tab::Game, Tab::Controls})
        for (const qol::Row& row : qol::rowsOf(tab))
            require(row.kind == qol::RowKind::Placeholder, "placeholder rows");

    auto quality = qol::rowsOf(Tab::QualityOfLife);
    require(quality.size() == 1 + qol::featureCount, "master plus one row per feature");
    require(quality[0].kind == qol::RowKind::Master, "first row is the master button");
    for (std::size_t i = 0; i < qol::featureCount; ++i) {
        require(quality[i + 1].kind == qol::RowKind::Feature, "feature row kind");
        require(quality[i + 1].feature == qol::featureTable[i].feature, "feature row feature");
        require(quality[i + 1].label == qol::featureTable[i].name, "feature row label");
    }
}

void eventMapping() {
    struct Case { qol::Key key; EventKind kind; };
    const Case cases[] = {
        {qol::Key::Up, EventKind::Up}, {qol::Key::PadUp, EventKind::Up},
        {qol::Key::Down, EventKind::Down}, {qol::Key::PadDown, EventKind::Down},
        {qol::Key::Left, EventKind::Left}, {qol::Key::PadLeft, EventKind::Left},
        {qol::Key::Right, EventKind::Right}, {qol::Key::PadRight, EventKind::Right},
        {qol::Key::Enter, EventKind::Activate}, {qol::Key::PadA, EventKind::Activate},
        {qol::Key::Escape, EventKind::Back}, {qol::Key::PadB, EventKind::Back},
        {qol::Key::PageUp, EventKind::TabPrevious}, {qol::Key::PadL, EventKind::TabPrevious},
        {qol::Key::PageDown, EventKind::TabNext}, {qol::Key::PadR, EventKind::TabNext},
    };
    for (const Case& c : cases)
        require(qol::eventForKey(c.key).kind == c.kind, "key maps to its event");
    require(qol::rowMove(3).kind == EventKind::RowMove && qol::rowMove(3).index == 3, "row move");
    require(qol::rowClick(2).kind == EventKind::RowClick && qol::rowClick(2).index == 2, "row click");
    require(qol::tabMove(1).kind == EventKind::TabMove && qol::tabMove(1).index == 1, "tab move");
    require(qol::tabClick(1).kind == EventKind::TabClick && qol::tabClick(1).index == 1, "tab click");
}

void navigation() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    require(menu.handle({EventKind::Up}) == Result::RowChanged && menu.row() == 3, "wraps from the first to the last");
    require(menu.handle({EventKind::Down}) == Result::RowChanged && menu.row() == 0, "wraps from the last to the first");
    require(menu.handle({EventKind::Down}) == Result::RowChanged && menu.row() == 1, "down");
    menu.handle({EventKind::Down});
    menu.handle({EventKind::Down});
    require(menu.row() == 3, "reaches the last Game row");
    require(menu.handle({EventKind::Down}) == Result::RowChanged && menu.row() == 0, "wraps at the bottom");
    menu.handle({EventKind::Up});
    require(menu.handle({EventKind::Up}) == Result::RowChanged && menu.row() == 2, "up");

    require(menu.handle({EventKind::TabNext}) == Result::TabChanged && menu.tab() == Tab::Display, "tab next");
    require(menu.row() == 0, "other tab has its own row");
    menu.handle({EventKind::TabPrevious});
    require(menu.tab() == Tab::Game && menu.row() == 2, "row remembered per tab");
    require(menu.handle({EventKind::TabPrevious}) == Result::TabChanged && menu.tab() == Tab::QualityOfLife,
            "tab wraps from the first to the last");
    menu.handle({EventKind::TabNext});
    require(menu.tab() == Tab::Game, "tab wraps from the last to the first");
    menu.handle({EventKind::TabNext});
    menu.handle({EventKind::TabNext});
    menu.handle({EventKind::TabNext});
    require(menu.tab() == Tab::QualityOfLife, "reaches the last tab");
    require(menu.handle({EventKind::TabNext}) == Result::TabChanged && menu.tab() == Tab::Game,
            "tab next wraps at the last");
    menu.handle({EventKind::TabPrevious});
    menu.handle({EventKind::TabPrevious});
    menu.handle({EventKind::TabPrevious});
    menu.handle({EventKind::TabPrevious});
    require(menu.tab() == Tab::Game, "four tab steps back return to the start");
    for (int i = 0; i < 3; ++i) menu.handle({EventKind::TabNext});

    require(menu.handle({EventKind::Left}) == Result::Ignored, "left is ignored");
    require(menu.handle({EventKind::Right}) == Result::Ignored, "right is ignored");

    require(menu.handle(qol::tabClick(0)) == Result::TabChanged && menu.tab() == Tab::Game, "tab click");
    require(menu.handle(qol::tabClick(0)) == Result::Ignored, "click on the current tab");
    require(menu.handle(qol::tabMove(2)) == Result::Ignored && menu.tab() == Tab::Game, "tab hover changes nothing");
    require(menu.handle(qol::rowMove(1)) == Result::RowChanged && menu.row() == 1, "row hover selects");
    require(menu.handle(qol::rowMove(1)) == Result::Ignored, "hover on the selected row");
    require(menu.handle(qol::rowClick(3)) == Result::RowChanged && menu.row() == 3,
            "click on a placeholder row selects it");
}

void masterCycling() {
    qol::Settings withoutMix;
    qol::MenuModel menu(withoutMix);
    menu.open();
    goToQualityTab(menu);
    require(menu.row() == 0 && menu.rows()[0].kind == qol::RowKind::Master, "master row selected first");
    require(menu.handle({EventKind::Activate}) == Result::MasterCycled &&
                withoutMix.master() == qol::Master::Improved, "Original to Improved");
    require(menu.handle({EventKind::Activate}) == Result::MasterCycled &&
                withoutMix.master() == qol::Master::Original, "Improved skips Custom without a mix");
    require(!withoutMix.mixSet(), "cycling sets no mix");

    qol::Settings withMix;
    withMix.setSwitch(qol::Feature::WidenedView, true);
    qol::MenuModel second(withMix);
    second.open();
    goToQualityTab(second);
    second.handle({EventKind::Activate});
    second.handle({EventKind::Activate});
    require(withMix.master() == qol::Master::Custom, "Improved to Custom with a mix");
    require(withMix.enabled(qol::Feature::WidenedView), "Custom restores the mix");
    second.handle({EventKind::Activate});
    require(withMix.master() == qol::Master::Original, "Custom to Original");

    qol::Settings viaClick;
    qol::MenuModel third(viaClick);
    third.open();
    goToQualityTab(third);
    require(third.handle(qol::rowClick(0)) == Result::MasterCycled && viaClick.master() == qol::Master::Improved,
            "click on the master row cycles");

    qol::Settings viaApi;
    qol::MenuModel fourth(viaApi);
    fourth.open();
    goToQualityTab(fourth);
    fourth.handle({EventKind::Activate});
    qol::Settings expected;
    expected.cycleMaster();
    require(viaApi.master() == expected.master(), "same result as the settings API");
}

void toggles() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    goToQualityTab(menu);
    menu.handle({EventKind::Down});
    require(menu.rows()[menu.row()].kind == qol::RowKind::Feature, "feature row selected");

    require(menu.handle({EventKind::Activate}) == Result::SwitchedToCustom, "Original: switches to Custom");
    require(settings.master() == qol::Master::Custom && settings.mixSet(), "Original: Custom with a mix");
    require(settings.customSwitch(qol::Feature::WidenedView) && settings.enabled(qol::Feature::WidenedView),
            "Original: the clicked switch is on (copied all off, then toggled)");
    require(menu.handle({EventKind::Activate}) == Result::FeatureToggled, "Custom: toggled");
    require(!settings.customSwitch(qol::Feature::WidenedView) && settings.master() == qol::Master::Custom,
            "Custom: off again, stays Custom");

    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    qol::MenuModel improvedMenu(improved);
    improvedMenu.open();
    goToQualityTab(improvedMenu);
    require(improved.enabled(qol::Feature::WidenedView), "Improved: feature on");
    require(improvedMenu.handle(qol::rowClick(1)) == Result::SwitchedToCustom, "Improved: click switches to Custom");
    require(improved.master() == qol::Master::Custom && improved.mixSet(), "Improved: Custom with a mix");
    require(!improved.customSwitch(qol::Feature::WidenedView) && !improved.enabled(qol::Feature::WidenedView),
            "Improved: mix copied all on, the click turned the feature off");

    qol::Settings stale;
    stale.setSwitch(qol::Feature::WidenedView, true);
    qol::MenuModel staleMenu(stale);
    staleMenu.open();
    goToQualityTab(staleMenu);
    require(!stale.enabled(qol::Feature::WidenedView), "Original ignores an old mix");
    staleMenu.handle(qol::rowClick(1));
    require(stale.master() == qol::Master::Custom && stale.customSwitch(qol::Feature::WidenedView),
            "an old mix is replaced by the current mode's switches, then the click applies");
}

void displayTab() {
    qol::Settings settings;
    const std::string before = written(settings);
    qol::MenuModel menu(settings);
    menu.open();
    menu.handle(qol::tabClick(static_cast<std::size_t>(Tab::Display)));
    require(menu.tab() == Tab::Display && menu.row() == 0, "display tab selected");
    require(settings.master() == qol::Master::Original, "original mode while using the display tab");
    require(menu.handle({EventKind::Up}) == Result::RowChanged && menu.row() == 1, "display rows wrap up");
    require(menu.handle({EventKind::Down}) == Result::RowChanged && menu.row() == 0, "display rows wrap down");
    require(menu.handle({EventKind::Left}) == Result::Ignored && menu.handle({EventKind::Right}) == Result::Ignored,
            "left and right ignored on display rows");
    require(menu.handle(qol::rowMove(2)) == Result::Rejected, "hover past the last display row");
    require(menu.handle(qol::rowClick(2)) == Result::Rejected, "click past the last display row");
    require(written(settings) == before, "navigation changes no setting");

    require(menu.handle(qol::eventForKey(qol::Key::Enter)) == Result::DisplayChanged &&
                settings.windowMode() == qol::WindowMode::Windowed,
            "Enter toggles the window mode");
    require(menu.displayRows()[0].value == "windowed", "row value follows");
    require(menu.handle(qol::rowClick(1)) == Result::RowChanged && !settings.resolution(),
            "resolution row without modes is ignored");

    const qol::ResolutionChoices modes{{800, 600}, {1280, 720}, {1920, 1080}};
    require(!menu.setDisplayModes({{399, 240}}, {1920, 1080}), "mode below the minimum rejected");
    require(!menu.setDisplayModes(modes, {300, 200}), "tiny desktop rejected");
    require(menu.setDisplayModes(modes, {1280, 720}), "modes accepted");
    require(menu.handle(qol::rowClick(1)) == Result::DisplayChanged &&
                settings.resolution() == std::optional<qol::Resolution>{{1920, 1080}},
            "the unresolved resolution steps on from the desktop size");
    require(menu.handle(qol::rowClick(1)) == Result::DisplayChanged &&
                settings.resolution() == std::optional<qol::Resolution>{{800, 600}},
            "click steps and wraps");
    require(menu.displayRows()[1].value == "800x600" && !menu.displayRows()[1].custom, "listed value");
    require(settings.master() == qol::Master::Original && !settings.mixSet(), "display leaves master and mix alone");
}

void rejections() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    const std::string before = written(settings);
    struct Case { Event event; const char* what; };
    const Case cases[] = {
        {qol::rowMove(4), "row hover past the last Game row"},
        {qol::rowClick(4), "row click past the last Game row"},
        {qol::rowMove(static_cast<std::size_t>(-1)), "row hover with a negative index"},
        {qol::rowClick(static_cast<std::size_t>(-1)), "row click with a negative index"},
        {qol::tabMove(qol::tabCount), "tab hover past the last tab"},
        {qol::tabClick(qol::tabCount), "tab click past the last tab"},
        {qol::tabClick(static_cast<std::size_t>(-1)), "tab click with a negative index"},
    };
    for (const Case& c : cases) {
        require(menu.handle(c.event) == Result::Rejected, std::string("rejects ") + c.what);
        require(menu.tab() == Tab::Game && menu.row() == 0, std::string("state unchanged after ") + c.what);
    }
    goToQualityTab(menu);
    require(menu.handle(qol::rowClick(1 + qol::featureCount)) == Result::Rejected,
            "row click past the last Quality of life row");
    require(menu.row() == 0, "row unchanged after a rejected click");
    require(written(settings) == before, "rejections change no setting");
}

}

int main() {
    originalModeFirst();
    structure();
    eventMapping();
    navigation();
    masterCycling();
    toggles();
    displayTab();
    rejections();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-menu-model: all checks passed\n";
    return 0;
}
