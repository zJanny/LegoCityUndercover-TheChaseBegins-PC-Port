// Input routing of the settings menu window, pinned to qol-settings-drawing.md §2026-10-29T12:11:55 (section 2 scale, section 4
// navigation: Up/Down, Enter and pad A, Escape and pad B, PageUp/PageDown and pad L/R, Left/Right, the wheel, hover only on entering a
// slot, clicks on the tap circles, Back and Select; while the Controls tab waits the inputs go to it and Escape cancels) and
// qol-settings.md §2026-10-29T11:06:51 (keys, mouse buttons and pad buttons are bound). Key names are SDL key names, pad names the
// controls-file names of docs/controls.md. Geometry: the values menuPageSetup reads from the layouts (20261029-qolmenu2-A, 2026-10-29,
// chase-layoutdump, as qol-menu-hit pins them). Written by 20261029-qolhook-B, 2026-10-29.
#include "qol/menu_window.hpp"

#include "qol/controls_port.hpp"

#include <iostream>
#include <string>

namespace {

using qol::Result;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

qol::MenuPageSetup setup() {
    qol::MenuPageSetup s;
    s.geometry.slotCentres = {{{54, 119}, {159, 119}, {264, 119}}};
    s.geometry.slotRadius = 32;
    s.geometry.back = {11, 209, 32, 32};
    s.geometry.select = {276, 209, 32, 32};
    s.geometry.previousTab = {2, 95, 16, 32};
    s.geometry.nextTab = {298, 95, 16, 32};
    s.geometry.header = {-70, 31.667f, 256, 32};
    return s;
}

constexpr int windowWidth = 1920, windowHeight = 1080;
constexpr float pageX = 240, scale = 4.5f;

float wx(float x) { return pageX + scale * x; }
float wy(float y) { return scale * y; }

void closedWindowIgnoresEverything() {
    qol::Settings settings;
    qol::MenuModel model(settings);
    qol::MenuWindow window(model, setup(), qol::controlsListForm(setup()));
    require(!window.isOpen(), "the window starts closed");
    require(window.keyDown("Return") == Result::Ignored && window.keyDown("Down") == Result::Ignored, "keys are ignored while closed");
    require(window.padDown("a") == Result::Ignored, "pad buttons are ignored while closed");
    require(window.mouseDown(qol::leftMouseButton, windowWidth, windowHeight, wx(159), wy(119)) == Result::Ignored,
        "clicks are ignored while closed");
    require(window.mouseMoved(windowWidth, windowHeight, wx(54), wy(119)) == Result::Ignored, "motion is ignored while closed");
    require(window.wheel(1) == Result::Ignored, "the wheel is ignored while closed");
    require(model.row() == 0 && model.tab() == qol::Tab::Game && settings.master() == qol::Master::Original,
        "nothing changed in original mode");
}

void keys() {
    require(qol::menuKeyForName("Return") == qol::Key::Enter && qol::menuKeyForName("Escape") == qol::Key::Escape &&
            qol::menuKeyForName("PageUp") == qol::Key::PageUp && qol::menuKeyForName("PageDown") == qol::Key::PageDown &&
            qol::menuKeyForName("Up") == qol::Key::Up && qol::menuKeyForName("Left") == qol::Key::Left,
        "SDL key names map to the menu keys");
    require(!qol::menuKeyForName("W") && !qol::menuKeyForName("Space") && !qol::menuKeyForName("up"), "other keys have no menu key");
    require(qol::menuKeyForPad("a") == qol::Key::PadA && qol::menuKeyForPad("b") == qol::Key::PadB &&
            qol::menuKeyForPad("dpup") == qol::Key::PadUp && qol::menuKeyForPad("dpright") == qol::Key::PadRight &&
            qol::menuKeyForPad("leftshoulder") == qol::Key::PadL && qol::menuKeyForPad("rightshoulder") == qol::Key::PadR,
        "pad names map to the pad keys");
    require(!qol::menuKeyForPad("x") && !qol::menuKeyForPad("start") && !qol::menuKeyForPad("A"), "other pad buttons have no menu key");

    qol::Settings settings;
    qol::MenuModel model(settings);
    qol::MenuWindow window(model, setup(), qol::controlsListForm(setup()));
    window.open();
    require(window.isOpen(), "open opens the model");
    require(window.keyDown("Right") == Result::RowChanged && model.row() == 1, "Right moves to the next row");
    require(window.keyDown("W") == Result::Ignored && model.row() == 1, "a key without a menu meaning is ignored");
    require(window.keyDown("PageUp") == Result::TabChanged && model.tab() == qol::Tab::QualityOfLife, "PageUp goes to the previous tab");
    require(window.keyDown("Return") == Result::MasterCycled && settings.master() == qol::Master::Improved, "Enter cycles the master row");
    require(window.padDown("rightshoulder") == Result::TabChanged && model.tab() == qol::Tab::Game, "pad R goes to the next tab");
    require(window.padDown("dpright") == Result::RowChanged && model.row() == 2, "pad Right moves rows");
    require(window.padDown("x") == Result::Ignored, "pad X is ignored");
    require(window.padDown("b") == Result::Closed && !window.isOpen(), "pad B closes the menu");
    window.open();
    require(window.keyDown("Escape") == Result::Closed && !window.isOpen(), "Escape closes the menu");
}

void pointer() {
    qol::Settings settings;
    qol::MenuModel model(settings);
    qol::MenuWindow window(model, setup(), qol::controlsListForm(setup()));
    window.open();
    require(window.mouseMoved(windowWidth, windowHeight, wx(100), wy(180)) == Result::Ignored, "no area under the pointer");
    require(window.mouseMoved(windowWidth, windowHeight, wx(54), wy(119)) == Result::RowChanged && model.row() == 3,
        "entering the left slot selects its row");
    require(window.mouseMoved(windowWidth, windowHeight, wx(56), wy(121)) == Result::Ignored && model.row() == 3,
        "a pointer resting in the slot does not spin the round");
    require(window.mouseMoved(windowWidth, windowHeight, 100, 500) == Result::Ignored, "the black bar is outside the page");
    require(window.mouseMoved(windowWidth, windowHeight, wx(54), wy(119)) == Result::RowChanged && model.row() == 2,
        "coming back from the bar enters the slot again");

    require(window.mouseDown(qol::leftMouseButton, windowWidth, windowHeight, wx(305), wy(100)) == Result::TabChanged &&
            model.tab() == qol::Tab::Display,
        "a click on the right arrow goes to the next tab");
    require(window.mouseDown(3, windowWidth, windowHeight, wx(305), wy(100)) == Result::Ignored && model.tab() == qol::Tab::Display,
        "the right button does not click");
    require(window.mouseDown(qol::leftMouseButton, windowWidth, windowHeight, 100, 500) == Result::Ignored, "a click in the bar does nothing");
    require(window.mouseDown(qol::leftMouseButton, 0, 0, 0, 0) == Result::Ignored, "an empty window does nothing");
    window.keyDown("PageDown");
    window.keyDown("PageDown");
    require(model.tab() == qol::Tab::QualityOfLife, "on the Quality of life tab");
    require(window.mouseDown(qol::leftMouseButton, windowWidth, windowHeight, wx(159), wy(119)) == Result::MasterCycled &&
            settings.master() == qol::Master::Improved,
        "a click on the centre slot activates the master row");
    require(window.mouseDown(qol::leftMouseButton, windowWidth, windowHeight, wx(20), wy(220)) == Result::Closed && !window.isOpen(),
        "a click on Back closes the menu");

    const qol::PageRect small = qol::menuPageRect(400, 240);
    window.open();
    require(window.mouseDown(qol::leftMouseButton, 400, 240, small.x + 290, 220) == Result::MasterCycled,
        "a 400x240 window maps its pixels at scale 1 behind the 40-pixel bar");
}

void wheel() {
    qol::Settings settings;
    qol::MenuModel model(settings);
    qol::MenuWindow window(model, setup(), qol::controlsListForm(setup()));
    window.open();
    require(window.wheel(1) == Result::RowChanged && model.row() == 3, "wheel up moves up with wrap");
    require(window.wheel(-2) == Result::RowChanged && model.row() == 0, "wheel down moves down one row");
    require(window.wheel(0) == Result::Ignored && model.row() == 0, "no step, no move");
}

void controlsWaiting() {
    using qol::BindingEntry;
    const std::vector<std::string> actions{"Up", "A", "Walk"};
    const std::vector<BindingEntry> defaults{{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"},
        {"A", "key", "Space", "hold"}, {"A", "mouse", "Left", "hold"}, {"Walk", "key", "Left_Alt", "hold"}};
    auto valid = [](const std::string& source, const std::string& code) {
        if (source == "key") return code == "W" || code == "Up" || code == "Space" || code == "Left Alt" || code == "E" || code == "Return";
        return source == "mouse" || (source == "pad" && (code == "x" || code == "a"));
    };
    qol::ControlsTab tab(qol::AnyInput{}, actions, defaults, defaults, valid);
    qol::ControlsTabPort port(tab);
    qol::Settings settings;
    qol::MenuModel model(settings);
    model.setControls(&port);
    qol::MenuWindow window(model, setup(), qol::controlsListForm(setup()));
    window.open();
    window.keyDown("PageDown");
    window.keyDown("PageDown");
    require(model.tab() == qol::Tab::Controls, "on the Controls tab");

    require(window.keyDown("Return") == Result::ControlsWaiting && model.controlsWaiting(), "Enter waits for an input");
    require(window.keyDown("Down") == Result::Rejected && model.row() == 0, "a navigation key while waiting is bound, not moved");
    require(window.keyDown("E") == Result::ControlsChanged && tab.bindings()[0].code == "E", "a key while waiting is bound");
    require(window.keyDown("Return") == Result::ControlsWaiting, "wait again");
    require(window.keyDown("Left Alt") == Result::ControlsConflict && model.controlsConflictAction() == "Walk",
        "SDL key names are bound with underscores, so a taken key is refused");
    require(window.padDown("x") == Result::ControlsChanged && tab.bindings()[0].source == "pad" && tab.bindings()[0].code == "x",
        "a pad button while waiting is bound");
    require(window.padDown("a") == Result::ControlsWaiting, "pad A waits again");
    require(window.mouseDown(3, windowWidth, windowHeight, 0, 0) == Result::ControlsChanged && tab.bindings()[0].source == "mouse" &&
            tab.bindings()[0].code == "Right",
        "a mouse button while waiting is bound, anywhere in the window");
    window.keyDown("Return");
    require(window.mouseDown(9, windowWidth, windowHeight, 0, 0) == Result::Ignored && model.controlsWaiting(),
        "an unknown mouse button is ignored and the slot keeps waiting");
    require(window.wheel(1) == Result::Ignored && model.controlsWaiting(), "the wheel does nothing while waiting");
    require(window.keyDown("Escape") == Result::ControlsCancelled && !model.controlsWaiting() && window.isOpen(),
        "Escape cancels the wait and keeps the menu open");
}

}

int main() {
    closedWindowIgnoresEverything();
    keys();
    pointer();
    wheel();
    controlsWaiting();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-menu-window: all checks passed\n";
    return 0;
}
