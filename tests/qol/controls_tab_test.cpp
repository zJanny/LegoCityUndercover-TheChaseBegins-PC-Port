// Spec: docs/design/qol-settings.md entry 2026-10-28T09:32:07 (the Controls tab edits the key bindings of config/controls.cfg).
// Binding fields and action names from docs/controls.md and src/input.cpp (Up, A, Walk ...; source key or mouse; hold or toggle).
// Rebind flow and scope: docs/design/qol-settings.md entry 2026-10-29T11:06:51 (taken key refused with a message, Reset all with confirmation, Escape cancels, keys and mouse buttons, two slots with a mode each, clearing); worker 20261029-qolview-C.
// Nothing else is specified; conflict, reset and cancel behaviour are the neutral choices of 20261029-qolwire-C, 2026-10-29 (open items QOLSET-26, QOLSET-27).
#include "qol/controls_tab.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
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

void originalModeDoesNothingWithoutActivation() {
    auto tab = make();
    require(!tab.waiting() && !tab.changed(), "idle after construction");
    require(tab.assignKey("E") == ControlsResult::Ignored, "key without activation ignored");
    require(tab.cancel() == ControlsResult::Ignored, "cancel without activation ignored");
    require(tab.bindings() == defaults() && !tab.changed(), "bindings untouched");
    require(tab.rowCount() == 3 && tab.rowAction(1) == "A", "rows follow the action list");
    auto keys = tab.rowKeys(1);
    require(keys.size() == 2 && keys[0] == "key Space" && keys[1] == "mouse Left", "row shows its bindings");
}

void rebindReplacesOneSlot() {
    auto tab = make();
    require(tab.activate(0, 1) == ControlsResult::Waiting && tab.waiting(), "activation waits");
    require(tab.assignKey("E") == ControlsResult::Changed && !tab.waiting(), "key assigned");
    require(tab.changed(), "changed flag");
    auto expected = defaults();
    expected[1].code = "E";
    require(tab.bindings() == expected, "only the chosen slot changed, mode kept");
    require(tab.rowOf("Walk") == std::optional<std::size_t>(2), "row lookup by action name");
}

void underscoredNameReachesResolverAsSpaces() {
    auto tab = make();
    tab.activate(0, 0);
    require(tab.assignKey("Left_Alt") == ControlsResult::Conflict && tab.conflictAction() == "Walk", "underscore name resolved");
}

void extraSlotAppends() {
    auto tab = make();
    tab.activate(2, 1);
    require(tab.assignKey("Q") == ControlsResult::Changed, "append");
    require(tab.bindings().size() == 6 && tab.bindings().back() == BindingEntry{"Walk", "key", "Q", "hold"}, "new hold binding appended");
}

void cancelKeepsEverything() {
    auto tab = make();
    tab.activate(0, 0);
    require(tab.cancel() == ControlsResult::Cancelled && !tab.waiting(), "cancel ends waiting");
    require(tab.bindings() == defaults() && !tab.changed(), "cancel changes nothing");
    require(tab.assignKey("E") == ControlsResult::Ignored, "key after cancel ignored");
}

void conflictIsReportedNotApplied() {
    auto tab = make();
    tab.activate(1, 0);
    require(tab.assignKey("W") == ControlsResult::Conflict, "bound key reported");
    require(tab.conflictAction() == "Up", "conflicting action named");
    require(tab.bindings() == defaults() && !tab.changed() && tab.waiting(), "nothing changed, still waiting");
    require(tab.assignKey("Space") == ControlsResult::Conflict && tab.conflictAction() == "A", "same key on same action is a conflict too");
    require(tab.cancel() == ControlsResult::Cancelled && tab.conflictAction().empty(), "cancel clears the conflict");
}

void resetRestoresDefaults() {
    auto tab = make();
    require(tab.reset() == ControlsResult::Ignored, "reset at defaults ignored");
    tab.activate(0, 0);
    tab.assignKey("E");
    require(tab.changed(), "changed before reset");
    require(tab.reset() == ControlsResult::Changed && tab.bindings() == defaults() && !tab.changed(), "reset gives the defaults");
}

void rejections() {
    auto tab = make();
    require(tab.activate(3, 0) == ControlsResult::Rejected, "row out of range");
    require(tab.activate(0, 3) == ControlsResult::Rejected, "slot beyond the append slot");
    require(!tab.waiting(), "rejected activation does not wait");
    tab.activate(0, 0);
    require(tab.assignKey("Nonsense_Key") == ControlsResult::Rejected, "malformed key name rejected by the resolver");
    require(tab.assignKey("") == ControlsResult::Rejected, "empty key name rejected");
    require(tab.waiting() && tab.bindings() == defaults(), "rejection keeps waiting and data");
    bool threw = false;
    try { tab.rowAction(9); } catch (const std::out_of_range&) { threw = true; }
    require(threw, "rowAction out of range throws");
    threw = false;
    try { tab.rowKeys(9); } catch (const std::out_of_range&) { threw = true; }
    require(threw, "rowKeys out of range throws");
    require(!tab.rowOf("Jump"), "unknown action has no row");
    threw = false;
    try { qol::ControlsTab(actions, {{"Jump", "key", "W", "hold"}}, defaults(), validKey); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "unknown action in bindings rejected");
    threw = false;
    try { qol::ControlsTab(actions, defaults(), {{"Jump", "key", "W", "hold"}}, validKey); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "unknown action in defaults rejected");
    threw = false;
    try { qol::ControlsTab(actions, defaults(), defaults(), nullptr); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "missing validator rejected");
}

void mouseButtonsCanBeBound() {
    auto tab = make();
    tab.activate(0, 1);
    require(tab.assignInput("mouse", "Right") == ControlsResult::Changed, "mouse button assigned");
    require(tab.bindings()[1] == BindingEntry{"Up", "mouse", "Right", "hold"}, "slot now holds the mouse button");
    tab.activate(2, 1);
    require(tab.assignInput("mouse", "Left") == ControlsResult::Conflict && tab.conflictAction() == "A", "taken mouse button refused");
    require(tab.assignInput("mouse", "Wheel") == ControlsResult::Rejected, "unknown mouse button rejected");
    require(tab.assignInput("pad", "A") == ControlsResult::Rejected, "pad rejected by the default validator");
}

void padInputsGoThroughTheValidator() {
    qol::ControlsTab tab(qol::AnyInput{}, actions, defaults(), defaults(), [](const std::string& source, const std::string& code) {
        return (source == "pad" && code == "South") || (source == "key" && code == "E");
    });
    tab.activate(2, 1);
    require(tab.assignInput("pad", "South") == ControlsResult::Changed, "pad button assigned through the validator");
    tab.activate(0, 0);
    require(tab.assignInput("pad", "South") == ControlsResult::Conflict && tab.conflictAction() == "Walk", "taken pad button refused");
    require(tab.assignInput("pad", "North") == ControlsResult::Rejected, "unknown pad button rejected");
}

void conflictCarriesAMessage() {
    auto tab = make();
    tab.activate(1, 0);
    tab.assignKey("W");
    require(tab.message() == "key W is already used by Up", "message names the input and the action");
    tab.assignKey("E");
    require(tab.message().empty() && !tab.waiting(), "message cleared after success");
}

void escapeCancelsAndCannotBeBound() {
    auto tab = make();
    tab.activate(0, 0);
    require(tab.assignKey("Escape") == ControlsResult::Cancelled && !tab.waiting(), "Escape cancels waiting");
    require(tab.bindings() == defaults(), "Escape is not bound");
}

void twoSlotsOnly() {
    auto tab = make();
    require(tab.activate(0, 2) == ControlsResult::Rejected, "a third slot is refused");
    require(tab.activate(2, 1) == ControlsResult::Waiting, "the secondary slot of a one-binding action is open");
    require(tab.activate(2, 2) == ControlsResult::Rejected, "no slot beyond the secondary");
}

void modePerSlot() {
    auto tab = make();
    require(tab.toggleMode(0, 1) == ControlsResult::Changed, "mode switched");
    require(tab.bindings()[1].mode == "toggle" && tab.bindings()[0].mode == "hold", "only that slot changed");
    require(tab.changed(), "mode change counts as changed");
    require(tab.toggleMode(0, 1) == ControlsResult::Changed && tab.bindings() == defaults() && !tab.changed(), "switching back restores the list");
    tab.toggleMode(0, 1);
    tab.activate(0, 1);
    tab.assignKey("E");
    require(tab.bindings()[1].mode == "toggle" && tab.bindings()[1].code == "E", "rebinding keeps the slot's mode");
    require(tab.toggleMode(0, 2) == ControlsResult::Rejected, "empty slot has no mode");
    require(tab.toggleMode(5, 0) == ControlsResult::Rejected, "row out of range");
}

void clearingRemovesASlot() {
    auto tab = make();
    require(tab.clearSlot(0, 0) == ControlsResult::Changed && tab.changed(), "slot cleared");
    auto keys = tab.rowKeys(0);
    require(keys.size() == 1 && keys[0] == "key Up", "the other slot stays");
    require(tab.clearSlot(0, 1) == ControlsResult::Rejected, "nothing left to clear");
    require(tab.clearSlot(7, 0) == ControlsResult::Rejected, "row out of range");
    tab.activate(0, 0);
    tab.clearSlot(2, 0);
    require(!tab.waiting() && tab.rowKeys(2).empty(), "clearing ends waiting and may empty a row");
    require(tab.reset() == ControlsResult::Changed && tab.bindings() == defaults(), "reset restores cleared slots");
}

void resetNeedsConfirmation() {
    auto tab = make();
    require(tab.requestReset() == ControlsResult::Ignored && !tab.confirming(), "nothing to reset at defaults");
    tab.activate(0, 0);
    tab.assignKey("E");
    require(tab.requestReset() == ControlsResult::Confirming && tab.confirming(), "request asks for confirmation");
    require(tab.bindings() != defaults(), "nothing reset before the confirmation");
    require(tab.cancel() == ControlsResult::Cancelled && !tab.confirming() && tab.bindings() != defaults(), "cancel declines the reset");
    tab.requestReset();
    require(tab.activate(1, 0) == ControlsResult::Waiting && !tab.confirming(), "starting a rebind drops the confirmation");
    tab.cancel();
    require(tab.confirmReset() == ControlsResult::Ignored, "confirm without request ignored");
    tab.requestReset();
    require(tab.confirmReset() == ControlsResult::Changed && tab.bindings() == defaults() && !tab.confirming() && !tab.changed(), "confirm resets all");
}

}

int main() {
    originalModeDoesNothingWithoutActivation();
    rebindReplacesOneSlot();
    underscoredNameReachesResolverAsSpaces();
    extraSlotAppends();
    cancelKeepsEverything();
    conflictIsReportedNotApplied();
    resetRestoresDefaults();
    rejections();
    mouseButtonsCanBeBound();
    padInputsGoThroughTheValidator();
    conflictCarriesAMessage();
    escapeCancelsAndCannotBeBound();
    twoSlotsOnly();
    modePerSlot();
    clearingRemovesASlot();
    resetNeedsConfirmation();
    if (failures == 0) std::cout << "qol-controls-tab: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
