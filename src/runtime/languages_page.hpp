#pragma once
// The Languages page of the pause screen (page vtable 0x364a68: enter 0x31dbd8, leave request 0x31dbac, A 0x31db18, selection changed
// 0x31db6c, B 0x31daf0, update 0x31df90) and its language list (table 0x3759b0, docs/re/ui-screens.md). No SDL dependency.
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr int kLanguageCount = 9;
// Table 0x3759b0 (24-byte entries read by 0x1ecc40, first byte the language index, ended by 0): the order of the rows.
constexpr std::array<std::uint8_t, kLanguageCount> kLanguageTable = {1, 2, 3, 4, 5, 7, 6, 9, 8};
// Gametext keys of the row names, by language index - 1 (switch 0x175d34 of 0x175d24).
constexpr std::array<std::uint32_t, kLanguageCount> kLanguageNameKeys = {0x4f11523f, 0x04ee198b, 0x952b154f, 0x04723177, 0xf7468743,
                                                                        0x363c0017, 0xf63a151e, 0xeecdb206, 0x3f283c62};
constexpr const char* kLanguageImageFolder = "Sprites/UI_StatusScreen/Languages";
// Flag pictures by language index - 1 (switch 0x31dc84 of 0x31dbd8).
constexpr std::array<const char*, kLanguageCount> kLanguageImages = {
    "UI_Language_UK.png",     "UI_Language_French.png", "UI_Language_Italian.png",    "UI_Language_German.png", "UI_Language_Spanish.png",
    "UI_Language_Dutch.png",  "UI_Language_Danish.png", "UI_Language_Portuguese.png", "UI_Language_Russian.png"};
constexpr int kLanguagesSoundSelect = 0x14, kLanguagesSoundBack = 0x11;
constexpr std::size_t kLanguageOffset = 1;

// What a page callback asks of the pause screen.
struct LanguagesPageActions {
    std::vector<int> sounds;                           // 0x22899c sound ids
    bool closeList = false;                            // 0x1e87d0(1, 0)
    std::optional<bool> backShortcut, selectShortcut;  // 0x228ad0(0, on) and 0x228ad0(1, on)
    bool optionsPage = false;                          // 0x217f88 with the Options page (pause +0x348)
    bool saveRequest = false;                          // 0x2400a0(0x1c34a4, 0x2b4c44, 0)
    std::optional<std::uint8_t> languageChanged;       // 0x1f440c(language), the new language index
};

class LanguagesPage {
public:
    // Enter 0x31dbd8: the list opens over the table's rows with no selection; Back on; no language is pending.
    LanguagesPageActions enter();
    // Selection changed 0x31db6c: while no language is pending, Select is shown when a row is selected (-1 = none).
    LanguagesPageActions selectionChanged(int selected);
    // A 0x31db18: the selected row's language becomes pending, the list closes, both shortcuts go off.
    LanguagesPageActions a(int selected);
    // B 0x31daf0: back sound and the Options page.
    LanguagesPageActions b();
    // Leave request 0x31dbac: the list closes, both shortcuts go off.
    LanguagesPageActions leaveRequest();
    // Update 0x31df90, with `listBusy` as 0x1fbedc: once the list has closed, options +1 takes the pending language, the language is changed
    // and the options are saved.
    LanguagesPageActions update(bool listBusy, std::uint8_t* options);

    std::uint8_t pending() const { return pending_; }

private:
    std::uint8_t pending_ = 0;
};

}
