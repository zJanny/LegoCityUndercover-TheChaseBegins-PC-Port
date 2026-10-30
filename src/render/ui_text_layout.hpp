#pragma once
// Glyph atlas and text layout of the UI font on the CPU; no OpenGL. Pixel sizes are rounded to whole pixels.
#include "render/ui_font.hpp"
#include <cstdint>
#include <map>
#include <string_view>
#include <vector>

namespace chase::render {

struct UiTextColour {
    std::uint8_t r = 255, g = 255, b = 255, a = 255;
};

// Window pixels, y counting down from the top.
struct UiClip {
    int x = 0, y = 0, width = 0, height = 0;
};

struct UiGlyphQuad {
    float x0, y0, x1, y1;
    float u0, v0, u1, v1;
};

class UiGlyphAtlas {
public:
    struct Entry {
        int x = 0, y = 0, width = 0, height = 0, offsetX = 0, offsetY = 0;
    };

    explicit UiGlyphAtlas(int size = 2048);
    const Entry& entry(char32_t codepoint, float pixelSize);
    int size() const { return size_; }
    const std::vector<std::uint8_t>& pixels() const { return pixels_; }
    bool takeDirty();

private:
    int size_, cursorX_ = 1, cursorY_ = 1, rowHeight_ = 0;
    bool dirty_ = false;
    std::vector<std::uint8_t> pixels_;
    std::map<std::uint64_t, Entry> entries_;
};

// The text starts at x with the top of its line box at y; a null clip draws everything.
std::vector<UiGlyphQuad> layoutUiText(UiGlyphAtlas& atlas, std::string_view utf8, float pixelSize, float x, float y, const UiClip* clip);

bool clipUiQuad(UiGlyphQuad& quad, const UiClip& clip);

}
