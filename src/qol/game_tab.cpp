#include "qol/game_tab.hpp"

namespace qol {

const char* gameRowLabel(GameRow row) {
    switch (row) {
    case GameRow::SfxVolume: return "SFX volume";
    case GameRow::MusicVolume: return "Music volume";
    case GameRow::InvertCamera: return "Invert camera";
    case GameRow::Language: return "Language";
    }
    return "";
}

int GameTab::value(GameRow row) const {
    switch (row) {
    case GameRow::SfxVolume: return options_.sfxVolume();
    case GameRow::MusicVolume: return options_.musicVolume();
    case GameRow::InvertCamera: return options_.invertCamera() ? 1 : 0;
    case GameRow::Language: return options_.language();
    }
    return 0;
}

std::string GameTab::valueText(GameRow row) const {
    if (row == GameRow::InvertCamera) return value(row) ? "On" : "Off";
    return std::to_string(value(row));
}

GameResult GameTab::setValue(GameRow row, int value) {
    switch (row) {
    case GameRow::SfxVolume:
    case GameRow::MusicVolume:
        if (value < 0 || value > maxVolume) return GameResult::Rejected;
        if (row == GameRow::SfxVolume) options_.setSfxVolume(value);
        else options_.setMusicVolume(value);
        return GameResult::Changed;
    case GameRow::InvertCamera:
        if (value != 0 && value != 1) return GameResult::Rejected;
        options_.setInvertCamera(value == 1);
        return GameResult::Changed;
    case GameRow::Language:
        return GameResult::Rejected;
    }
    return GameResult::Rejected;
}

GameResult GameTab::step(GameRow row, int direction) {
    if (direction != -1 && direction != 1) return GameResult::Rejected;
    if (row == GameRow::InvertCamera) return setValue(row, value(row) ? 0 : 1);
    if (row == GameRow::Language) return GameResult::Ignored;
    const int next = value(row) + direction;
    if (next < 0 || next > maxVolume) return GameResult::Ignored;
    return setValue(row, next);
}

GameResult GameTab::activate(GameRow row) {
    if (row == GameRow::Language) {
        options_.openLanguages();
        return GameResult::OpenedLanguages;
    }
    if (row != GameRow::InvertCamera) return GameResult::Ignored;
    return setValue(row, value(row) ? 0 : 1);
}

}
