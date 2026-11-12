#include "runtime/qol_game_options.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr std::size_t kFlagsOffset = 3, kSoundOffset = 4, kMusicOffset = 6;
constexpr int kFirstLanguage = 1, kLastLanguage = 9;

void requireVolume(int volume) {
    if (volume < 0 || volume > kOptionsMaxVolume) throw std::out_of_range("volume out of range 0 to 10: " + std::to_string(volume));
}

}

QolGameOptions::QolGameOptions(OptionsPage& page, std::uint8_t* options, OptionVolumes& live) : page_(page), options_(options), live_(live) {}

int QolGameOptions::sfxVolume() const {
    const std::optional<std::size_t> slot = optionsSlot(options_);
    return slot ? options_[kSoundOffset + *slot] : live_.sound;
}

int QolGameOptions::musicVolume() const {
    const std::optional<std::size_t> slot = optionsSlot(options_);
    return slot ? options_[kMusicOffset + *slot] : live_.music;
}

bool QolGameOptions::invertCamera() const { return (options_[kFlagsOffset] & kInvertCameraBit) != 0; }

int QolGameOptions::language() const {
    const int language = options_[kLanguageOffset];
    if (language < kFirstLanguage || language > kLastLanguage) throw std::out_of_range("language out of range 1 to 9: " + std::to_string(language));
    return language;
}

void QolGameOptions::collect(const OptionsPageActions& actions) {
    if (actions.soundGain) soundGain_ = actions.soundGain;
    if (actions.musicGain) musicGain_ = actions.musicGain;
    if (actions.page != OptionsPageRequest::None) pageRequest_ = actions.page;
}

void QolGameOptions::setVolume(OptionsRow row, int volume) {
    requireVolume(volume);
    const auto current = [&] { return row == OptionsRow::SoundVolume ? sfxVolume() : musicVolume(); };
    while (current() != volume) {
        const bool up = current() < volume;
        const int before = current();
        collect(page_.update(int(row), up, !up, options_, live_));
        if (current() == before) throw std::logic_error("volume does not move: live byte and slot byte differ");
    }
}

void QolGameOptions::setSfxVolume(int volume) { setVolume(OptionsRow::SoundVolume, volume); }

void QolGameOptions::setMusicVolume(int volume) {
    setVolume(OptionsRow::MusicVolume, volume);
    restoreSoundVolume(options_, live_);
    if (soundGain_) soundGain_ = gainOf(live_.sound);
}

void QolGameOptions::setInvertCamera(bool invert) {
    if (invert != invertCamera()) collect(page_.update(int(OptionsRow::InvertCamera), true, false, options_, live_));
}

void QolGameOptions::openLanguages() { collect(page_.a(int(OptionsRow::Language), options_)); }

void QolGameOptions::chooseLanguage(int language) {
    if (language < kFirstLanguage || language > kLastLanguage) throw std::out_of_range("language out of range 1 to 9: " + std::to_string(language));
    const auto row = std::find(kLanguageTable.begin(), kLanguageTable.end(), std::uint8_t(language));
    languages_.enter();
    languages_.a(int(row - kLanguageTable.begin()));
    if (languages_.update(false, options_).languageChanged) languageChanged_ = true;
}

void QolGameOptions::saved() {
    languageChanged_ = false;
    page_.saveDone();
}

OptionsPageRequest QolGameOptions::takePageRequest() {
    const OptionsPageRequest request = pageRequest_;
    pageRequest_ = OptionsPageRequest::None;
    return request;
}

std::optional<float> QolGameOptions::takeSoundGain() {
    const std::optional<float> gain = soundGain_;
    soundGain_.reset();
    return gain;
}

std::optional<float> QolGameOptions::takeMusicGain() {
    const std::optional<float> gain = musicGain_;
    musicGain_.reset();
    return gain;
}

}
