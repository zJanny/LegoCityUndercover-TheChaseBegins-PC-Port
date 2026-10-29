#include "qol/text_check.hpp"

#include <set>
#include <stdexcept>

namespace qol {

std::vector<std::string> splitCharacters(const std::string& utf8) {
    std::vector<std::string> characters;
    for (std::size_t i = 0; i < utf8.size();) {
        const auto lead = static_cast<unsigned char>(utf8[i]);
        if (lead >= 0x80 && lead < 0xc0) throw std::invalid_argument("Text is not valid UTF-8");
        const std::size_t length = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
        if (i + length > utf8.size()) throw std::invalid_argument("Text is not valid UTF-8");
        characters.push_back(utf8.substr(i, length));
        i += length;
    }
    return characters;
}

GlyphWidth glyphWidthOf(const chase::assets::Font& font, const std::vector<std::string>& extended) {
    return [font, extended](const std::string& character) -> std::optional<int> {
        const auto placed = chase::assets::layoutText(font, character, extended);
        if (placed.size() != 1 || placed[0].glyph.width == 0) return std::nullopt;
        return placed[0].glyph.width;
    };
}

int textWidth(const GlyphWidth& glyphWidth, const std::string& utf8, int spacing) {
    int width = 0;
    int count = 0;
    for (const std::string& character : splitCharacters(utf8)) {
        const std::optional<int> advance = glyphWidth(character);
        if (!advance) continue;
        width += *advance;
        ++count;
    }
    return count > 0 ? width + (count - 1) * spacing : 0;
}

std::vector<TextIssue> checkTexts(const std::vector<MenuText>& texts, const GlyphWidth& glyphWidth) {
    std::vector<TextIssue> issues;
    for (const MenuText& item : texts) {
        std::set<std::string> missing;
        for (const std::string& character : splitCharacters(item.text))
            if (!glyphWidth(character)) missing.insert(character);
        const int width = textWidth(glyphWidth, item.text);
        if (!missing.empty()) {
            std::string detail;
            for (const std::string& character : missing) detail += (detail.empty() ? "" : " ") + character;
            issues.push_back({item, TextProblem::MissingGlyph, detail, width});
        }
        if (item.box > 0 && width > item.box)
            issues.push_back({item, TextProblem::TooWide, std::to_string(width) + " > " + std::to_string(item.box), width});
    }
    return issues;
}

}
