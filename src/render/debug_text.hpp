#pragma once
// Own 5x7 bitmap font for the developer debug overlay (docs/debug-overlay.md); no game asset, no OpenGL.
#include <cstdint>
#include <string>
#include <vector>

namespace chase::render {

inline constexpr int kDebugGlyphWidth = 5, kDebugGlyphHeight = 7, kDebugGlyphAdvance = 6, kDebugLineAdvance = 9, kDebugPadding = 3;

struct DebugTextImage {
    int width = 0, height = 0;
    std::vector<std::uint8_t> rgba;   // row 0 on top
};

// Glyph rows (bit 4 = leftmost column) for a character; lower case is shown as upper case, unknown characters as '?'.
const std::uint8_t* debugGlyph(char character);

// White text on a translucent dark panel, every font pixel `scale` pixels wide.
DebugTextImage rasteriseDebugText(const std::vector<std::string>& lines, int scale);

}
