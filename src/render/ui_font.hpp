#pragma once
// Bundled UI font (Noto Sans Regular, third_party/fonts) measured and rasterised through stb_truetype; no OpenGL.
// Pixel sizes are em sizes in window pixels.
#include <cstdint>
#include <string_view>
#include <vector>

namespace chase::render {

struct UiFontMetrics {
    float ascent = 0, descent = 0, lineHeight = 0;
};

std::vector<char32_t> decodeUtf8(std::string_view text);

UiFontMetrics uiFontMetrics(float pixelSize);
bool uiFontHasGlyph(char32_t codepoint);
float uiGlyphAdvance(char32_t codepoint, float pixelSize);
float uiKerning(char32_t left, char32_t right, float pixelSize);
float uiTextWidth(std::string_view utf8, float pixelSize);

struct UiGlyphBitmap {
    int width = 0, height = 0, offsetX = 0, offsetY = 0;
    std::vector<std::uint8_t> coverage;
};

UiGlyphBitmap rasteriseUiGlyph(char32_t codepoint, float pixelSize);

}
