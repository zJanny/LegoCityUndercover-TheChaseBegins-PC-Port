#include "qol/ui_page_game.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace qol::ui {

const char* const gameResetQuestion = "Reset the Game settings?";

namespace {

enum Row { SfxRow, MusicRow, InvertRow, LanguageRow, ResetRow };

GameRow gameRowOf(int row) {
    return row == SfxRow ? GameRow::SfxVolume : GameRow::MusicVolume;
}

bool languageValid(int language) {
    return language >= firstLanguage && language <= lastLanguage;
}

}

GamePage::GamePage(GameOptions& options, std::vector<std::string> languageNames, int startLanguage)
    : tab_(options), languageNames_(std::move(languageNames)),
      startLanguage_(startLanguage), language_(options.language()) {
    if (static_cast<int>(languageNames_.size()) != gameLanguageCount)
        throw std::invalid_argument("the Game page needs " + std::to_string(gameLanguageCount) + " language names");
    if (!languageValid(startLanguage_))
        throw std::invalid_argument("start language out of range: " + std::to_string(startLanguage_));
    if (!languageValid(language_))
        throw std::invalid_argument("language out of range: " + std::to_string(language_));
}

std::vector<RowSpec> GamePage::rows() const {
    std::vector<RowSpec> out;
    for (GameRow row : {GameRow::SfxVolume, GameRow::MusicVolume}) {
        const int volume = tab_.value(row);
        out.push_back({gameRowLabel(row), RowKind::Slider, std::to_string(volume * volumeDisplayScale),
            static_cast<double>(volume) / volumeSteps, true});
    }
    out.push_back({gameRowLabel(GameRow::InvertCamera), RowKind::Toggle,
        tab_.valueText(GameRow::InvertCamera), 0, true});
    out.push_back({gameRowLabel(GameRow::Language), RowKind::Selector,
        languageNames_[static_cast<std::size_t>(language_ - firstLanguage)], 0, true});
    out.push_back({"Reset to defaults", RowKind::Button, "", 0, true});
    return out;
}

DropdownInfo GamePage::dropdown(int) const { return {}; }

GamePageResult GamePage::chooseLanguage(int language) {
    if (language == language_) return {};
    language_ = language;
    GamePageResult result;
    result.languageChosen = language_;
    return result;
}

GamePageResult GamePage::changeValue(const Action& action) {
    switch (action.index) {
    case SfxRow:
    case MusicRow: {
        const GameRow row = gameRowOf(action.index);
        const int volume = static_cast<int>(std::lround(action.fraction * volumeSteps));
        if (volume == tab_.value(row)) return {};
        GamePageResult result;
        result.changed = tab_.setValue(row, volume) == GameResult::Changed;
        return result;
    }
    case InvertRow: {
        GamePageResult result;
        result.changed = tab_.step(GameRow::InvertCamera, action.direction < 0 ? -1 : 1) == GameResult::Changed;
        return result;
    }
    case LanguageRow: {
        if (action.direction == 0) return {};
        const int offset = (language_ - firstLanguage + (action.direction < 0 ? -1 : 1) + gameLanguageCount) % gameLanguageCount;
        return chooseLanguage(firstLanguage + offset);
    }
    default: return {};
    }
}

GamePageResult GamePage::handle(const Action& action) {
    if (action.kind == ActionKind::ValueChanged) return changeValue(action);
    if (action.kind == ActionKind::Activate && action.index == ResetRow) {
        GamePageResult result;
        result.needsConfirm = gameResetQuestion;
        return result;
    }
    return {};
}

GamePageResult GamePage::reset() {
    GamePageResult result;
    const std::pair<GameRow, int> defaults[] = {
        {GameRow::SfxVolume, defaultSfxVolume},
        {GameRow::MusicVolume, defaultMusicVolume},
        {GameRow::InvertCamera, 0},
    };
    for (const auto& [row, value] : defaults) {
        if (tab_.value(row) == value) continue;
        tab_.setValue(row, value);
        result.changed = true;
    }
    result.languageChosen = chooseLanguage(startLanguage_).languageChosen;
    return result;
}

}
