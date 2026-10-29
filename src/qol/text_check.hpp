#pragma once

#include "assets/assets.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qol {

inline constexpr int namePlateWidth = 182;
inline constexpr int listBarWidth = 168;

struct MenuText {
    std::string source;
    std::string text;
    int box = 0;
};

enum class TextProblem { MissingGlyph, TooWide };

struct TextIssue {
    MenuText item;
    TextProblem problem;
    std::string detail;
    int width = 0;
};

using GlyphWidth = std::function<std::optional<int>(const std::string& character)>;

std::vector<std::string> splitCharacters(const std::string& utf8);
GlyphWidth glyphWidthOf(const chase::assets::Font& font, const std::vector<std::string>& extended);
int textWidth(const GlyphWidth& glyphWidth, const std::string& utf8);
std::vector<TextIssue> checkTexts(const std::vector<MenuText>& texts, const GlyphWidth& glyphWidth);

}
