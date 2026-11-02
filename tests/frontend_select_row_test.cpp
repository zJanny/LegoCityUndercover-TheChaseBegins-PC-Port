// The pointer's row pick of FrontInput (QOLBOT-22, docs/design/qol-screen-layout.md §2026-10-30T14:07:44: a click on a list row selects it
// and presses A). Written by 20261102-qolctl2-D on 2026-11-02 from the Flow rules of the language list and the slot page: the row is
// applied before the pressed buttons, an out-of-range row and the current row change nothing, the pad never sets it. Values: the language
// of row 3 is kLanguageOfRow[3] = 4 (German) and row 5 is 7 (Danish), flow.hpp.
#include "frontend/flow.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

FrontInput pick(unsigned row, std::uint32_t pressed = 0) {
    FrontInput input;
    input.pressed = pressed;
    input.selectRow = row;
    return input;
}

void languageList() {
    Flow flow(nullptr, {});
    flow.start(Screen::Language);
    flow.update(0.03f, pick(3));
    require(flow.screen() == Screen::Language && flow.selectedRow() == 3, "a row pick moves the selection");
    const std::vector<int> sounds = flow.takeSounds();
    require(sounds.size() == 1 && sounds[0] == kSoundMove, "a changed selection plays the move sound");

    flow.update(0.03f, pick(3));
    require(flow.selectedRow() == 3 && flow.takeSounds().empty(), "picking the selected row again is silent");
    flow.update(0.03f, pick(9));
    require(flow.selectedRow() == 3, "a row beyond the list is ignored");

    flow.update(0.03f, pick(5, kButtonA));
    require(flow.screen() == Screen::Prompt && flow.language() == 7, "a row pick with A chooses that row's language, not the old selection");
}

void pickAloneDoesNotConfirm() {
    Flow flow(nullptr, {});
    flow.start(Screen::Language);
    flow.update(0.03f, pick(2));
    require(flow.screen() == Screen::Language && flow.language() == 1, "a row pick without A confirms nothing");
}

void padNeverPicks() {
    Flow flow(nullptr, {});
    flow.start(Screen::Language);
    flow.update(0.03f, FrontInput{kButtonDown});
    require(flow.selectedRow() == 1, "the pad still moves the selection by one row");
    require(!FrontInput{}.selectRow, "a default input picks no row");
}

void slotPage() {
    Flow flow(nullptr, {});
    flow.start(Screen::SlotSelect);
    flow.update(0.03f, pick(1));
    require(flow.selectedSlot() == 1 && !flow.confirming(), "a row pick selects slot 2 without confirming");
    flow.update(0.03f, pick(0, kButtonA));
    require(flow.selectedSlot() == 0 && flow.confirming(), "a row pick with A selects slot 1 and confirms");

    Flow outOfRange(nullptr, {});
    outOfRange.start(Screen::SlotSelect);
    outOfRange.update(0.03f, pick(2, kButtonA));
    require(outOfRange.selectedSlot() == 0 && outOfRange.confirming(), "a row beyond the slots is ignored, A still confirms the selection");
}

void otherScreensIgnoreIt() {
    Flow flow(nullptr, {});
    flow.start(Screen::Autosave);
    flow.update(0.03f, pick(1));
    require(flow.screen() == Screen::Autosave && !flow.confirming(), "a pick on a page without rows does nothing");
    flow.update(0.03f, pick(1, kButtonA));
    require(flow.confirming(), "A of the same input is still read");
}

}

int main() {
    languageList();
    pickAloneDoesNotConfirm();
    padNeverPicks();
    slotPage();
    otherScreensIgnoreIt();
    if (failures) return EXIT_FAILURE;
    std::cout << "frontend select row ok\n";
    return EXIT_SUCCESS;
}
