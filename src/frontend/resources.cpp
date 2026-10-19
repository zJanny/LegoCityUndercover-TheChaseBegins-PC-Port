#include "frontend/resources.hpp"
#include <cmath>
#include <stdexcept>

namespace chase::frontend {

Resources::Resources(assets::FuseArchive& archive) : archive_(archive) {}

bool Resources::has(const std::string& path) const {
    return archive_.find(path) != nullptr;
}

assets::Bytes Resources::bytes(const std::string& path) {
    const assets::FuseEntry* entry = archive_.find(path);
    if (!entry) throw std::runtime_error("not in archive: " + path);
    return archive_.read(*entry);
}

const assets::Image& Resources::image(const std::string& path) {
    auto found = images_.find(path);
    if (found == images_.end()) found = images_.emplace(path, assets::decodeTexture(bytes(path))).first;
    return found->second;
}

const BitmapFont& Resources::font(const std::string& name) {
    auto found = fonts_.find(name);
    if (found == fonts_.end()) {
        BitmapFont loaded{assets::parseFont(bytes("fonts/" + name + ".tnf")), image("fonts/" + name + ".btga")};
        found = fonts_.emplace(name, std::move(loaded)).first;
    }
    return found->second;
}

const assets::UiLayout& Resources::layout(const std::string& path) {
    auto found = layouts_.find(path);
    if (found == layouts_.end()) found = layouts_.emplace(path, assets::parseLayout(bytes(path))).first;
    return found->second;
}

std::string Resources::tableText(const std::string& path, std::size_t sheet, std::size_t row, std::size_t column) {
    auto found = dataTables_.find(path);
    if (found == dataTables_.end()) found = dataTables_.emplace(path, assets::parseTable(bytes(path))).first;
    if (sheet >= found->second.sheets.size()) throw std::runtime_error("table sheet out of range: " + path);
    const assets::TableSheet& data = found->second.sheets[sheet];
    if (row >= data.rows) throw std::runtime_error("table row out of range: " + path);
    return assets::tableValue(data, row, column).text;
}

const std::vector<std::string>& Resources::extendedCharacters() {
    if (!extendedLoaded_) {
        extended_ = assets::extendedCharacters(bytes("localisation/extended_chars_utf8.txt"));
        extendedLoaded_ = true;
    }
    return extended_;
}

const std::map<std::uint32_t, std::string>& Resources::table(const std::string& file) {
    auto found = tables_.find(file);
    if (found == tables_.end()) found = tables_.emplace(file, assets::parseLocalisation(bytes(file))).first;
    return found->second;
}

std::string Resources::textForKey(const std::string& file, std::uint32_t key) {
    const auto& entries = table(file);
    const auto found = entries.find(key);
    if (found == entries.end()) return {};
    return assets::decodeText(found->second, extendedCharacters());
}

std::string Resources::text(const std::string& file, const std::string& identifier) {
    return textForKey(file, assets::textKey(identifier));
}

int textWidth(const BitmapFont& font, const std::vector<std::string>& extended, const std::string& utf8, int spacing) {
    const auto placed = assets::layoutText(font.font, utf8, extended);
    if (placed.empty()) return 0;
    return placed.back().x + int(placed.size() - 1) * (spacing - 1) + placed.back().glyph.width;
}

void drawText(Canvas& canvas, const BitmapFont& font, const std::vector<std::string>& extended, const std::string& utf8,
              float x, float y, Align align, const Colour& colour, int spacing, bool middle) {
    const auto placed = assets::layoutText(font.font, utf8, extended);
    if (placed.empty()) return;
    const int width = textWidth(font, extended, utf8, spacing);
    auto roundHalfUp = [](float value) { return int(std::floor(value + 0.5f)); };
    int left = roundHalfUp(x);
    if (align == Align::Centre) left = roundHalfUp(x - width / 2.0f);
    else if (align == Align::Right) left = roundHalfUp(x - width);
    const int top = middle ? roundHalfUp(y - font.font.lineHeight / 2.0f) : roundHalfUp(y);
    int index = 0;
    for (const auto& glyph : placed) {
        const IntRect source{glyph.glyph.x, glyph.glyph.y, glyph.glyph.width, glyph.glyph.height};
        canvas.draw(font.atlas, source, left + glyph.x + index * (spacing - 1), top, colour);
        ++index;
    }
}

}
