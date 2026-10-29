#include "qol/text_check.hpp"

#include <iostream>
#include <stdexcept>

namespace {

int failures = 0;

void require(bool ok, const char* what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

chase::assets::Font syntheticFont() {
    chase::assets::Font font;
    font.first = 0x20;
    font.lineHeight = 23;
    font.glyphs.resize(0x7f - 0x20 + 0x100);
    for (std::size_t i = 0; i < 0x7f - 0x20; ++i) font.glyphs[i].width = 8;
    font.glyphs['i' - 0x20].width = 4;
    font.glyphs['!' - 0x20].width = 0;
    font.glyphs[0x100 - 0x20 + 1].width = 9;
    return font;
}

}

int main() {
    const std::vector<std::string> extended{"\xd0\x96", "\xd0\xaf"};
    const qol::GlyphWidth glyphWidth = qol::glyphWidthOf(syntheticFont(), extended);

    require(qol::splitCharacters("a\xc3\xa7z\xd0\xaf").size() == 4, "UTF-8 split by character");
    bool threw = false;
    try { qol::splitCharacters("a\x80"); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "a stray continuation byte is rejected");
    threw = false;
    try { qol::splitCharacters("a\xc3"); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "a cut-off sequence is rejected");

    require(qol::textWidth(glyphWidth, "") == 0, "empty text has no width");
    require(qol::textWidth(glyphWidth, "a") == 8, "one glyph is its width");
    require(qol::textWidth(glyphWidth, "ai") == 8 + 4, "glyphs follow each other at the layout spacing 0");
    require(qol::textWidth(glyphWidth, "ai", 1) == 8 + 1 + 4, "an explicit spacing is added between glyphs");
    require(qol::textWidth(glyphWidth, "a!a") == 8 + 8, "a character without a glyph adds nothing");
    require(qol::textWidth(glyphWidth, "\xd0\xaf") == 9, "an extended character uses its atlas glyph");

    const std::vector<qol::MenuText> texts{
        {"fits", "aaaa", 32},
        {"exact", "aaaa", 32},
        {"too wide", "aaaaa", 32},
        {"no box", "aaaaaaaaaaaa", 0},
        {"missing", "a!b!", 0},
        {"unknown", "\xc2\xa3", 0},
        {"both", "aaaaa!", 32},
    };
    const auto issues = qol::checkTexts(texts, glyphWidth);
    require(issues.size() == 5, "five problems found");
    require(issues[0].item.source == "too wide" && issues[0].problem == qol::TextProblem::TooWide &&
            issues[0].width == 40 && issues[0].detail == "40 > 32", "too wide text reports width and box");
    require(issues[1].item.source == "missing" && issues[1].problem == qol::TextProblem::MissingGlyph &&
            issues[1].detail == "!", "missing glyph is named once");
    require(issues[2].item.source == "unknown" && issues[2].detail == "\xc2\xa3", "a sign outside the font is missing");
    require(issues[3].item.source == "both" && issues[3].problem == qol::TextProblem::MissingGlyph, "missing glyph first");
    require(issues[4].item.source == "both" && issues[4].problem == qol::TextProblem::TooWide, "then the width");
    return failures == 0 ? 0 : 1;
}
