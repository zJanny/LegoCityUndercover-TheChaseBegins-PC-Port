// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (approved suggestion: Up and Down between rows,
// Left and Right change the value, arrows reach the tabs, Q/E or PageUp/PageDown, Enter, Esc layering, right-click as
// Back, hover focus, wheel, slider drag, LB/RB/D-pad/A/B; answer 6: tabs wrap, rows do not).
// Expectations written from the spec by 20261030-qolui2-A on 2026-10-30; sizes from the layout model at 1080p.
// Rows of the sample screen: 0 slider, 1 selector, 2 toggle, 3 dropdown, 4 button, 5 disabled selector.
#include "qol/ui_nav.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

using namespace qol::ui;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

float fakeWidth(std::string_view text, float pixelSize) { return 0.5f * pixelSize * static_cast<float>(text.size()); }

std::vector<RowSpec> sampleRows(std::size_t count = 6) {
    const RowKind kinds[] = {RowKind::Slider, RowKind::Selector, RowKind::Toggle, RowKind::Dropdown, RowKind::Button, RowKind::Selector};
    std::vector<RowSpec> rows;
    for (std::size_t i = 0; i < count; ++i) rows.push_back({"Setting " + std::to_string(i), kinds[i % 6], "On", 0.5, i != 5});
    return rows;
}

UiNav makeNav(int w = 1920, int h = 1080, std::size_t rows = 6) {
    UiNav nav(fakeWidth);
    nav.setWindow(w, h);
    nav.setTabs({"Game", "Display", "Controls", "Quality of life"});
    nav.setRows(sampleRows(rows));
    nav.setDropdownInfo([](int) { return DropdownInfo{12, 2}; });
    return nav;
}

std::vector<Action> key(UiNav& nav, Key k) { return nav.handle(keyEvent(k)); }

bool only(const std::vector<Action>& actions, ActionKind kind) { return actions.size() == 1 && actions[0].kind == kind; }

std::pair<int, int> centre(const qol::Rect& r) { return {r.x + r.width / 2, r.y + r.height / 2}; }

void checkCommands() {
    require(commandOf(Key::Up) == Command::Up && commandOf(Key::PadUp) == Command::Up, "up keys");
    require(commandOf(Key::Down) == Command::Down && commandOf(Key::PadDown) == Command::Down, "down keys");
    require(commandOf(Key::Left) == Command::Left && commandOf(Key::PadLeft) == Command::Left, "left keys");
    require(commandOf(Key::Right) == Command::Right && commandOf(Key::PadRight) == Command::Right, "right keys");
    require(commandOf(Key::Enter) == Command::Confirm && commandOf(Key::PadA) == Command::Confirm, "confirm keys");
    require(commandOf(Key::Escape) == Command::Back && commandOf(Key::PadB) == Command::Back, "back keys");
    require(commandOf(Key::Q) == Command::TabPrevious && commandOf(Key::PageUp) == Command::TabPrevious &&
                commandOf(Key::PadLB) == Command::TabPrevious, "previous tab keys");
    require(commandOf(Key::E) == Command::TabNext && commandOf(Key::PageDown) == Command::TabNext &&
                commandOf(Key::PadRB) == Command::TabNext, "next tab keys");
}

void checkFocusMovement() {
    UiNav nav = makeNav();
    nav.setRows(sampleRows());
    require(nav.screen().focusRow == 0, "initial focus is the first row");
    key(nav, Key::Down);
    require(nav.screen().focusRow == 1, "down moves to the next row");
    for (int i = 0; i < 10; ++i) key(nav, Key::Down);
    require(nav.screen().focusRow == 4, "rows do not wrap at the end and skip the disabled last row");
    require(!nav.screen().focusOnTabs, "down at the end stays on the rows");
    key(nav, Key::Up);
    require(nav.screen().focusRow == 3, "up moves one row");
    key(nav, Key::PadUp);
    require(nav.screen().focusRow == 2, "pad up moves one row");
    key(nav, Key::PadDown);
    require(nav.screen().focusRow == 3, "pad down moves one row");
    for (int i = 0; i < 3; ++i) key(nav, Key::Up);
    require(nav.screen().focusRow == 0, "up reaches the first row");
    require(!nav.screen().focusOnTabs, "first row is not the tab bar");
    key(nav, Key::Up);
    require(nav.screen().focusOnTabs && nav.screen().focusRow == -1, "up on the first row reaches the tab bar");
    key(nav, Key::Up);
    require(nav.screen().focusOnTabs, "up on the tab bar stays");
    key(nav, Key::Down);
    require(!nav.screen().focusOnTabs && nav.screen().focusRow == 0, "down returns to the first row");
}

void checkValues() {
    UiNav nav = makeNav();
    auto a = key(nav, Key::Right);
    require(only(a, ActionKind::ValueChanged) && a[0].index == 0 && std::fabs(a[0].fraction - 0.6) < 1e-9 && a[0].direction == 1,
            "right raises the slider by one step of ten");
    a = key(nav, Key::PadLeft);
    require(only(a, ActionKind::ValueChanged) && std::fabs(a[0].fraction - 0.5) < 1e-9, "left lowers the slider");
    for (int i = 0; i < 10; ++i) key(nav, Key::Left);
    require(key(nav, Key::Left).empty() && std::fabs(nav.screen().rows[0].fraction) < 1e-9, "slider stops at zero");
    key(nav, Key::Down);
    a = key(nav, Key::Right);
    require(only(a, ActionKind::ValueChanged) && a[0].index == 1 && a[0].direction == 1, "right turns a selector forward");
    a = key(nav, Key::Left);
    require(only(a, ActionKind::ValueChanged) && a[0].index == 1 && a[0].direction == -1, "left turns a selector back");
    require(key(nav, Key::Enter).empty(), "enter does not turn a selector");
    key(nav, Key::Down);
    a = key(nav, Key::Enter);
    require(only(a, ActionKind::ValueChanged) && a[0].index == 2 && a[0].direction == 1, "enter flips a toggle");
    a = key(nav, Key::PadA);
    require(only(a, ActionKind::ValueChanged), "pad A flips a toggle");
    key(nav, Key::Down);
    require(key(nav, Key::Left).empty(), "left does nothing on a dropdown row");
    key(nav, Key::Down);
    a = key(nav, Key::Enter);
    require(only(a, ActionKind::Activate) && a[0].index == 4, "enter activates a button");
    require(key(nav, Key::Down).empty() && nav.screen().focusRow == 4, "down skips a disabled row");
    const auto p = centre(nav.layout().rows[5].rect);
    nav.handle(mouseMove(p.first, p.second));
    require(nav.screen().focusRow == 5, "hover focuses a disabled row to show its note");
    require(key(nav, Key::Right).empty() && key(nav, Key::Enter).empty(), "disabled row gives no action");
}

void checkDisabledSkip() {
    UiNav nav = makeNav();
    auto rows = sampleRows();
    rows[1].enabled = false;
    rows[0].enabled = false;
    nav.setRows(rows);
    require(nav.screen().focusRow == 2, "initial focus skips disabled rows");
    key(nav, Key::Up);
    require(nav.screen().focusOnTabs, "up with only disabled rows above reaches the tab bar");
    key(nav, Key::Down);
    require(nav.screen().focusRow == 2, "down from the tab bar lands on the first enabled row");
    rows = sampleRows();
    rows[3].enabled = false;
    nav.setRows(rows);
    key(nav, Key::Down);
    key(nav, Key::PadDown);
    require(nav.screen().focusRow == 4, "down jumps over a disabled middle row");
    key(nav, Key::PadUp);
    require(nav.screen().focusRow == 2, "up jumps over a disabled middle row");
}

void checkTabs() {
    UiNav nav = makeNav();
    auto a = key(nav, Key::E);
    require(only(a, ActionKind::TabChanged) && a[0].index == 1 && nav.screen().selectedTab == 1, "E goes to the next tab");
    a = key(nav, Key::PageDown);
    require(only(a, ActionKind::TabChanged) && a[0].index == 2, "PageDown goes to the next tab");
    a = key(nav, Key::PadRB);
    require(a[0].index == 3, "RB goes to the next tab");
    a = key(nav, Key::E);
    require(only(a, ActionKind::TabChanged) && a[0].index == 0, "tabs wrap forward");
    a = key(nav, Key::Q);
    require(only(a, ActionKind::TabChanged) && a[0].index == 3, "tabs wrap back");
    a = key(nav, Key::PageUp);
    require(a[0].index == 2, "PageUp goes to the previous tab");
    a = key(nav, Key::PadLB);
    require(a[0].index == 1, "LB goes to the previous tab");
    require(nav.screen().scrollOffset == 0, "tab change resets scroll");

    nav.setRows(sampleRows());
    key(nav, Key::Up);
    require(nav.screen().focusOnTabs, "on the tab bar");
    a = key(nav, Key::Right);
    require(only(a, ActionKind::TabChanged) && a[0].index == 2, "right switches tab on the tab bar");
    a = key(nav, Key::Left);
    require(only(a, ActionKind::TabChanged) && a[0].index == 1, "left switches tab on the tab bar");
    key(nav, Key::PadLeft);
    a = key(nav, Key::Left);
    require(a[0].index == 3, "arrows wrap on the tab bar");
    require(nav.screen().focusOnTabs, "focus stays on the tab bar after switching");
    require(key(nav, Key::Enter).empty(), "enter on the tab bar does nothing");
    nav.setRows(sampleRows());
    key(nav, Key::Down);
    require(!nav.screen().focusOnTabs && nav.screen().focusRow == 0, "down returns to the rows of the new tab");

    UiNav empty(fakeWidth);
    empty.setWindow(1920, 1080);
    require(key(empty, Key::E).empty() && key(empty, Key::Down).empty() && key(empty, Key::Up).empty(), "empty screen is inert");
}

void checkEscLayers() {
    UiNav nav = makeNav();
    require(only(key(nav, Key::Escape), ActionKind::Close), "Esc closes the menu");
    require(only(key(nav, Key::PadB), ActionKind::Close), "B closes the menu");
    require(only(nav.handle(rightClick(5, 5)), ActionKind::Close), "right-click is Back");

    nav.setModal(Modal::Prompt);
    require(only(key(nav, Key::Escape), ActionKind::PromptCancelled), "Esc cancels a prompt first");
    require(key(nav, Key::PadB).empty() && key(nav, Key::Down).empty() && key(nav, Key::Enter).empty(), "prompt takes no navigation");
    auto a = nav.handle(rightClick(7, 9));
    require(only(a, ActionKind::ModalClick) && a[0].button == 3 && a[0].x == 7 && a[0].y == 9, "right-click is a binding in a prompt");
    a = nav.handle(mouseDown(11, 13));
    require(only(a, ActionKind::ModalClick) && a[0].button == 1, "left click is a binding in a prompt");
    require(nav.handle(wheel(1)).empty() && nav.handle(mouseMove(5, 5)).empty(), "prompt ignores wheel and motion");
    nav.setModal(Modal::None);
    require(only(key(nav, Key::Escape), ActionKind::Close), "after the prompt Esc closes the menu");

    nav.setModal(Modal::Dialog);
    require(only(key(nav, Key::Escape), ActionKind::DialogCancelled), "Esc cancels a dialog");
    require(only(key(nav, Key::PadB), ActionKind::DialogCancelled), "B cancels a dialog");
    require(only(nav.handle(rightClick(1, 1)), ActionKind::DialogCancelled), "right-click cancels a dialog");
    require(only(key(nav, Key::Enter), ActionKind::DialogConfirmed), "Enter confirms a dialog");
    require(only(key(nav, Key::PadA), ActionKind::DialogConfirmed), "A confirms a dialog");
    nav.setModal(Modal::SwapDialog);
    require(only(key(nav, Key::Enter), ActionKind::DialogCancelled), "Enter cancels the swap dialog");
    require(only(key(nav, Key::PadA), ActionKind::DialogCancelled), "A cancels the swap dialog");
    require(only(key(nav, Key::Escape), ActionKind::DialogCancelled), "Esc cancels the swap dialog");
    require(only(key(nav, Key::PadB), ActionKind::DialogCancelled), "B cancels the swap dialog");
    require(only(nav.handle(rightClick(1, 1)), ActionKind::DialogCancelled), "right-click cancels the swap dialog");
    require(only(nav.handle(mouseDown(3, 4)), ActionKind::ModalClick), "click in the swap dialog is passed on");
    require(key(nav, Key::Down).empty(), "swap dialog blocks navigation");
    nav.setModal(Modal::Dialog);
    a = nav.handle(mouseDown(3, 4));
    require(only(a, ActionKind::ModalClick) && a[0].button == 1, "left click in a dialog is passed on");
    require(key(nav, Key::Down).empty() && key(nav, Key::E).empty(), "dialog blocks navigation");
    nav.setModal(Modal::None);

    for (int i = 0; i < 3; ++i) key(nav, Key::Down);
    a = key(nav, Key::Enter);
    require(only(a, ActionKind::DropdownOpened) && a[0].index == 3, "enter opens a dropdown");
    require(nav.screen().openDropdownRow == 3 && nav.dropdownFocus() == 2, "dropdown opens on the current item");
    require(key(nav, Key::E).empty() && key(nav, Key::Right).empty(), "open dropdown takes no tab or value keys");
    key(nav, Key::Down);
    require(nav.dropdownFocus() == 3, "down moves in the dropdown");
    a = key(nav, Key::Escape);
    require(only(a, ActionKind::DropdownClosed) && nav.screen().openDropdownRow == -1, "Esc closes the dropdown first");
    require(only(key(nav, Key::Escape), ActionKind::Close), "second Esc closes the menu");
    key(nav, Key::Enter);
    key(nav, Key::Up);
    key(nav, Key::Up);
    key(nav, Key::Up);
    require(nav.dropdownFocus() == 0, "dropdown list does not wrap at the start");
    a = key(nav, Key::Enter);
    require(only(a, ActionKind::DropdownSelected) && a[0].index == 3 && a[0].item == 0, "enter picks the dropdown item");
    key(nav, Key::Enter);
    for (int i = 0; i < 30; ++i) key(nav, Key::Down);
    require(nav.dropdownFocus() == 11, "dropdown list does not wrap at the end");
    require(nav.screen().dropdownFirstItem + static_cast<int>(nav.layout().dropdown.items.size()) > 11 &&
                nav.screen().dropdownFirstItem <= 11, "dropdown scrolls to the focused item");
    a = nav.handle(rightClick(1, 1));
    require(only(a, ActionKind::DropdownClosed), "right-click closes the dropdown");

    UiNav plain = makeNav();
    plain.setDropdownInfo(nullptr);
    for (int i = 0; i < 4; ++i) key(plain, Key::Down);
    require(only(key(plain, Key::Enter), ActionKind::Activate), "dropdown without items just activates");
}

void checkMouse(int w, int h) {
    UiNav nav = makeNav(w, h);
    UiLayout layout = nav.layout();
    auto p = centre(layout.rows[2].rect);
    require(nav.handle(mouseMove(p.first, p.second)).empty() && nav.screen().focusRow == 2 && !nav.screen().focusOnTabs,
            "hover focuses a row");
    key(nav, Key::Down);
    require(nav.screen().focusRow == 3, "keyboard continues from the hovered row");
    p = centre(layout.tabs[1].rect);
    nav.handle(mouseMove(p.first, p.second));
    require(nav.screen().focusOnTabs && nav.screen().selectedTab == 0, "hover on a tab focuses the bar without switching");
    auto a = nav.handle(mouseDown(p.first, p.second));
    require(only(a, ActionKind::TabChanged) && a[0].index == 1, "click on a tab switches");
    require(nav.handle(mouseDown(p.first, p.second)).empty(), "click on the selected tab does nothing");
    nav.setRows(sampleRows());

    layout = nav.layout();
    const RowLayout& toggle = layout.rows[2];
    a = nav.handle(mouseDown(toggle.leftArrow.x + 1, toggle.leftArrow.y + 1));
    require(only(a, ActionKind::ValueChanged) && a[0].index == 2 && a[0].direction == -1, "left arrow click");
    a = nav.handle(mouseDown(toggle.rightArrow.x + 1, toggle.rightArrow.y + 1));
    require(only(a, ActionKind::ValueChanged) && a[0].direction == 1, "right arrow click");
    p = centre(toggle.valueArea);
    a = nav.handle(mouseDown(p.first, p.second));
    require(only(a, ActionKind::ValueChanged) && a[0].direction == 1, "click on the value cycles it");
    a = nav.handle(mouseDown(toggle.label.x + 1, toggle.label.y + 1));
    require(a.empty(), "click on the label only focuses");
    p = centre(layout.rows[1].valueArea);
    a = nav.handle(mouseDown(p.first, p.second));
    require(only(a, ActionKind::ValueChanged) && a[0].index == 1, "click on a selector value cycles it");
    p = centre(layout.rows[4].control);
    a = nav.handle(mouseDown(p.first, p.second));
    require(only(a, ActionKind::Activate) && a[0].index == 4, "click on a button activates it");
    p = centre(layout.rows[5].control);
    require(nav.handle(mouseDown(p.first, p.second)).empty(), "click on a disabled row does nothing");
    p = centre(layout.backButton);
    require(only(nav.handle(mouseDown(p.first, p.second)), ActionKind::Close), "Back button closes");

    p = centre(layout.rows[3].control);
    a = nav.handle(mouseDown(p.first, p.second));
    require(only(a, ActionKind::DropdownOpened), "click opens the dropdown");
    layout = nav.layout();
    require(layout.dropdown.open && layout.dropdown.items.size() > 3, "dropdown popup is laid out");
    p = centre(layout.dropdown.items[1]);
    nav.handle(mouseMove(p.first, p.second));
    require(nav.dropdownFocus() == layout.dropdown.firstItem + 1, "hover focuses a dropdown item");
    a = nav.handle(mouseDown(p.first, p.second));
    require(only(a, ActionKind::DropdownSelected) && a[0].item == layout.dropdown.firstItem + 1 && nav.screen().openDropdownRow == -1,
            "click on an item picks it");
    p = centre(nav.layout().rows[3].control);
    nav.handle(mouseDown(p.first, p.second));
    require(only(nav.handle(mouseDown(2, 2)), ActionKind::DropdownClosed), "click outside closes the dropdown");
    require(only(nav.handle(rightClick(2, 2)), ActionKind::Close), "right-click closes the menu with no dropdown");
}

void checkSliderAndWheel() {
    UiNav nav = makeNav(1920, 1080, 40);
    UiLayout layout = nav.layout();
    const RowLayout& slider = layout.rows[0];
    const int span = slider.track.width - slider.thumb.width;
    const auto xAt = [&](double f) { return slider.track.x + slider.thumb.width / 2 + static_cast<int>(std::lround(f * span)); };
    const int y = slider.track.y + slider.track.height / 2;

    auto a = nav.handle(mouseDown(xAt(0.8), y));
    require(only(a, ActionKind::ValueChanged) && a[0].index == 0 && std::fabs(a[0].fraction - 0.8) < 1e-9, "click on the track sets the value");
    require(nav.dragging() && nav.screen().focusRow == 0, "press on the track starts a drag and focuses");
    a = nav.handle(mouseMove(xAt(0.3), y + 400));
    require(only(a, ActionKind::ValueChanged) && std::fabs(a[0].fraction - 0.3) < 1e-9, "drag follows the pointer outside the row");
    require(nav.handle(mouseMove(xAt(0.3) + 1, y)).empty(), "drag inside one step gives no action");
    a = nav.handle(mouseMove(xAt(1.0) + 500, y));
    require(only(a, ActionKind::ValueChanged) && std::fabs(a[0].fraction - 1.0) < 1e-9, "drag clamps at the right end");
    nav.handle(mouseUp(0, 0));
    require(!nav.dragging(), "release ends the drag");
    require(nav.handle(mouseMove(xAt(0.1), y)).empty() && std::fabs(nav.screen().rows[0].fraction - 1.0) < 1e-9, "no drag after release");

    const RowLayout& number = layout.rows[0];
    require(nav.handle(mouseDown(number.numberBox.x + 1, number.numberBox.y + 1)).empty(), "number box click only focuses");

    require(nav.screen().scrollOffset == 0, "starts at the top");
    a = nav.handle(wheel(1));
    require(only(a, ActionKind::Scrolled) && a[0].item == layout.scale.rowHeight && nav.screen().scrollOffset == layout.scale.rowHeight,
            "wheel notch scrolls one row down");
    require(only(nav.handle(wheel(-1)), ActionKind::Scrolled) && nav.screen().scrollOffset == 0, "wheel scrolls back up");
    require(nav.handle(wheel(-1)).empty(), "wheel stops at the top");
    nav.handle(wheel(1000));
    require(nav.screen().scrollOffset == layout.scroll.maxOffset, "wheel stops at the end");
    require(nav.handle(wheel(1)).empty(), "wheel stops at the bottom");

    UiNav keys = makeNav(1920, 1080, 40);
    for (int i = 0; i < 60; ++i) key(keys, Key::Down);
    require(keys.screen().focusRow == 39, "keyboard reaches the last row");
    require(keys.screen().scrollOffset == keys.layout().scroll.maxOffset, "focus scrolls the last row into view");
    require(keys.layout().rows[39].visible, "last row visible");
    for (int i = 0; i < 38; ++i) key(keys, Key::Up);
    require(keys.screen().scrollOffset == 0 && keys.screen().focusRow == 0, "focus scrolls back to the first row");
}

}

int main() {
    checkCommands();
    checkFocusMovement();
    checkValues();
    checkDisabledSkip();
    checkTabs();
    checkEscLayers();
    const int sizes[][2] = {{1280, 720}, {1920, 1080}, {3840, 2160}, {2560, 1080}, {800, 600}};
    for (const auto& size : sizes) checkMouse(size[0], size[1]);
    checkSliderAndWheel();
    if (failures != 0) return 1;
    std::cout << "ui nav ok\n";
    return 0;
}
