// Held keys and the F3 toggle across the on-foot/vehicle set switch (QOLCTL-14). The QoL switch rebinds with rebindKeepingHeld: a
// physical key held stays held in the new set, a toggle binding with the same action, key and mode keeps its state. The plain rebind
// (original mode, controls file reloads) still drops both. Spec: docs/design/qol-controls.md entry 2026-11-01T13:50:47 (the bindings
// depend on whether the player sits in a vehicle; the vehicle set keeps the F3 key of the on-foot set). Cases written by
// 20261112-qolpc-A on 2026-11-12.
#include "input.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace chase;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

constexpr int kW = 'w';
constexpr int kF = 'f';
constexpr int kF3 = 1003;

int resolve(const std::string& name) {
    if (name == "W") return kW;
    if (name == "F") return kF;
    if (name == "F3") return kF3;
    return 0;
}

std::vector<Binding> bindings(const std::string& text) {
    std::istringstream file(text);
    return readBindings(file, resolve);
}

const std::string onFoot = "Up key W hold\nX key F hold\nDebugOverlay key F3 toggle\n";
const std::string vehicle = "A key W hold\nX key F hold\nDebugOverlay key F3 toggle\n";

void heldKeysStayHeld() {
    InputState input(bindings(onFoot));
    input.event(Source::Key, kW, true);
    require(input.active(Action::Up), "W held on foot moves up");
    input.rebindKeepingHeld(bindings(vehicle));
    require(input.active(Action::A) && !input.active(Action::Up), "W held while sitting down accelerates without a new press");
    input.event(Source::Key, kW, false);
    require(!input.active(Action::A), "releasing W releases the held action");
    input.rebindKeepingHeld(bindings(onFoot));
    require(!input.active(Action::Up), "a released key is not held after the switch back");
}

void unboundKeyDoesNotLinger() {
    InputState input(bindings(onFoot));
    input.event(Source::Key, kF, true);
    input.rebindKeepingHeld(bindings("A key W hold\n"));
    input.event(Source::Key, kF, false);
    input.rebindKeepingHeld(bindings(onFoot));
    require(!input.active(Action::X), "a key released while unbound is up when it is bound again");
}

void toggleKeepsItsState() {
    InputState input(bindings(onFoot));
    input.event(Source::Key, kF3, true);
    input.event(Source::Key, kF3, false);
    require(input.active(Action::DebugOverlay), "F3 pressed once is on");
    input.rebindKeepingHeld(bindings(vehicle));
    require(input.active(Action::DebugOverlay), "F3 stays on after sitting down");
    input.rebindKeepingHeld(bindings(onFoot));
    require(input.active(Action::DebugOverlay), "F3 stays on after leaving");
    input.event(Source::Key, kF3, true);
    input.event(Source::Key, kF3, false);
    require(!input.active(Action::DebugOverlay), "the next F3 press turns it off");
}

void toggleStateNeedsTheSameAction() {
    InputState input(bindings(onFoot));
    input.event(Source::Key, kF3, true);
    input.event(Source::Key, kF3, false);
    input.rebindKeepingHeld(bindings("Fullscreen key F3 toggle\n"));
    require(!input.active(Action::Fullscreen), "a toggle state does not move to another action");
}

void fixedBindingsAreKept() {
    InputState input(bindings(onFoot));
    input.fix(Action::Start, Source::Key, 27);
    input.event(Source::Key, 27, true);
    input.rebindKeepingHeld(bindings(vehicle));
    require(input.active(Action::Start), "a held fixed binding stays held");
}

void defaultRebindIsUnchanged() {
    InputState input(bindings(onFoot));
    input.event(Source::Key, kW, true);
    input.event(Source::Key, kF3, true);
    input.event(Source::Key, kF3, false);
    input.rebind(bindings(vehicle));
    require(!input.active(Action::A) && !input.active(Action::DebugOverlay),
        "the plain rebind of the original mode still drops held keys and the toggle");
}

}

int main() {
    defaultRebindIsUnchanged();
    heldKeysStayHeld();
    unboundKeyDoesNotLinger();
    toggleKeepsItsState();
    toggleStateNeedsTheSameAction();
    fixedBindingsAreKept();
    if (failures == 0) std::cout << "qol-held-keys-switch: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
