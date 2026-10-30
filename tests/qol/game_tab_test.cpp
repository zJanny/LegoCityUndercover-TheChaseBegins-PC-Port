// Spec: docs/design/qol-settings.md entry 2026-10-28T09:32:07 (Game tab mirrors SFX volume, Music volume, Invert camera, Language;
// same values, same save, in sync with the original Options page).
// Ranges and steps from the port's Options page (src/runtime/options_page.hpp): volumes 0..10 in steps of 1
// (kOptionsMaxVolume = 10, OptionsPage::step), invert camera a yes/no toggle, languages 1..9 (src/save/save.hpp Language).
// Defaults sound 10, music 8 (kDefaultSoundVolume, kDefaultMusicVolume). Cases written by 20261029-qolset-C, 2026-10-29.
// The language row opens the Languages page and never cycles: the user's decision of 2026-10-29, relayed in the brief of 20261029-qolwire-B.
#include "qol/game_tab.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

struct Fake : qol::GameOptions {
    int sfx = 10, music = 8, lang = 1;
    bool invert = false;
    int writes = 0, languagePages = 0;
    int sfxVolume() const override { return sfx; }
    int musicVolume() const override { return music; }
    bool invertCamera() const override { return invert; }
    int language() const override { return lang; }
    void setSfxVolume(int v) override { sfx = v; ++writes; }
    void setMusicVolume(int v) override { music = v; ++writes; }
    void setInvertCamera(bool v) override { invert = v; ++writes; }
    void openLanguages() override { ++languagePages; }
};

using qol::GameResult;
using qol::GameRow;

void readsThroughTheInterface() {
    Fake fake;
    qol::GameTab tab(fake);
    require(tab.value(GameRow::SfxVolume) == 10 && tab.value(GameRow::MusicVolume) == 8, "default volumes");
    require(tab.value(GameRow::InvertCamera) == 0 && tab.valueText(GameRow::InvertCamera) == "Off", "invert off");
    require(tab.value(GameRow::Language) == 1, "language");
    fake.sfx = 3; fake.music = 0; fake.invert = true; fake.lang = 9;
    require(tab.value(GameRow::SfxVolume) == 3 && tab.value(GameRow::MusicVolume) == 0, "original page change seen by the tab");
    require(tab.valueText(GameRow::InvertCamera) == "On" && tab.value(GameRow::Language) == 9, "invert and language seen by the tab");
    require(tab.valueText(GameRow::SfxVolume) == "3", "volume text");
}

void changesReachTheOptions() {
    Fake fake;
    qol::GameTab tab(fake);
    require(tab.step(GameRow::SfxVolume, -1) == GameResult::Changed && fake.sfx == 9, "sfx down");
    require(tab.step(GameRow::MusicVolume, 1) == GameResult::Changed && fake.music == 9, "music up");
    require(tab.activate(GameRow::InvertCamera) == GameResult::Changed && fake.invert, "invert on");
    require(tab.activate(GameRow::InvertCamera) == GameResult::Changed && !fake.invert, "invert off");
    require(tab.activate(GameRow::Language) == GameResult::OpenedLanguages && fake.languagePages == 1 && fake.lang == 1, "language row opens the Languages page and does not cycle");
    require(tab.setValue(GameRow::SfxVolume, 0) == GameResult::Changed && fake.sfx == 0, "sfx to 0");
}

void rangesAndRejections() {
    Fake fake;
    qol::GameTab tab(fake);
    require(tab.step(GameRow::SfxVolume, 1) == GameResult::Ignored && fake.sfx == 10, "no step above 10");
    fake.music = 0;
    require(tab.step(GameRow::MusicVolume, -1) == GameResult::Ignored && fake.music == 0, "no step below 0");
    require(tab.step(GameRow::InvertCamera, 1) == GameResult::Changed && fake.invert && fake.writes == 1, "invert flips on Right");
    require(tab.step(GameRow::InvertCamera, -1) == GameResult::Changed && !fake.invert && fake.writes == 2, "invert flips on Left");
    fake.writes = 0;
    require(tab.step(GameRow::Language, 1) == GameResult::Ignored, "language has no step");
    require(tab.activate(GameRow::SfxVolume) == GameResult::Ignored, "volume has no activate");
    require(tab.step(GameRow::SfxVolume, 0) == GameResult::Rejected, "step 0 rejected");
    require(tab.step(GameRow::SfxVolume, 2) == GameResult::Rejected, "step 2 rejected");
    require(tab.setValue(GameRow::SfxVolume, 11) == GameResult::Rejected, "volume 11 rejected");
    require(tab.setValue(GameRow::MusicVolume, -1) == GameResult::Rejected, "volume -1 rejected");
    require(tab.setValue(GameRow::InvertCamera, 2) == GameResult::Rejected, "invert 2 rejected");
    require(tab.setValue(GameRow::Language, 5) == GameResult::Rejected, "the language row is read only");
    require(fake.writes == 0, "rejections and ignored events write nothing");
}

void labels() {
    require(std::string(qol::gameRowLabel(GameRow::SfxVolume)) == "SFX volume", "label sfx");
    require(std::string(qol::gameRowLabel(GameRow::MusicVolume)) == "Music volume", "label music");
    require(std::string(qol::gameRowLabel(GameRow::InvertCamera)) == "Invert camera", "label invert");
    require(std::string(qol::gameRowLabel(GameRow::Language)) == "Language", "label language");
}

}

int main() {
    readsThroughTheInterface();
    changesReachTheOptions();
    rangesAndRejections();
    labels();
    return failures == 0 ? 0 : 1;
}
