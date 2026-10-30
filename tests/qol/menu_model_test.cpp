// Spec: docs/design/qol-settings.md entries 2026-10-28T09:32:07 (tabs, input),
// 09:37:29 (master cycles Original, Improved, Custom; tabs always active),
// 09:40:40 (Custom skipped while no mix), 09:53:33 (switches apply only in Custom).
// Cases written by 20261028-qolmenu-A on 2026-10-28.
// Answers of the user, 2026-10-28 (docs/design/qol-settings.md, QOLSET-16 and QOLSET-17): feature rows
// outside Custom copy the mode's switches into an own mix and switch to Custom; rows wrap; Left/Right ignored.
#include "qol/controls_file.hpp"
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
    menu.handle(qol::slotClick(1, 1));
    require(menu.controlsRows().empty() && !menu.controlsWaiting(), "Controls rows without bindings do nothing");
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
    auto controls = qol::rowsOf(Tab::Controls);
    require(controls.size() == qol::controlActions().size() + 1, "Controls rows: one per action and Reset all");
    for (std::size_t i = 0; i < qol::controlActions().size(); ++i)
        require(controls[i].kind == qol::RowKind::Control && controls[i].label == qol::controlActions()[i], "action row");
    require(controls.back().kind == qol::RowKind::ResetAll && controls.back().label == "Reset all", "Reset all row last");
    for (const qol::Row& row : qol::rowsOf(Tab::Game))
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
    require(menu.handle({EventKind::Left}) == Result::RowChanged && menu.row() == 3, "wraps from the first to the last");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.row() == 0, "wraps from the last to the first");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.row() == 1, "right");
    menu.handle({EventKind::Right});
    menu.handle({EventKind::Right});
    require(menu.row() == 3, "reaches the last Game row");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.row() == 0, "wraps at the last row");
    menu.handle({EventKind::Left});
    require(menu.handle({EventKind::Left}) == Result::RowChanged && menu.row() == 2, "left");

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
    menu.handle({EventKind::Right});
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

void valueStepping() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    goToQualityTab(menu);
    require(menu.handle({EventKind::Up}) == Result::MasterCycled && settings.master() == qol::Master::Improved, "Up steps the master forward");
    require(menu.handle({EventKind::Up}) == Result::Ignored && settings.master() == qol::Master::Improved, "Up stops without a custom mix");
    require(menu.handle({EventKind::Down}) == Result::MasterCycled && settings.master() == qol::Master::Original, "Down steps it back");
    require(menu.handle({EventKind::Down}) == Result::Ignored && settings.master() == qol::Master::Original, "Down stops at Original");
    settings.setSwitch(qol::Feature::WidenedView, true);
    menu.handle({EventKind::Up});
    require(menu.handle({EventKind::Up}) == Result::MasterCycled && settings.master() == qol::Master::Custom, "Up reaches Custom once a mix is set");
    require(menu.handle({EventKind::Up}) == Result::Ignored, "Up stops at Custom");
    menu.handle({EventKind::Right});
    require(menu.handle({EventKind::Up}) == Result::Ignored && settings.customSwitch(qol::Feature::WidenedView), "Up on an On toggle stays");
    require(menu.handle({EventKind::Down}) == Result::FeatureToggled && !settings.customSwitch(qol::Feature::WidenedView), "Down turns a toggle off");
    require(menu.handle({EventKind::Down}) == Result::Ignored, "Down on an Off toggle stays");
    require(menu.handle({EventKind::Up}) == Result::FeatureToggled && settings.customSwitch(qol::Feature::WidenedView), "Up turns it on");

    qol::Settings fresh;
    qol::MenuModel second(fresh);
    second.open();
    goToQualityTab(second);
    second.handle({EventKind::Right});
    require(second.handle({EventKind::Up}) == Result::SwitchedToCustom && fresh.master() == qol::Master::Custom &&
                fresh.customSwitch(qol::Feature::WidenedView), "Up on a toggle outside Custom switches to Custom with it on");

    qol::Settings display;
    qol::MenuModel third(display);
    third.setDisplayModes({{800, 600}, {1280, 720}, {1920, 1080}}, {1920, 1080});
    third.open();
    third.handle(qol::tabClick(static_cast<std::size_t>(Tab::Display)));
    require(display.windowMode() == qol::WindowMode::Fullscreen, "default window mode");
    require(third.handle({EventKind::Up}) == Result::Ignored, "Up on Fullscreen stays");
    require(third.handle({EventKind::Down}) == Result::DisplayChanged && display.windowMode() == qol::WindowMode::Windowed, "Down selects windowed");
    require(third.handle({EventKind::Down}) == Result::Ignored, "Down on windowed stays");
    require(third.handle({EventKind::Up}) == Result::DisplayChanged && display.windowMode() == qol::WindowMode::Fullscreen, "Up selects full screen");
    third.handle({EventKind::Right});
    require(third.handle({EventKind::Down}) == Result::DisplayChanged && display.resolution() && *display.resolution() == qol::Resolution{1280, 720},
            "Down from the desktop size steps to the previous mode");
    require(third.handle({EventKind::Down}) == Result::DisplayChanged && *display.resolution() == qol::Resolution{800, 600}, "Down again");
    require(third.handle({EventKind::Down}) == Result::Ignored && *display.resolution() == qol::Resolution{800, 600}, "Down stops at the first mode");
    third.handle({EventKind::Up});
    require(third.handle({EventKind::Up}) == Result::DisplayChanged && *display.resolution() == qol::Resolution{1920, 1080}, "Up steps to the last");
    require(third.handle({EventKind::Up}) == Result::Ignored, "Up stops at the last mode");
}

void displayTab() {
    qol::Settings settings;
    const std::string before = written(settings);
    qol::MenuModel menu(settings);
    menu.open();
    menu.handle(qol::tabClick(static_cast<std::size_t>(Tab::Display)));
    require(menu.tab() == Tab::Display && menu.row() == 0, "display tab selected");
    require(settings.master() == qol::Master::Original, "original mode while using the display tab");
    require(menu.handle({EventKind::Left}) == Result::RowChanged && menu.row() == 1, "display rows wrap left");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.row() == 0, "display rows wrap right");
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

void controlsTab() {
    using qol::BindingEntry;
    const std::vector<std::string> actions{"Up", "A", "Walk"};
    const std::vector<BindingEntry> defaults{{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"},
        {"A", "key", "Space", "hold"}, {"A", "mouse", "Left", "hold"}, {"Walk", "key", "Left_Alt", "hold"}};
    auto validKey = [](const std::string& name) {
        return name == "W" || name == "Up" || name == "Space" || name == "Left Alt" || name == "E";
    };
    qol::ControlsTab tab(actions, defaults, defaults, validKey);
    qol::ControlsTabPort port(tab);
    qol::Settings settings;
    const std::string before = written(settings);
    qol::MenuModel menu(settings);
    menu.setControls(&port);
    menu.open();
    menu.handle(qol::tabClick(static_cast<std::size_t>(Tab::Controls)));
    require(menu.rows().size() == 4 && menu.rows()[3].kind == qol::RowKind::ResetAll, "rows: actions plus Reset all");

    auto rows = menu.controlsRows();
    require(rows.size() == 3 && rows[0].action == "Up" && rows[0].slots[0].text == "key W" && rows[0].slots[1].text == "key Up",
        "row with two slot texts");
    require(rows[1].slots[1].source == "mouse" && rows[1].slots[1].code == "Left" && rows[1].slots[1].mode == "hold", "slot parts");
    require(rows[2].slots[0].bound && !rows[2].slots[1].bound && rows[2].slots[1].text.empty(), "empty second slot");

    require(menu.handle({EventKind::Left}) == Result::RowChanged && menu.row() == 3, "rows wrap left to Reset all");
    require(menu.handle({EventKind::Up}) == Result::Ignored && menu.controlsSlot() == 0, "no slot on Reset all");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.row() == 0, "rows wrap right");
    require(menu.controlsSlot() == 0 && menu.handle({EventKind::Down}) == Result::Ignored && menu.controlsSlot() == 0,
        "down stops at the first slot");
    require(menu.handle({EventKind::Up}) == Result::SlotChanged && menu.controlsSlot() == 1, "up picks the second slot");
    require(menu.handle({EventKind::Up}) == Result::Ignored && menu.controlsSlot() == 1, "up stops at the last slot");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.controlsSlot() == 0, "a new row starts at the first slot");
    menu.handle({EventKind::Left});
    menu.handle({EventKind::Up});
    require(menu.handle(qol::eventForKey(qol::Key::Enter)) == Result::ControlsWaiting, "Activate waits for the selected slot");
    require(menu.controlsWaiting() && menu.controlsWaiting()->row == 0 && menu.controlsWaiting()->slot == 1, "waiting slot is the picked one");
    menu.handle(qol::eventForKey(qol::Key::Escape));
    menu.handle({EventKind::Down});
    menu.handle(qol::eventForKey(qol::Key::Enter));
    require(menu.controlsWaiting() && menu.controlsWaiting()->slot == 0, "down picks the first slot again");
    require(menu.handle({EventKind::Right}) == Result::Ignored && menu.row() == 0, "keys go to the tab while waiting");
    require(menu.handle(qol::tabClick(0)) == Result::Ignored && menu.tab() == Tab::Controls, "no tab change while waiting");
    require(menu.assignInput("key", "Space") == Result::ControlsConflict && menu.controlsWaiting(), "taken input refused, still waiting");
    require(menu.controlsMessage() == "key Space is already used by A" && menu.controlsConflictAction() == "A", "refuse message");
    require(menu.assignInput("key", "Nonsense") == Result::Rejected, "invalid input rejected");
    require(menu.handle(qol::eventForKey(qol::Key::Escape)) == Result::ControlsCancelled && menu.isOpen() &&
                !menu.controlsWaiting() && menu.controlsMessage().empty(),
        "Escape cancels the wait and keeps the menu open");
    require(menu.assignInput("key", "E") == Result::Ignored, "input without waiting ignored");

    require(menu.handle(qol::slotClick(0, 1)) == Result::ControlsWaiting && menu.controlsWaiting()->slot == 1, "slot click waits for that slot");
    require(menu.assignInput("key", "E") == Result::ControlsChanged && menu.controlsRows()[0].slots[1].text == "key E", "rebound slot");
    require(menu.handle(qol::rowClick(2)) == Result::ControlsWaiting && menu.controlsWaiting()->row == 2 && menu.controlsWaiting()->slot == 0,
        "row click waits for the first slot");
    menu.handle(qol::eventForKey(qol::Key::PadB));
    require(menu.handle(qol::slotClick(2, 2)) == Result::Rejected, "slot past the last slot rejected");
    require(menu.handle(qol::slotClick(9, 0)) == Result::Rejected, "row past the last row rejected");

    require(menu.handle(qol::rowClick(3)) == Result::ControlsConfirming && menu.controlsConfirming(), "Reset all asks first");
    require(std::string(qol::resetAllQuestion) == "Reset all controls to their defaults?", "question text");
    require(menu.handle(qol::eventForKey(qol::Key::Escape)) == Result::ControlsCancelled && !menu.controlsConfirming() &&
                tab.bindings() != defaults,
        "Escape declines the question and keeps the bindings");
    menu.handle(qol::rowClick(3));
    require(menu.handle({EventKind::Right}) == Result::Ignored && menu.controlsConfirming(), "the question is modal");
    require(menu.handle(qol::eventForKey(qol::Key::Enter)) == Result::ControlsChanged && !menu.controlsConfirming() &&
                tab.bindings() == defaults,
        "confirming restores the defaults");
    require(menu.handle(qol::rowClick(3)) == Result::Ignored && !menu.controlsConfirming(),
        "Reset all with nothing to reset asks nothing");
    require(menu.handle(qol::modeClick(0, 0)) == Result::ControlsChanged && menu.controlsRows()[0].slots[0].mode == "toggle",
        "mode click switches the slot to toggle");
    require(menu.handle(qol::clearClick(0, 1)) == Result::ControlsChanged && !menu.controlsRows()[0].slots[1].bound,
        "clear click empties the slot");
    require(menu.handle(qol::modeClick(0, 1)) == Result::Rejected, "mode click on an empty slot rejected");
    require(menu.handle(qol::clearClick(9, 0)) == Result::Rejected, "clear click past the last row rejected");
    require(menu.handle(qol::modeClick(3, 0)) == Result::Ignored, "mode click on Reset all ignored");
    require(menu.handle({EventKind::Right}) == Result::RowChanged && menu.row() == 0, "keys work again");
    require(written(settings) == before, "the Controls tab changes no QoL setting");
}

}

int main() {
    originalModeFirst();
    structure();
    eventMapping();
    navigation();
    valueStepping();
    masterCycling();
    toggles();
    displayTab();
    controlsTab();
    rejections();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-menu-model: all checks passed\n";
    return 0;
}
