// The Languages page of the pause screen (src/runtime/languages_page.cpp; docs/re/ui-screens.md). Values read from the shipped code
// (llvm-objdump on the analysis ELF, 20261030-qolstores-E, 2026-10-30): table 0x3759b0 (24-byte entries, first bytes 1 2 3 4 5 7 6 9 8, ended by 0),
// the sound ids 0x14 (A 0x31db18) and 0x11 (B 0x31daf0), the name keys of switch 0x175d34 and the picture paths of switch 0x31dc84.
#include "runtime/languages_page.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void tables() {
    const std::uint8_t table[kLanguageCount] = {1, 2, 3, 4, 5, 7, 6, 9, 8};
    for (int i = 0; i < kLanguageCount; ++i) require(kLanguageTable[std::size_t(i)] == table[i], "table order");
    require(kLanguageNameKeys[0] == 0x4f11523f && kLanguageNameKeys[5] == 0x363c0017 && kLanguageNameKeys[6] == 0xf63a151e && kLanguageNameKeys[8] == 0x3f283c62,
            "name keys by language index");
    require(std::string(kLanguageImages[3]) == "UI_Language_German.png" && std::string(kLanguageImages[7]) == "UI_Language_Portuguese.png", "pictures");
}

void flow() {
    LanguagesPage page;
    std::uint8_t options[0x21c] = {};
    options[kLanguageOffset] = 1;

    LanguagesPageActions actions = page.enter();
    require(actions.backShortcut == true && !actions.selectShortcut && !actions.closeList, "enter: Back on");
    require(page.selectionChanged(-1).selectShortcut == false, "no row selected: Select off");
    require(page.selectionChanged(4).selectShortcut == true, "a row selected: Select on");

    actions = page.b();
    require(actions.sounds == std::vector<int>{0x11} && actions.optionsPage && !actions.saveRequest, "B: back sound, Options page");
    require(!page.update(false, options).saveRequest && options[kLanguageOffset] == 1, "nothing pending, nothing saved");

    actions = page.a(5);
    require(actions.sounds == std::vector<int>{0x14} && actions.closeList && actions.selectShortcut == false && actions.backShortcut == false,
            "A: select sound, list closes, both shortcuts off");
    require(page.pending() == 7, "row 5 is Danish (index 7)");
    require(!page.selectionChanged(5).selectShortcut, "no shortcut change while a language is pending");
    require(!page.update(true, options).saveRequest && options[kLanguageOffset] == 1, "waits while the list is busy");
    actions = page.update(false, options);
    require(actions.saveRequest && actions.languageChanged == 7 && options[kLanguageOffset] == 7 && page.pending() == 0, "closed: options +1 set, saved");
    require(!page.update(false, options).saveRequest, "once");

    actions = page.leaveRequest();
    require(actions.closeList && actions.selectShortcut == false && actions.backShortcut == false, "leave request");

    bool rejected = false;
    try {
        page.a(kLanguageCount);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    require(rejected, "a row outside the table is rejected");
}
}

int main() {
    try {
        tables();
        flow();
    } catch (const std::exception& error) {
        std::cerr << "languages-page-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "languages-page-test: ok\n";
    return 0;
}
