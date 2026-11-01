#include "runtime/qol_game_wiring.hpp"

#include "runtime/options_page.hpp"
#include "runtime/qol_game_options.hpp"
#include "save/save.hpp"
#include "save/save_writer.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

namespace chase::runtime {
namespace {

save::SaveImage openedImage(const QolGameSetup& setup, const std::string& target) {
    if (!setup.loadFile.empty()) {
        const save::LoadResult loaded = save::load(setup.loadFile);
        if (!loaded.found) throw std::runtime_error("save file not found: " + setup.loadFile);
        if (loaded.status != save::Status::Ok || !loaded.image) throw std::runtime_error("not a valid save file: " + setup.loadFile);
        return *loaded.image;
    }
    if (!target.empty()) {
        const save::LoadResult existing = save::load(target);
        if (existing.found && existing.status == save::Status::Ok && existing.image) return *existing.image;
    }
    save::SaveImage image;
    image.resetOptions(std::uint8_t(setup.language));
    image.seal();
    return image;
}

std::string writeTarget(const QolGameSetup& setup) {
    if (!setup.saveFile.empty()) return setup.saveFile;
    try {
        return save::defaultSavePath();
    } catch (const std::exception& error) {
        std::cout << "settings menu: saving disabled (" << error.what() << ")\n";
        return std::string();
    }
}

class QolGamePort final : public qol::GamePort {
public:
    explicit QolGamePort(const QolGameSetup& setup)
        : setGains_(setup.setGains), startLanguage_(setup.language), target_(writeTarget(setup)), image_(openedImage(setup, target_)),
          writer_(target_, image_, 0, std::nullopt), game_(page_, options_.data(), live_) {
        std::copy(image_.options(), image_.options() + save::kOptionsSize, options_.begin());
        live_ = {std::uint8_t(setup.soundVolume), std::uint8_t(setup.musicVolume)};
        loadingScreenVolumes(options_.data(), live_);
    }

    int sfxVolume() const override { return game_.sfxVolume(); }
    int musicVolume() const override { return game_.musicVolume(); }
    bool invertCamera() const override { return game_.invertCamera(); }
    int language() const override { return game_.language(); }
    void setSfxVolume(int volume) override { game_.setSfxVolume(volume); }
    void setMusicVolume(int volume) override { game_.setMusicVolume(volume); }
    void setInvertCamera(bool invert) override { game_.setInvertCamera(invert); }
    void openLanguages() override { game_.openLanguages(); }
    int startLanguage() const override { return startLanguage_; }

    void applied() override {
        const std::optional<float> sound = game_.takeSoundGain(), music = game_.takeMusicGain();
        if ((sound || music) && setGains_) setGains_(sound, music);
        if (game_.takePageRequest() == OptionsPageRequest::Languages)
            std::cout << "settings menu: the Languages page is requested (not shown in the settings menu yet, QOLSET-S6)\n";
    }

    void left() override {
        if (!game_.saveNeeded()) return;
        const std::size_t slot = optionsSlot(options_.data()).value_or(0);
        const bool stored = writer_.storeOptions(options_.data());
        std::cout << "settings menu: options saved" << (stored ? " to " + writer_.path() : std::string(" (writing disabled)")) << ", slot " << slot
                  << " sound volume " << unsigned(options_[4 + slot]) << ", music volume " << unsigned(options_[6 + slot]) << ", invert camera "
                  << ((options_[3] & kInvertCameraBit) ? "on" : "off") << '\n';
        page_.saveDone();
    }

private:
    std::function<void(std::optional<float>, std::optional<float>)> setGains_;
    int startLanguage_;
    std::string target_;
    save::SaveImage image_;
    save::SaveRequestWriter writer_;
    OptionsPage page_;
    std::array<std::uint8_t, save::kOptionsSize> options_{};
    OptionVolumes live_;
    QolGameOptions game_;
};

}

std::function<std::unique_ptr<qol::GamePort>()> qolGamePortFactory(QolGameSetup setup) {
    return [setup = std::move(setup)]() -> std::unique_ptr<qol::GamePort> { return std::make_unique<QolGamePort>(setup); };
}

}
