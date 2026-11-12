#pragma once
#include "qol/game_tab.hpp"
#include "runtime/languages_page.hpp"
#include "runtime/options_page.hpp"

#include <cstdint>

namespace chase::runtime {

class QolGameOptions final : public qol::GameOptions {
public:
    QolGameOptions(OptionsPage& page, std::uint8_t* options, OptionVolumes& live);

    int sfxVolume() const override;
    int musicVolume() const override;
    bool invertCamera() const override;
    int language() const override;
    void setSfxVolume(int volume) override;
    void setMusicVolume(int volume) override;
    void setInvertCamera(bool invert) override;
    void openLanguages() override;
    void chooseLanguage(int language);

    OptionsPageRequest takePageRequest();
    std::optional<float> takeSoundGain();
    std::optional<float> takeMusicGain();
    bool saveNeeded() const { return page_.changed() || languageChanged_; }
    void saved();

private:
    void setVolume(OptionsRow row, int volume);
    void collect(const OptionsPageActions& actions);

    OptionsPage& page_;
    std::uint8_t* options_;
    OptionVolumes& live_;
    LanguagesPage languages_;
    bool languageChanged_ = false;
    OptionsPageRequest pageRequest_ = OptionsPageRequest::None;
    std::optional<float> soundGain_, musicGain_;
};

}
