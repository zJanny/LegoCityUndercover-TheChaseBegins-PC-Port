#pragma once

#include "qol/game_tab.hpp"
#include "qol/ui_nav.hpp"

#include <optional>
#include <string>
#include <vector>

namespace qol::ui {

inline constexpr int volumeSteps = maxVolume;
inline constexpr int volumeDisplayScale = 10;
inline constexpr int defaultSfxVolume = 10;
inline constexpr int defaultMusicVolume = 8;
inline constexpr int gameLanguageCount = lastLanguage - firstLanguage + 1;

extern const char* const gameResetQuestion;

struct GamePageResult {
    bool changed = false;
    std::optional<int> languageChosen;
    std::optional<std::string> needsConfirm;
};

class GamePage {
public:
    GamePage(GameOptions& options, std::vector<std::string> languageNames,
        int startLanguage);

    std::vector<RowSpec> rows() const;
    DropdownInfo dropdown(int row) const;
    GamePageResult handle(const Action& action);
    GamePageResult reset();

    int language() const { return language_; }

private:
    GamePageResult changeValue(const Action& action);
    GamePageResult chooseLanguage(int language);

    GameTab tab_;
    std::vector<std::string> languageNames_;
    int startLanguage_;
    int language_;
};

}
