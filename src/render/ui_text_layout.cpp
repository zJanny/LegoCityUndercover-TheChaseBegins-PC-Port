#include "render/ui_text_layout.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace chase::render {

namespace {
int wholeSize(float pixelSize) { return std::max(1, static_cast<int>(std::lround(pixelSize))); }
}

UiGlyphAtlas::UiGlyphAtlas(int size) : size_(size), pixels_(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 0) {}

bool UiGlyphAtlas::takeDirty() {
    const bool was = dirty_;
    dirty_ = false;
    return was;
}

const UiGlyphAtlas::Entry& UiGlyphAtlas::entry(char32_t codepoint, float pixelSize) {
    const int whole = wholeSize(pixelSize);
    const std::uint64_t key = (static_cast<std::uint64_t>(codepoint) << 16) | static_cast<std::uint64_t>(whole);
    const auto found = entries_.find(key);
    if (found != entries_.end()) return found->second;
    const char32_t drawn = uiFontHasGlyph(codepoint) ? codepoint : 0;
    const UiGlyphBitmap bitmap = rasteriseUiGlyph(drawn, static_cast<float>(whole));
    Entry placed;
    placed.width = bitmap.width;
    placed.height = bitmap.height;
    placed.offsetX = bitmap.offsetX;
    placed.offsetY = bitmap.offsetY;
    if (bitmap.width > 0 && bitmap.height > 0) {
        if (cursorX_ + bitmap.width + 1 > size_) {
            cursorX_ = 1;
            cursorY_ += rowHeight_ + 1;
            rowHeight_ = 0;
        }
        if (cursorY_ + bitmap.height + 1 > size_) throw std::runtime_error("UI glyph atlas is full");
        placed.x = cursorX_;
        placed.y = cursorY_;
        for (int row = 0; row < bitmap.height; ++row)
            std::copy_n(bitmap.coverage.begin() + static_cast<std::ptrdiff_t>(row) * bitmap.width, bitmap.width,
                        pixels_.begin() + static_cast<std::ptrdiff_t>(cursorY_ + row) * size_ + cursorX_);
        cursorX_ += bitmap.width + 1;
        rowHeight_ = std::max(rowHeight_, bitmap.height);
        dirty_ = true;
    }
    return entries_.emplace(key, placed).first->second;
}

bool clipUiQuad(UiGlyphQuad& quad, const UiClip& clip) {
    const float left = std::max(quad.x0, static_cast<float>(clip.x)), right = std::min(quad.x1, static_cast<float>(clip.x + clip.width));
    const float top = std::max(quad.y0, static_cast<float>(clip.y)), bottom = std::min(quad.y1, static_cast<float>(clip.y + clip.height));
    if (left >= right || top >= bottom) return false;
    const float du = (quad.u1 - quad.u0) / (quad.x1 - quad.x0), dv = (quad.v1 - quad.v0) / (quad.y1 - quad.y0);
    quad.u0 += (left - quad.x0) * du;
    quad.u1 -= (quad.x1 - right) * du;
    quad.v0 += (top - quad.y0) * dv;
    quad.v1 -= (quad.y1 - bottom) * dv;
    quad.x0 = left;
    quad.x1 = right;
    quad.y0 = top;
    quad.y1 = bottom;
    return true;
}

std::vector<UiGlyphQuad> layoutUiText(UiGlyphAtlas& atlas, std::string_view utf8, float pixelSize, float x, float y, const UiClip* clip) {
    const float size = static_cast<float>(wholeSize(pixelSize));
    const std::vector<char32_t> codepoints = decodeUtf8(utf8);
    const float baseline = std::round(y + uiFontMetrics(size).ascent);
    const float inverse = 1.0f / static_cast<float>(atlas.size());
    std::vector<UiGlyphQuad> quads;
    float pen = x;
    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        const UiGlyphAtlas::Entry& entry = atlas.entry(codepoints[i], size);
        if (entry.width > 0 && entry.height > 0) {
            const float left = std::round(pen) + static_cast<float>(entry.offsetX), top = baseline + static_cast<float>(entry.offsetY);
            UiGlyphQuad quad{left, top, left + static_cast<float>(entry.width), top + static_cast<float>(entry.height),
                             static_cast<float>(entry.x) * inverse, static_cast<float>(entry.y) * inverse,
                             static_cast<float>(entry.x + entry.width) * inverse, static_cast<float>(entry.y + entry.height) * inverse};
            if (!clip || clipUiQuad(quad, *clip)) quads.push_back(quad);
        }
        pen += uiGlyphAdvance(codepoints[i], size);
        if (i + 1 < codepoints.size()) pen += uiKerning(codepoints[i], codepoints[i + 1], size);
    }
    return quads;
}

}
