#pragma once
#include "assets/assets.hpp"
#include "frontend/canvas.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace chase::frontend {

struct BitmapFont {
    assets::Font font;
    assets::Image atlas;
};

class Resources {
public:
    explicit Resources(assets::FuseArchive& archive);
    assets::FuseArchive& archive() { return archive_; }
    bool has(const std::string& path) const;
    assets::Bytes bytes(const std::string& path);
    const assets::Image& image(const std::string& path);
    const BitmapFont& font(const std::string& name);
    const assets::UiLayout& layout(const std::string& path);
    const std::vector<std::string>& extendedCharacters();
    std::string text(const std::string& file, const std::string& identifier);
    std::string textForKey(const std::string& file, std::uint32_t key);
    std::string tableText(const std::string& path, std::size_t sheet, std::size_t row, std::size_t column);
    std::size_t tableRows(const std::string& path, std::size_t sheet);

private:
    const std::map<std::uint32_t, std::string>& table(const std::string& file);
    const assets::TableSheet& dataSheet(const std::string& path, std::size_t sheet);
    assets::FuseArchive& archive_;
    std::map<std::string, assets::Image> images_;
    std::map<std::string, BitmapFont> fonts_;
    std::map<std::string, assets::UiLayout> layouts_;
    std::map<std::string, assets::Table> dataTables_;
    std::map<std::string, std::map<std::uint32_t, std::string>> tables_;
    std::vector<std::string> extended_;
    bool extendedLoaded_ = false;
};

enum class Align { Left, Centre, Right };

int textWidth(const BitmapFont& font, const std::vector<std::string>& extended, const std::string& utf8, int spacing = 1);
void drawText(Canvas& canvas, const BitmapFont& font, const std::vector<std::string>& extended, const std::string& utf8,
              float x, float y, Align align, const Colour& colour, int spacing = 1, bool middle = false);

}
