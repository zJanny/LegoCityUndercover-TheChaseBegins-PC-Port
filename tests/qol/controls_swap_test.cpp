// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04, "Controls tab" bullet (a key bound to another action opens a dialog that names the other action and offers to swap the two bindings or cancel) and answer 13 (Enter = Cancel, Swap needs a click or moving the focus to it first), answers 17 (Cancel leaves the prompt open), 18 (the slot's own key opens no dialog) and 19 (the action's other slot opens the dialog).
// Behaviour of the swap (the other action takes the slot's previous binding or becomes unbound, modes stay with their actions): brief of worker 20261101-qolui3-D, 2026-11-01.
// Binding rows and action names: docs/controls.md and the fixtures of controls_tab_test.cpp and controls_file_test.cpp.
#include "qol/controls_file.hpp"
#include "qol/controls_swap.hpp"
#include "qol/controls_tab.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

using qol::BindingEntry;
using qol::ControlsResult;
using qol::SwapButton;

const std::vector<std::string> actions{"Up", "A", "Walk"};

std::vector<BindingEntry> defaults() {
    return {{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"},
        {"A", "mouse", "Left", "hold"}, {"Walk", "key", "Left_Alt", "hold"}};
}

bool validKey(const std::string& name) {
    static const std::set<std::string> known{"W", "Up", "Space", "Left Alt", "E", "Q"};
    return known.count(name) != 0;
}

qol::ControlsTab make() { return qol::ControlsTab(actions, defaults(), defaults(), validKey); }

void swapWithBoundPreviousBinding() {
    auto tab = make();
    tab.activate(1, 0);
    require(tab.assignKey("W") == ControlsResult::Conflict && tab.conflictPending(), "W is held by Up: a swap is pending");
    require(tab.bindings() == defaults() && !tab.changed(), "nothing changes before the choice");
    require(tab.waiting(), "the slot stays selected while the dialog is open");
    require(tab.confirmSwap() == ControlsResult::Changed, "swap confirmed");
    auto expected = defaults();
    expected[0].code = "Space";
    expected[2].code = "W";
    require(tab.bindings() == expected, "A slot 1 holds W, Up slot 1 holds Space, the other slots stay");
    require(tab.changed() && !tab.waiting() && !tab.conflictPending() && tab.conflictAction().empty() && tab.message().empty(), "state after the swap");
}

void swapWithUnboundPreviousBinding() {
    auto tab = make();
    tab.activate(2, 1);
    require(tab.assignKey("W") == ControlsResult::Conflict && tab.conflictPending() && tab.conflictAction() == "Up", "empty slot, W is held by Up");
    require(tab.confirmSwap() == ControlsResult::Changed, "swap confirmed");
    const std::vector<BindingEntry> expected{{"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"}, {"A", "mouse", "Left", "hold"},
        {"Walk", "key", "Left_Alt", "hold"}, {"Walk", "key", "W", "hold"}};
    require(tab.bindings() == expected, "Up loses the key and has one slot left, Walk gets it as its second slot");
    require(tab.rowKeys(0).size() == 1 && tab.rowKeys(2).size() == 2 && tab.rowKeys(2)[1] == "key W", "rows follow");
}

void secondSlotsAndMouseButtons() {
    auto tab = make();
    tab.activate(1, 1);
    require(tab.assignKey("Up") == ControlsResult::Conflict && tab.conflictAction() == "Up", "second slot against the second slot of Up");
    require(tab.confirmSwap() == ControlsResult::Changed, "swap confirmed");
    auto expected = defaults();
    expected[1] = {"Up", "mouse", "Left", "hold"};
    expected[3] = {"A", "key", "Up", "hold"};
    require(tab.bindings() == expected, "a key and a mouse button exchange slots, source and code travel together");
}

void modesStayWithTheirActions() {
    auto tab = make();
    tab.toggleMode(0, 0);
    tab.activate(1, 0);
    tab.assignKey("W");
    tab.confirmSwap();
    require(tab.bindings()[0] == BindingEntry{"Up", "key", "Space", "toggle"}, "Up keeps its toggle mode with the new key");
    require(tab.bindings()[2] == BindingEntry{"A", "key", "W", "hold"}, "A keeps its hold mode with the new key");
}

void swapInsideOneAction() {
    auto tab = make();
    tab.activate(1, 0);
    require(tab.assignInput("mouse", "Left") == ControlsResult::Conflict && tab.conflictAction() == "A" && tab.conflictPending(), "own second slot is a swap");
    tab.confirmSwap();
    require(tab.bindings()[2] == BindingEntry{"A", "mouse", "Left", "hold"} && tab.bindings()[3] == BindingEntry{"A", "key", "Space", "hold"}, "the two slots of A exchange");
}

void rebindingTheSameSlotIsNoSwap() {
    auto tab = make();
    tab.activate(1, 0);
    require(tab.assignKey("Space") == ControlsResult::Conflict && !tab.conflictPending(), "the slot's own key opens no dialog");
    require(!qol::swapDialog(tab) && tab.confirmSwap() == ControlsResult::Ignored && tab.cancelConflict() == ControlsResult::Ignored, "nothing to swap or cancel");
    require(tab.bindings() == defaults(), "unchanged");
}

void cancelLeavesEverythingUnchanged() {
    auto tab = make();
    tab.activate(1, 0);
    tab.assignKey("W");
    require(tab.cancelConflict() == ControlsResult::Cancelled, "cancel answered");
    require(tab.bindings() == defaults() && !tab.changed() && !tab.conflictPending() && tab.conflictAction().empty() && tab.message().empty(), "bindings, flag and message as before");
    require(tab.waiting() && tab.waitingRow() == 1 && tab.waitingSlot() == 0, "the prompt for the slot is still open");
    require(tab.confirmSwap() == ControlsResult::Ignored && tab.cancelConflict() == ControlsResult::Ignored, "no second answer");
    require(tab.assignKey("E") == ControlsResult::Changed && tab.bindings()[2].code == "E", "another key can be bound afterwards");
}

void pendingEndsWithOtherActions() {
    auto tab = make();
    tab.activate(1, 0);
    tab.assignKey("W");
    tab.cancel();
    require(!tab.conflictPending() && tab.confirmSwap() == ControlsResult::Ignored, "Escape of the prompt drops the pending swap");
    tab.activate(1, 0);
    tab.assignKey("W");
    tab.toggleMode(0, 0);
    require(!tab.conflictPending() && tab.confirmSwap() == ControlsResult::Ignored, "a mode change drops it");
    tab.activate(1, 0);
    tab.assignKey("W");
    tab.clearSlot(1, 0);
    require(!tab.conflictPending() && tab.confirmSwap() == ControlsResult::Ignored, "clearing a slot drops it");
    tab.activate(1, 0);
    tab.assignKey("W");
    tab.activate(2, 0);
    require(!tab.conflictPending(), "choosing another slot drops it");
    tab.assignKey("W");
    require(tab.conflictPending(), "pending again");
    tab.requestReset();
    require(!tab.conflictPending() && tab.confirmSwap() == ControlsResult::Ignored, "asking for the reset drops it");
}

void swapBackRestoresTheChangedFlag() {
    auto tab = make();
    tab.activate(1, 0);
    tab.assignKey("W");
    tab.confirmSwap();
    require(tab.changed(), "changed after the swap");
    tab.activate(1, 0);
    tab.assignKey("Space");
    require(tab.confirmSwap() == ControlsResult::Changed && tab.bindings() == defaults() && !tab.changed(), "swapping back gives the original list");
}

void dialogDescriptor() {
    auto tab = make();
    require(!qol::swapDialog(tab), "no dialog without a conflict");
    tab.activate(1, 0);
    require(!qol::swapDialog(tab), "no dialog while only waiting");
    tab.assignKey("W");
    auto dialog = qol::swapDialog(tab);
    require(dialog.has_value(), "dialog while a swap is pending");
    if (!dialog) return;
    require(dialog->title == "Key already in use", "title");
    require(dialog->body == "key W is already used by Up. Swap the two bindings?", "body names the key and the other action");
    require(dialog->buttons.size() == 2 && dialog->buttons[0] == "Swap" && dialog->buttons[1] == "Cancel", "buttons Swap and Cancel");
    require(dialog->defaultFocus == SwapButton::Cancel, "Enter cancels: the default focus is Cancel (answer 13)");
}

void choicesGoThroughResolveSwap() {
    auto tab = make();
    tab.activate(1, 0);
    tab.assignKey("W");
    require(qol::resolveSwap(tab, SwapButton::Cancel) == ControlsResult::Cancelled && tab.bindings() == defaults(), "Cancel leaves the bindings");
    tab.assignKey("W");
    require(qol::resolveSwap(tab, SwapButton::Swap) == ControlsResult::Changed && tab.bindings() != defaults(), "Swap exchanges them");
    require(qol::resolveSwap(tab, SwapButton::Swap) == ControlsResult::Ignored, "no pending swap, no effect");
}

const std::map<std::string, int> keyCodes{{"W", 1}, {"Up", 5}, {"Space", 9}, {"Q", 10}, {"Left Alt", 17}, {"F3", 18}};

int resolve(const std::string& name) {
    auto found = keyCodes.find(name);
    return found == keyCodes.end() ? 0 : found->second;
}

std::string nameOf(int code) {
    for (const auto& [name, value] : keyCodes)
        if (value == code) return name;
    return {};
}

std::vector<BindingEntry> fileEntries() {
    return {{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"}, {"A", "mouse", "Left", "hold"},
        {"B", "mouse", "Right", "toggle"}, {"Walk", "key", "Left_Alt", "hold"}, {"DebugOverlay", "key", "F3", "toggle"}};
}

std::vector<BindingEntry> readFile(const std::string& path) {
    std::ifstream file(path);
    return qol::readBindingEntries(file, resolve, nameOf);
}

ControlsResult savedLikeTheAdapter(qol::ControlsTab& tab, const std::string& path, ControlsResult result) {
    if (result == ControlsResult::Changed) qol::writeBindingFile(path, tab.bindings(), resolve);
    return result;
}

void swapReachesTheFileAndCancelDoesNot() {
    auto directory = std::filesystem::temp_directory_path() / "chase-qol-controls-swap-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    auto path = (directory / "controls.cfg").string();
    qol::writeBindingFile(path, fileEntries(), resolve);
    qol::ControlsTab tab(qol::AnyInput{}, qol::controlActions(), fileEntries(), fileEntries(), qol::fileInputValidator(resolve));
    tab.activate(*tab.rowOf("A"), 0);
    tab.assignKey("W");
    require(savedLikeTheAdapter(tab, path, tab.cancelConflict()) == ControlsResult::Cancelled && readFile(path) == fileEntries(), "cancel writes nothing");
    tab.assignKey("W");
    require(savedLikeTheAdapter(tab, path, tab.confirmSwap()) == ControlsResult::Changed, "swap is a change the adapter saves");
    auto written = readFile(path);
    require(written == tab.bindings() && written != fileEntries(), "the file holds the swapped list");
    require(written[0] == BindingEntry{"Up", "key", "Space", "hold"} && written[2] == BindingEntry{"A", "key", "W", "hold"}, "swapped lines in the file");
    std::filesystem::remove_all(directory);
}

}

int main() {
    swapWithBoundPreviousBinding();
    swapWithUnboundPreviousBinding();
    secondSlotsAndMouseButtons();
    modesStayWithTheirActions();
    swapInsideOneAction();
    rebindingTheSameSlotIsNoSwap();
    cancelLeavesEverythingUnchanged();
    pendingEndsWithOtherActions();
    swapBackRestoresTheChangedFlag();
    dialogDescriptor();
    choicesGoThroughResolveSwap();
    swapReachesTheFileAndCancelDoesNot();
    if (failures == 0) std::cout << "qol-controls-swap: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
