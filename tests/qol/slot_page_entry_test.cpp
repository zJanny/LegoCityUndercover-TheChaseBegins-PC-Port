// Cursor rules of the "PC Settings" bar of the slot page, pinned to the user's report of 2026-11-02 ("The PC settings menu in the save
// selection screen should be able to be selected just like the save slots") and the brief of 20261102-qolmenu-A: Down from the last
// slot reaches the bar, Up leaves it, the activate press opens the menu, nothing wraps, a pointer pick or leaving the page drops the
// focus. Written by 20261102-qolmenu-A, 2026-11-02.
#include "qol/slot_page_entry.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

using Keys = qol::SlotPageEntry::Keys;

Keys down() { Keys k; k.down = true; return k; }
Keys up() { Keys k; k.up = true; return k; }
Keys activate() { Keys k; k.activate = true; return k; }
Keys pick() { Keys k; k.pick = true; return k; }

void reaching() {
    qol::SlotPageEntry entry;
    require(!entry.step(down(), 2, 0, true, 0.1f).swallow && !entry.focused(), "Down from the first of two slots stays on the slots");
    require(!entry.step(activate(), 2, 1, true, 0.1f).open && !entry.focused(), "the activate press on a slot is not the bar's");
    require(!entry.step(up(), 2, 1, true, 0.1f).swallow && !entry.focused(), "Up on the slots is the slots'");
    require(!entry.step(down(), 2, 1, true, 0.1f).swallow && entry.focused(), "Down from the last slot focuses the bar");
}

void onTheBar() {
    qol::SlotPageEntry entry;
    entry.step(down(), 2, 1, true, 0);
    require(entry.step(Keys{}, 2, 1, true, 0.1f).swallow && entry.seconds() > 0.09f, "the focus time counts while focused");
    require(entry.step(down(), 2, 1, true, 0.1f).swallow && entry.focused(), "Down on the bar goes nowhere and is not passed on");
    const auto left = entry.step(up(), 2, 1, true, 0.1f);
    require(left.swallow && !left.open && !entry.focused() && entry.seconds() == 0, "Up leaves the bar and is not passed on");
    entry.step(down(), 2, 1, true, 0);
    const auto pressed = entry.step(activate(), 2, 1, true, 0.1f);
    require(pressed.open && pressed.swallow && entry.focused(), "the activate press opens the menu and keeps the focus on the bar");
}

void dropping() {
    qol::SlotPageEntry entry;
    entry.step(down(), 2, 1, true, 0);
    require(!entry.step(pick(), 2, 1, true, 0.1f).swallow && !entry.focused(), "a pointer pick of a slot drops the focus");
    entry.step(down(), 2, 1, true, 0);
    require(!entry.step(Keys{}, 2, 1, false, 0.1f).swallow && !entry.focused(), "leaving the page (or a confirming slot) drops the focus");
    require(!entry.step(activate(), 2, 1, true, 0.1f).open, "back on the page the focus is on the slots again");
}

}

int main() {
    reaching();
    onTheBar();
    dropping();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-slot-page-entry: all checks passed\n";
    return 0;
}
