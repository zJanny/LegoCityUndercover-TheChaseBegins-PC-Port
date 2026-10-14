#include "render/debug_text.hpp"
#include <iostream>
#include <stdexcept>
using namespace chase::render;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
const std::uint8_t* pixelAt(const DebugTextImage& image, int x, int y) {
    return &image.rgba[(std::size_t(y) * std::size_t(image.width) + std::size_t(x)) * 4];
}
}
int main() {
    try {
        require(debugGlyph('a') == debugGlyph('A'), "lower case maps to upper case");
        require(debugGlyph('~') == debugGlyph('?'), "unknown characters show as a question mark");
        const DebugTextImage image = rasteriseDebugText({"I", "-"}, 1);
        require(image.width == 2 * kDebugPadding + kDebugGlyphAdvance && image.height == 2 * kDebugPadding + 2 * kDebugLineAdvance, "panel size");
        require(image.rgba.size() == std::size_t(image.width) * std::size_t(image.height) * 4, "buffer size");
        require(pixelAt(image, kDebugPadding + 1, kDebugPadding)[0] == 255 && pixelAt(image, kDebugPadding, kDebugPadding)[3] == 170,
                "glyph pixels and panel background (I row 0 is 0x0E)");
        require(pixelAt(image, kDebugPadding, kDebugPadding + kDebugLineAdvance + 3)[0] == 255 &&
                pixelAt(image, kDebugPadding + 4, kDebugPadding + kDebugLineAdvance + 3)[0] == 255, "dash row (- row 3 is 0x1F)");
        const DebugTextImage doubled = rasteriseDebugText({"I"}, 2);
        require(doubled.width == 2 * (2 * kDebugPadding + kDebugGlyphAdvance) &&
                pixelAt(doubled, 2 * (kDebugPadding + 1) + 1, 2 * kDebugPadding + 1)[0] == 255, "scale");
        std::cout << "Debug text tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
