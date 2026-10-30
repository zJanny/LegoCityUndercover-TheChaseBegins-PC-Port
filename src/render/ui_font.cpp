#include "render/ui_font.hpp"
#include "stb_truetype.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace chase::render {

extern const unsigned char kUiFontData[];

namespace {

const stbtt_fontinfo& font() {
    static const stbtt_fontinfo info = [] {
        stbtt_fontinfo created{};
        if (!stbtt_InitFont(&created, kUiFontData, stbtt_GetFontOffsetForIndex(kUiFontData, 0))) throw std::runtime_error("bundled UI font does not load");
        return created;
    }();
    return info;
}

float scaleFor(float pixelSize) { return stbtt_ScaleForMappingEmToPixels(&font(), std::max(1.0f, std::round(pixelSize))); }

}

std::vector<char32_t> decodeUtf8(std::string_view text) {
    std::vector<char32_t> out;
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        int length = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 0;
        char32_t value = length == 1 ? lead : length == 2 ? lead & 0x1F : length == 3 ? lead & 0x0F : lead & 0x07;
        bool valid = length != 0 && i + static_cast<std::size_t>(length) <= text.size();
        for (int k = 1; valid && k < length; ++k) {
            const unsigned char next = static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]);
            if ((next & 0xC0) != 0x80) valid = false;
            else value = (value << 6) | (next & 0x3F);
        }
        const char32_t minimum[5] = {0, 0, 0x80, 0x800, 0x10000};
        if (valid && (value < minimum[length] || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))) valid = false;
        if (!valid) {
            out.push_back(0xFFFD);
            ++i;
        } else {
            out.push_back(value);
            i += static_cast<std::size_t>(length);
        }
    }
    return out;
}

UiFontMetrics uiFontMetrics(float pixelSize) {
    int ascent = 0, descent = 0, gap = 0;
    stbtt_GetFontVMetrics(&font(), &ascent, &descent, &gap);
    const float scale = scaleFor(pixelSize);
    return {ascent * scale, -descent * scale, (ascent - descent + gap) * scale};
}

bool uiFontHasGlyph(char32_t codepoint) { return stbtt_FindGlyphIndex(&font(), static_cast<int>(codepoint)) != 0; }

float uiGlyphAdvance(char32_t codepoint, float pixelSize) {
    int advance = 0, bearing = 0;
    stbtt_GetCodepointHMetrics(&font(), static_cast<int>(codepoint), &advance, &bearing);
    return advance * scaleFor(pixelSize);
}

float uiKerning(char32_t left, char32_t right, float pixelSize) {
    return stbtt_GetCodepointKernAdvance(&font(), static_cast<int>(left), static_cast<int>(right)) * scaleFor(pixelSize);
}

float uiTextWidth(std::string_view utf8, float pixelSize) {
    const std::vector<char32_t> codepoints = decodeUtf8(utf8);
    float width = 0;
    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        width += uiGlyphAdvance(codepoints[i], pixelSize);
        if (i + 1 < codepoints.size()) width += uiKerning(codepoints[i], codepoints[i + 1], pixelSize);
    }
    return width;
}

UiGlyphBitmap rasteriseUiGlyph(char32_t codepoint, float pixelSize) {
    const float scale = scaleFor(pixelSize);
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetCodepointBitmapBox(&font(), static_cast<int>(codepoint), scale, scale, &x0, &y0, &x1, &y1);
    UiGlyphBitmap glyph;
    glyph.width = x1 - x0;
    glyph.height = y1 - y0;
    glyph.offsetX = x0;
    glyph.offsetY = y0;
    if (glyph.width > 0 && glyph.height > 0) {
        glyph.coverage.assign(static_cast<std::size_t>(glyph.width) * static_cast<std::size_t>(glyph.height), 0);
        stbtt_MakeCodepointBitmap(&font(), glyph.coverage.data(), glyph.width, glyph.height, glyph.width, scale, scale, static_cast<int>(codepoint));
    }
    return glyph;
}

}
