// Glyph coverage and box fit of every settings-menu text against the real front-end font.
// Skips (exit 77) without the extracted RomFS. Usage: qol-text-check-real-test FIB CSV
#include "qol/menu_texts.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace chase;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

const char* const sdlKeyNames[] = {
#include "../data/qol_sdl_key_names.inc"
};

assets::Bytes readFile(assets::FuseArchive& archive, const std::string& path) {
    const assets::FuseEntry* entry = archive.find(path);
    if (!entry) throw std::runtime_error("missing archive file " + path);
    return archive.read(*entry);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-text-check-real-test FIB CSV\n";
        return 2;
    }
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
        std::cout << "SKIP: game data not extracted\n";
        return 77;
    }
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    const assets::Font font = assets::parseFont(readFile(archive, "fonts/frontendfont.tnf"));
    const auto extended = assets::extendedCharacters(readFile(archive, "localisation/extended_chars_utf8.txt"));
    const auto glyphWidth = qol::glyphWidthOf(font, extended);

    qol::MenuTextInputs inputs;
    for (const char* name : sdlKeyNames) inputs.keyNames.push_back(name);
    inputs.languageNames = {"Deutsch", "Fran\xc3\xa7" "ais", "Nederlands", "English", "Italiano",
        "Portugu\xc3\xaas", "Dansk", "Espa\xc3\xb1ol", "Polski", "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9"};
    inputs.resolutions = {"400x240", "1280x720", "1920x1080", "2560x1440", "3840x2160", "7680x4320"};

    const auto gametext = assets::parseLocalisation(readFile(archive, "localisation/gametext_uk.loc"));
    const std::map<std::uint32_t, std::string> languageKeys{
        {0x04723177, "Deutsch"}, {0x04ee198b, "Fran\xc3\xa7" "ais"}, {0x363c0017, "Nederlands"},
        {0x4f11523f, "English"}, {0x952b154f, "Italiano"}, {0xeecdb206, "Portugu\xc3\xaas"},
        {0xf63a151e, "Dansk"}, {0xf7468743, "Espa\xc3\xb1ol"}};
    for (const auto& [key, name] : languageKeys) {
        const auto found = gametext.find(key);
        require(found != gametext.end() && assets::decodeText(found->second, extended) == name,
            "gametext_uk language name " + name);
    }

    const auto texts = qol::menuTexts(inputs);
    require(texts.size() == 335, "number of checked texts");
    const auto issues = qol::checkTexts(texts, glyphWidth);
    for (const qol::TextIssue& issue : issues) require(false, "text does not fit: " + issue.item.text + " " + issue.detail);

    // Widths at the atlas size with the layout text spacing 0 (0x2b4dc4, HUD-75), recorded 2026-10-29 by 20261029-qolmenu2-C with
    // qol-text-check-real-test over fonts/frontendfont.tnf of the extracted archive (the three front-end fonts are identical).
    const std::map<std::string, int> widths{
        {"PC Settings", 79}, {"Quality of life", 93}, {"Window mode: windowed", 169},
        {"Window mode: fullscreen", 171}, {"Language: Nederlands", 149}, {"Resolution: 7680x4320", 153},
        {"Master: Improved", 123}, {"DebugOverlay", 92}, {"Keypad MemSubtract", 145}, {"righttrigger", 86}};
    for (const auto& [text, width] : widths) {
        require(qol::textWidth(glyphWidth, text) == width, "width of " + text);
    }
    require(qol::textWidth(glyphWidth, "Language: \xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9") == 131,
        "width of Language: Russian name");

    int widestKey = 0;
    for (const char* name : sdlKeyNames) widestKey = std::max(widestKey, qol::textWidth(glyphWidth, name));
    require(widestKey == 145, "widest SDL key name");
    return failures == 0 ? 0 : 1;
}
