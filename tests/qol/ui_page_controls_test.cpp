// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04: Controls tab bullet (columns Action, Slot 1,
// Slot 2; "Press a key... (Esc cancel, Del clear)"; Esc and the menu keys cannot be rebound; key already bound asks
// Swap or Cancel), answer 7 (Mode column per action, both slots follow it), 7b (pad buttons only with a pad),
// 8 (Reset to defaults resets all bindings and modes and asks first), 12 (right button binds, only Esc cancels),
// 15 and 16 (cells on a row, empty Slot 2 refused behind an empty Slot 1).
// Expectations written from the spec by 20261101-qolui3-A on 2026-11-01. The menu keys are the brief's list
// (Esc, Q, E, PageUp, PageDown, the arrows, Enter) plus Delete, which clears.
#include "qol/ui_page_controls.hpp"

#include <iostream>
#include <set>
#include <string>

namespace {

using namespace qol;
using namespace qol::ui;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

const std::vector<std::string> actions{"Up", "A", "Walk", "Jump"};

std::vector<BindingEntry> defaults() {
    return {{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"},
        {"A", "mouse", "Left", "hold"}, {"Walk", "key", "Left_Alt", "hold"}};
}

bool validInput(const std::string& source, const std::string& code) {
    static const std::set<std::string> keys{"W", "Up", "Space", "Left Alt", "E", "Q", "F", "G", "Page Up", "PageUp", "PageDown",
        "Down", "Left", "Right", "Return", "Enter", "Delete", "Escape"};
    static const std::set<std::string> mouse{"Left", "Middle", "Right", "X1", "X2"};
    static const std::set<std::string> pad{"A", "B", "X", "Y"};
    if (source == "key") return keys.count(code) != 0;
    if (source == "mouse") return mouse.count(code) != 0;
    return source == "pad" && pad.count(code) != 0;
}

struct Fixture {
    ControlsTab tab{AnyInput{}, actions, defaults(), defaults(), validInput};
    ControlsTabPort port{tab};
    ControlsPage page{port};
};

Action activate(int row, int cell) {
    Action a;
    a.kind = ActionKind::Activate;
    a.index = row;
    a.item = cell;
    return a;
}

Action input(InputSource source, const std::string& code) {
    Action a;
    a.kind = ActionKind::PromptInput;
    a.source = source;
    a.code = code;
    return a;
}

Action simple(ActionKind kind) {
    Action a;
    a.kind = kind;
    return a;
}

Action click(int button) {
    Action a;
    a.kind = ActionKind::ModalClick;
    a.button = button;
    return a;
}

bool is(const ControlsPageOutcome& outcome, ControlsPageEvent event) { return outcome.event == event; }

std::vector<std::string> cells(const Fixture& f, std::size_t row) { return f.page.rows()[row].cells; }

void rowText() {
    Fixture f;
    const auto rows = f.page.rows();
    require(rows.size() == 5, "one row per action plus the reset button");
    require(f.page.columnTitles() == std::vector<std::string>({"Action", "Slot 1", "Slot 2", "Mode"}), "column titles");
    require(rows[0].label == "Up" && rows[0].cells == std::vector<std::string>({"W", "Up", "< Hold >"}), "two keys");
    require(rows[1].cells == std::vector<std::string>({"Space", "Mouse Left", "< Hold >"}), "key and mouse button");
    require(rows[2].cells == std::vector<std::string>({"Left Alt", "Unbound", "< Hold >"}), "key names have spaces, an empty slot is unbound");
    require(rows[3].cells == std::vector<std::string>({"Unbound", "Unbound", "< Hold >"}), "no binding at all");
    require(rows[0].value == "W | Up | < Hold >", "value text lists the cells");
    require(rows[4].label == "Reset to defaults" && rows[4].kind == RowKind::Button && rows[4].cells.empty(), "reset button row");
    require(f.page.dropdown(0).count == 0 && f.page.dropdown(4).count == 0, "no dropdown rows");
}

void promptAndAssign() {
    Fixture f;
    auto out = f.page.handle(activate(2, 1));
    require(is(out, ControlsPageEvent::PromptOpen) && out.text == "Press a key... (Esc cancel, Del clear)", "slot opens the prompt");
    require(f.page.promptOpen() && f.page.promptText() == out.text, "prompt state and text");
    out = f.page.handle(input(InputSource::Key, "F"));
    require(is(out, ControlsPageEvent::Changed) && !f.page.promptOpen(), "a key binds and closes the prompt");
    require(cells(f, 2) == std::vector<std::string>({"Left Alt", "F", "< Hold >"}), "new key shown in slot 2");

    require(is(f.page.handle(activate(0, 0)), ControlsPageEvent::PromptOpen), "slot 1 of Up");
    out = f.page.handle(input(InputSource::Key, "G"));
    require(is(out, ControlsPageEvent::Changed) && cells(f, 0)[0] == "G" && cells(f, 0)[1] == "Up", "rebinding replaces the slot");

    require(is(f.page.handle(activate(3, 1)), ControlsPageEvent::Rejected) && !f.page.promptOpen(), "slot 2 cannot be filled before slot 1");
    require(is(f.page.handle(input(InputSource::Key, "F")), ControlsPageEvent::Nothing), "raw input without a prompt does nothing");
    require(is(f.page.handle(activate(9, 0)), ControlsPageEvent::Nothing), "row out of range does nothing");
}

void escapeAndDelete() {
    Fixture f;
    f.page.handle(activate(0, 0));
    auto out = f.page.handle(simple(ActionKind::PromptCancelled));
    require(is(out, ControlsPageEvent::PromptClosed) && !f.page.promptOpen() && f.tab.bindings() == defaults(), "Esc cancels and keeps the binding");
    f.page.handle(activate(0, 0));
    out = f.page.handle(input(InputSource::Key, "Escape"));
    require(is(out, ControlsPageEvent::PromptClosed) && !f.page.promptOpen() && f.tab.bindings() == defaults(), "a raw Escape cancels as well");

    f.page.handle(activate(2, 0));
    out = f.page.handle(simple(ActionKind::PromptClear));
    require(is(out, ControlsPageEvent::Changed) && !f.page.promptOpen(), "Del clears the slot and closes the prompt");
    require(cells(f, 2) == std::vector<std::string>({"Unbound", "Unbound", "< Hold >"}), "cleared slot is unbound");

    f.page.handle(activate(0, 0));
    f.page.handle(simple(ActionKind::PromptClear));
    require(cells(f, 0)[0] == "Up" && cells(f, 0)[1] == "Unbound", "clearing slot 1 moves slot 2 up");

    f.page.handle(activate(3, 0));
    out = f.page.handle(simple(ActionKind::PromptClear));
    require(is(out, ControlsPageEvent::PromptClosed) && !f.page.promptOpen(), "Del on an empty slot just closes the prompt");
    require(is(f.page.handle(simple(ActionKind::PromptClear)), ControlsPageEvent::Nothing), "Del without a prompt does nothing");
    require(is(f.page.handle(simple(ActionKind::PromptCancelled)), ControlsPageEvent::Nothing), "cancel without a prompt does nothing");
}

void menuKeysRejected() {
    for (const char* key : {"Q", "E", "PageUp", "Page_Up", "PageDown", "Up", "Down", "Left", "Right", "Return", "Enter", "Delete"}) {
        Fixture f;
        f.page.handle(activate(2, 1));
        const auto out = f.page.handle(input(InputSource::Key, key));
        require(is(out, ControlsPageEvent::Rejected) && f.page.promptOpen() && f.tab.bindings() == defaults(),
            std::string("menu key cannot be bound: ") + key);
    }
    Fixture f;
    f.page.handle(activate(2, 1));
    require(isMenuKey("Page Up") && isMenuKey("page_down") && !isMenuKey("F") && !isMenuKey("Left_Alt"), "menu key names");
    require(is(f.page.handle(input(InputSource::Key, "F")), ControlsPageEvent::Changed), "other keys still bind");
}

void padAndMouse() {
    Fixture f;
    f.page.handle(activate(2, 1));
    auto out = f.page.handle(input(InputSource::Pad, "A"));
    require(is(out, ControlsPageEvent::Rejected) && f.page.promptOpen() && !f.page.padConnected(), "pad button refused without a pad");
    f.page.setPadConnected(true);
    out = f.page.handle(input(InputSource::Pad, "A"));
    require(is(out, ControlsPageEvent::Changed) && cells(f, 2)[1] == "Pad A", "pad button binds with a pad");

    f.page.handle(activate(3, 0));
    out = f.page.handle(click(3));
    require(is(out, ControlsPageEvent::Changed) && cells(f, 3)[0] == "Mouse Right", "right mouse button binds");
    f.page.handle(activate(3, 1));
    out = f.page.handle(input(InputSource::Mouse, "Middle"));
    require(is(out, ControlsPageEvent::Changed) && cells(f, 3)[1] == "Mouse Middle", "middle mouse button binds");
    f.page.handle(activate(0, 0));
    out = f.page.handle(click(1));
    require(is(out, ControlsPageEvent::NeedsSwapDialog) && out.text == "A", "left click is a binding too and conflicts with A");
    f.page.handle(simple(ActionKind::DialogCancelled));
    f.page.handle(activate(0, 0));
    require(is(f.page.handle(click(9)), ControlsPageEvent::Rejected) && f.page.promptOpen(), "unknown mouse button refused");
    f.page.handle(simple(ActionKind::PromptCancelled));
    require(is(f.page.handle(click(1)), ControlsPageEvent::Nothing), "click without a prompt does nothing");
}

void modeColumn() {
    Fixture f;
    auto out = f.page.handle(activate(1, controlsModeCell));
    require(is(out, ControlsPageEvent::Changed), "mode cell changes the mode");
    require(cells(f, 1)[2] == "< Toggle >" && f.tab.bindings()[2].mode == "toggle" && f.tab.bindings()[3].mode == "toggle", "both slots follow the mode");
    f.page.handle(activate(2, controlsModeCell));
    f.page.handle(activate(2, 1));
    f.page.handle(input(InputSource::Key, "F"));
    require(cells(f, 2)[2] == "< Toggle >" && f.tab.bindings()[5].mode == "toggle" && f.tab.bindings()[4].mode == "toggle", "a new slot follows the mode of the row");
    f.page.handle(activate(1, controlsModeCell));
    require(cells(f, 1)[2] == "< Hold >" && f.tab.bindings()[2].mode == "hold" && f.tab.bindings()[3].mode == "hold", "mode toggles back");
    require(is(f.page.handle(activate(3, controlsModeCell)), ControlsPageEvent::Nothing), "mode of an unbound action does nothing");
}

void resetAfterConfirm() {
    Fixture f;
    require(is(f.page.handle(activate(f.page.resetRow(), 0)), ControlsPageEvent::Nothing), "reset at defaults needs nothing");
    f.page.handle(activate(0, 0));
    f.page.handle(input(InputSource::Key, "G"));
    f.page.handle(activate(1, controlsModeCell));
    const auto changed = f.tab.bindings();
    auto out = f.page.handle(activate(f.page.resetRow(), 0));
    require(is(out, ControlsPageEvent::NeedsConfirm) && out.text == "Reset all controls to their defaults?" && out.text == f.page.resetQuestion(), "reset asks first");
    require(f.tab.bindings() == changed, "nothing is reset before the confirm");
    out = f.page.handle(simple(ActionKind::DialogCancelled));
    require(is(out, ControlsPageEvent::PromptClosed) && f.tab.bindings() == changed, "cancelling the question keeps the bindings");
    require(is(f.page.reset(), ControlsPageEvent::Nothing), "reset without a question does nothing");
    f.page.handle(activate(f.page.resetRow(), 0));
    out = f.page.reset();
    require(is(out, ControlsPageEvent::Changed) && f.tab.bindings() == defaults(), "reset restores all bindings and modes");
}

void conflictOutcome() {
    Fixture f;
    f.page.handle(activate(2, 1));
    auto out = f.page.handle(input(InputSource::Key, "Space"));
    require(is(out, ControlsPageEvent::NeedsSwapDialog) && out.text == "A" && f.page.swapOtherAction() == "A", "a bound key names the other action");
    require(f.tab.bindings() == defaults() && f.page.promptOpen(), "nothing is swapped by the page");
    out = f.page.handle(simple(ActionKind::DialogCancelled));
    require(is(out, ControlsPageEvent::PromptClosed) && !f.page.promptOpen() && f.page.swapOtherAction().empty(), "cancelling the swap dialog closes everything");
}

void withNavigation() {
    for (const auto size : {std::pair<int, int>{1280, 720}, {1920, 1080}, {3840, 2160}}) {
        Fixture f;
        UiNav nav([](std::string_view text, float pixels) { return 0.5f * pixels * static_cast<float>(text.size()); });
        nav.setWindow(size.first, size.second);
        nav.setTabs({"Game", "Display", "Controls", "Quality of life"});
        nav.selectTab(2);
        nav.setRows(f.page.rows());
        const auto layout = nav.layout();
        require(layout.rows[2].cells.size() == 3, "layout has three cells for a binding row");
        const auto& cell = layout.rows[2].cells[1];
        auto actions = nav.handle(mouseDown(cell.x + cell.width / 2, cell.y + cell.height / 2));
        require(actions.size() == 1 && actions[0].kind == ActionKind::Activate && actions[0].index == 2 && actions[0].item == 1, "click on a slot cell activates it");
        const auto result = f.page.handle(actions[0]);
        require(is(result, ControlsPageEvent::PromptOpen), "the page opens the prompt");
        nav.setModal(Modal::Prompt);
        actions = nav.handle(promptInput(InputSource::Key, "F"));
        require(actions.size() == 1 && is(f.page.handle(actions[0]), ControlsPageEvent::Changed), "a raw key reaches the page through the nav");
        nav.setModal(Modal::None);
        nav.setRows(f.page.rows());
        require(nav.screen().rows[2].cells[1] == "F", "rows refresh after the change");
        nav.setModal(Modal::Prompt);
        actions = nav.handle(keyEvent(Key::Delete));
        require(actions.size() == 1 && actions[0].kind == ActionKind::PromptClear, "Delete reaches the page as PromptClear");
        actions = nav.handle(keyEvent(Key::Escape));
        require(actions.size() == 1 && is(f.page.handle(actions[0]), ControlsPageEvent::Nothing), "Esc reaches the page");
    }
}

}

int main() {
    rowText();
    promptAndAssign();
    escapeAndDelete();
    menuKeysRejected();
    padAndMouse();
    modeColumn();
    resetAfterConfirm();
    conflictOutcome();
    withNavigation();
    if (failures != 0) return 1;
    std::cout << "ui page controls ok\n";
    return 0;
}
