#include "render/ui_text_layout.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace chase::render;
namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }
}
int main() {
    try {
        const std::vector<char32_t> mixed = decodeUtf8("a\xc3\xa7\xd0\xa0\xe2\x82\xac\xf0\x9f\x98\x80");
        require(mixed.size() == 5 && mixed[1] == 0xE7 && mixed[2] == 0x420 && mixed[3] == 0x20AC && mixed[4] == 0x1F600, "utf-8 decoding");
        require(decodeUtf8("\xc3").size() == 1 && decodeUtf8("\xc3")[0] == 0xFFFD, "truncated sequence");
        require(decodeUtf8("\xc0\x80").size() == 2 && decodeUtf8("\xc0\x80")[0] == 0xFFFD, "overlong sequence");

        const char* names[] = {"Deutsch", "Fran\xc3\xa7" "ais", "Nederlands", "English", "Italiano",
                               "Portugu\xc3\xaas", "Dansk", "Espa\xc3\xb1ol", "Polski", "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9"};
        for (const char* name : names)
            for (const char32_t codepoint : decodeUtf8(name)) require(uiFontHasGlyph(codepoint), std::string("glyph missing in ") + name);
        for (char32_t codepoint = 0x20; codepoint < 0x7F; ++codepoint) require(uiFontHasGlyph(codepoint), "ASCII glyph");
        for (char32_t codepoint = 0xA0; codepoint <= 0xFF; ++codepoint) require(uiFontHasGlyph(codepoint), "Latin-1 glyph");
        for (char32_t codepoint = 0x410; codepoint <= 0x44F; ++codepoint) require(uiFontHasGlyph(codepoint), "Cyrillic glyph");
        require(uiFontHasGlyph(0x401) && uiFontHasGlyph(0x451), "Cyrillic Io");

        require(uiTextWidth("", 20) == 0, "empty text");
        require(near(uiTextWidth("Hello", 40), 2 * uiTextWidth("Hello", 20)) || std::fabs(uiTextWidth("Hello", 40) - 2 * uiTextWidth("Hello", 20)) < 2.0f,
                "width scales with the pixel size");
        require(uiTextWidth("AV", 100) < uiGlyphAdvance('A', 100) + uiGlyphAdvance('V', 100), "kerning narrows AV");
        require(uiKerning('A', 'V', 100) < 0 && uiKerning('A', 'A', 100) == 0, "kerning pairs");
        require(uiTextWidth("Wide", 20) > uiTextWidth("i", 20), "width orders");
        require(uiFontMetrics(20).ascent > 0 && uiFontMetrics(20).descent > 0 && uiFontMetrics(20).lineHeight >= uiFontMetrics(20).ascent + uiFontMetrics(20).descent - 0.01f,
                "metrics");
        require(near(uiTextWidth("x", 20.4f), uiTextWidth("x", 20.0f)), "pixel sizes round to whole pixels");

        UiGlyphAtlas atlas(256);
        const UiGlyphAtlas::Entry& space = atlas.entry(' ', 20);
        require(space.width == 0, "space has no bitmap");
        const UiGlyphAtlas::Entry& letter = atlas.entry('H', 20);
        require(letter.width > 0 && letter.height > 10 && atlas.takeDirty() && !atlas.takeDirty(), "H bitmap and dirty flag");
        require(&atlas.entry('H', 20) == &letter, "entries are cached");
        const UiGlyphAtlas::Entry& other = atlas.entry('H', 21);
        require(other.height >= letter.height && (other.x != letter.x || other.y != letter.y), "each pixel size has its own entry");
        bool inked = false;
        for (int y = 0; y < letter.height; ++y)
            for (int x = 0; x < letter.width; ++x) inked = inked || atlas.pixels()[std::size_t(letter.y + y) * 256 + std::size_t(letter.x + x)] > 200;
        require(inked, "H has solid pixels");
        require(atlas.entry(0x1F600, 20).width == atlas.entry(0, 20).width, "unknown codepoints use the missing glyph");

        const auto quads = layoutUiText(atlas, "Hi", 20, 10, 5, nullptr);
        require(quads.size() == 2 && quads[0].x1 <= quads[1].x0 + 1 && quads[0].y0 >= 5 && quads[0].y0 < 5 + 20, "two glyph quads in the line box");
        require(layoutUiText(atlas, "   ", 20, 0, 0, nullptr).empty(), "spaces draw nothing");
        const UiClip clip{12, 0, 5, 100};
        const auto clipped = layoutUiText(atlas, "Hi", 20, 10, 5, &clip);
        require(!clipped.empty(), "clip keeps part of the text");
        for (const UiGlyphQuad& q : clipped) require(q.x0 >= 12 && q.x1 <= 17, "clipped quads stay inside");
        UiGlyphQuad quad{0, 0, 10, 10, 0, 0, 1, 1};
        require(clipUiQuad(quad, {5, 0, 100, 100}) && near(quad.x0, 5) && near(quad.u0, 0.5f) && near(quad.u1, 1), "clip moves the uv with the edge");
        quad = {0, 0, 10, 10, 0, 0, 1, 1};
        require(!clipUiQuad(quad, {20, 20, 5, 5}), "fully clipped quad is dropped");

        // Provenance: 20261030-qolui-A, 2026-10-30, widths printed by this test over third_party/fonts/NotoSans-Regular.ttf (sha256 pinned by qol-bundled-font).
        require(near(uiTextWidth("Hello", 20), 48.52f) && near(uiTextWidth("AV", 100), 119.9f) && near(uiKerning('A', 'V', 100), -4.0f), "pinned widths");
        std::cout << "UI text tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
