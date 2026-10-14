#include "frontend/script_text_pages.hpp"
#include "frontend/layout.hpp"
#include "frontend/pages.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <optional>

namespace chase::frontend {

namespace {

constexpr const char* kBoxTexture = "sprites/ui_dialog/ui_dialog_box.btga";
constexpr int kBoxEdge = 16;
constexpr float kScreenCentreBottomX = 160, kScreenCentreTopX = 200;
constexpr float kBoxMinHalfWidth = 120, kBoxMinHalfHeight = 20;
constexpr float kTipCentreY = 120, kTipPadHeight = 16, kTipWrapWidth = 300;
constexpr float kBadgeOffsetX = -52, kInfoIconOffsetX = -36, kInfoIconOffsetY = -43;
constexpr float kDialogueCentreXOffset = 10, kDialogueBottomLine = 206, kDialoguePadWidth = 24, kDialogueWrapWidth = 280;
// STAND-INS (HUD-S8) fitted to the oracle screenshots: glyph centre against the line box centre, line pitch, icon width.
constexpr float kTipTextShiftFitted = 3.5f, kDialogueTextShiftFitted = 2.5f, kTextPitchFitted = 17.0f;
constexpr int kIconSizeFitted = 16;
// STAND-IN (HUD-S12) fitted to the oracle picture: glyph centre against the text position (160, 30) of 0x254d80.
constexpr float kHeadingTextShiftFitted = 0.5f;
constexpr float kHeadingCentreY = 30, kHeadingHalfHeight = 12, kHeadingMargin = 15, kHeadingMaxAlpha = 240;
constexpr unsigned kCharacterNameKeyColumn = 3;
constexpr unsigned kLevelNameColumn = 21, kLevelHeadingKeyColumn = 11;
constexpr float kPortraitOffsetX = -7, kPortraitOffsetY = -1;
constexpr const char* kPortraitFrameTexture = "sprites/ui_hud/blend/images/heartsbg_generic.bpng";
constexpr Colour kTipTextColour{74.0f / 255.0f, 173.0f / 255.0f, 1.0f, 1.0f};
constexpr Colour kDialogueTextColour{1.0f, 169.0f / 255.0f, 22.0f / 255.0f, 1.0f};

int evenRounded(float value) {
    int rounded = int(std::floor(value + 0.5f));
    return rounded + (rounded & 1);
}

void drawFramedBox(Canvas& canvas, Resources& resources, float centreX, float centreY, float halfWidth, float halfHeight, Colour fade) {
    const int innerHalfWidth = evenRounded(halfWidth), innerHalfHeight = evenRounded(halfHeight);
    const int left = evenRounded(centreX) - innerHalfWidth - kBoxEdge, top = evenRounded(centreY) - innerHalfHeight - kBoxEdge;
    canvas.drawNineSlice(resources.image(kBoxTexture), kBoxEdge, left, top, 2 * (innerHalfWidth + kBoxEdge), 2 * (innerHalfHeight + kBoxEdge), fade);
}

struct Word {
    std::string text;
    std::string icon;
    int width = 0;
};

Colour withAlpha(Colour colour, float alpha) {
    colour.a *= alpha;
    return colour;
}

std::string lowered(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

std::string buttonIconFile(const std::string& token) {
    static const std::map<std::string, std::string> kIcons = {
        {"<east>", "dsbuttona"}, {"<south>", "dsbuttonb"}, {"<north>", "dsbuttonx"}, {"<west>", "dsbuttony"},
        {"<l>", "dsbuttonl"}, {"<r>", "dsbuttonr"}, {"<stick>", "button_analog"}};
    const auto found = kIcons.find(lowered(token));
    return found == kIcons.end() ? std::string() : "sprites/buttons/" + found->second + ".btga";
}

std::string dialoguePortraitName(Resources& resources, unsigned character) {
    if (character == 0 || character == 147) return {};
    static std::optional<assets::Table> table;
    if (!table) {
        const assets::FuseEntry* entry = resources.archive().find("tables/tablescharacters.bxls");
        if (!entry) return {};
        table = assets::parseTable(resources.archive().read(*entry));
    }
    if (table->sheets.empty() || character >= table->sheets[0].rows) return {};
    return assets::tableValue(table->sheets[0], character, 1).text;
}

std::uint32_t characterNameKey(Resources& resources, unsigned character) {
    const assets::FuseEntry* entry = resources.archive().find("tables/tablescharacters.bxls");
    if (!entry) return 0;
    const assets::Table table = assets::parseTable(resources.archive().read(*entry));
    if (table.sheets.empty() || character >= table.sheets[0].rows) return 0;
    return assets::tableValue(table.sheets[0], character, kCharacterNameKeyColumn).bits;
}

std::uint32_t levelHeadingKey(Resources& resources, const std::string& levelName) {
    const assets::FuseEntry* entry = resources.archive().find("tables/tableslevels.bxls");
    if (!entry) return 0;
    const assets::Table table = assets::parseTable(resources.archive().read(*entry));
    if (table.sheets.empty() || table.sheets[0].columns.size() <= kLevelNameColumn) return 0;
    const assets::TableSheet& sheet = table.sheets[0];
    for (unsigned row = 0; row < sheet.rows; ++row)
        if (lowered(assets::tableValue(sheet, row, kLevelNameColumn).text) == levelName) return assets::tableValue(sheet, row, kLevelHeadingKeyColumn).bits;
    return 0;
}

void drawHeadingBox(Canvas& bottom, Resources& resources, const std::string& text, float alpha) {
    const int byteAlpha = int(alpha * 255.0f);
    if (byteAlpha == 0) return;
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const float halfWidth = std::min(float(textWidth(font, extended, text, 0)) * 0.5f, kScreenCentreBottomX - kHeadingMargin);
    const float boxAlpha = std::min(float(byteAlpha), kHeadingMaxAlpha) / 255.0f;
    drawFramedBox(bottom, resources, kScreenCentreBottomX, kHeadingCentreY, halfWidth, kHeadingHalfHeight, Colour{1, 1, 1, boxAlpha});
    drawText(bottom, font, extended, text, kScreenCentreBottomX, kHeadingCentreY + kHeadingTextShiftFitted, Align::Centre,
             Colour{1, 1, 1, float(byteAlpha) / 255.0f}, 0, true);
}

void drawDialogueBackground(Canvas& bottom, Resources& resources) {
    drawDialogBackground(bottom, resources);
}

void drawTipPage(Canvas& bottom, Resources& resources, const std::string& text, float alpha) {
    const Colour fade{1, 1, 1, alpha};
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const int space = textWidth(font, extended, "a a", 0) - textWidth(font, extended, "aa", 0);
    std::vector<Word> words;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        Word word;
        word.text = text.substr(start, end - start);
        word.icon = buttonIconFile(word.text);
        word.width = word.icon.empty() ? textWidth(font, extended, word.text, 0) : kIconSizeFitted;
        words.push_back(word);
        start = end + 1;
    }
    std::vector<std::vector<Word>> lines(1);
    std::vector<int> lineWidths(1, 0);
    for (const Word& word : words) {
        const int added = lines.back().empty() ? word.width : space + word.width;
        if (!lines.back().empty() && float(lineWidths.back() + added) > kTipWrapWidth) {
            lines.emplace_back();
            lineWidths.push_back(0);
        }
        lineWidths.back() += lines.back().empty() ? word.width : space + word.width;
        lines.back().push_back(word);
    }
    const float widest = float(*std::max_element(lineWidths.begin(), lineWidths.end()));
    const float halfWidth = std::max(kBoxMinHalfWidth, widest / 2);
    const float halfHeight = std::max(kBoxMinHalfHeight, kTextPitchFitted * float(lines.size()) / 2);

    drawFramedBox(bottom, resources, kScreenCentreBottomX, kTipCentreY, halfWidth, halfHeight + kTipPadHeight, fade);
    const assets::Image& info = resources.image("sprites/ui_dialog/ui_dialog_infoicon.btga");
    bottom.drawScaled(info, {0, 0, info.width, info.height}, kScreenCentreBottomX + halfWidth + kInfoIconOffsetX,
                      kTipCentreY - halfHeight + kInfoIconOffsetY, float(info.width), float(info.height), fade);
    const assets::Image& badge = resources.image("sprites/ui_dialog/ui_dialog_badge.btga");
    bottom.drawScaled(badge, {0, 0, badge.width, badge.height}, kScreenCentreBottomX + halfWidth + kBadgeOffsetX, kTipCentreY + halfHeight,
                      float(badge.width), float(badge.height), fade);

    const float first = kTipCentreY + kTipTextShiftFitted - kTextPitchFitted * float(lines.size() - 1) / 2;
    for (std::size_t row = 0; row < lines.size(); ++row) {
        int x = int(std::floor(kScreenCentreBottomX - float(lineWidths[row]) / 2.0f + 0.5f));
        const float y = first + kTextPitchFitted * float(row);
        for (std::size_t i = 0; i < lines[row].size(); ++i) {
            if (i) x += space;
            const Word& word = lines[row][i];
            if (!word.icon.empty()) {
                const assets::Image& icon = resources.image(word.icon);
                bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, float(x), y - kIconSizeFitted / 2.0f, float(kIconSizeFitted),
                                  float(kIconSizeFitted), fade);
            } else {
                drawText(bottom, font, extended, word.text, float(x), y, Align::Left, withAlpha(kTipTextColour, alpha), 0, true);
            }
            x += word.width;
        }
    }
}

void drawDialogueBox(Canvas& top, Resources& resources, unsigned character, const std::string& text, float alpha) {
    const Colour fade{1, 1, 1, alpha};
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const std::vector<std::string> lines = wrapText(font, extended, text, int(kDialogueWrapWidth), 0);
    int widest = 0;
    for (const std::string& line : lines) widest = std::max(widest, textWidth(font, extended, line, 0));
    const float halfWidth = std::max(kBoxMinHalfWidth, float(widest) / 2);
    const float halfHeight = std::max(kBoxMinHalfHeight, kTextPitchFitted * float(lines.size()) / 2);
    const float centreX = kScreenCentreTopX + kDialogueCentreXOffset, centreY = kDialogueBottomLine - halfHeight;

    drawFramedBox(top, resources, centreX, centreY, halfWidth + kDialoguePadWidth, halfHeight, fade);
    const float first = centreY + kDialogueTextShiftFitted - kTextPitchFitted * float(lines.size() - 1) / 2;
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawText(top, font, extended, lines[i], centreX, first + kTextPitchFitted * float(i), Align::Centre,
                 withAlpha(kDialogueTextColour, alpha), 0, true);
    const std::string portrait = dialoguePortraitName(resources, character);
    const std::string path = "sprites/portraits/" + portrait + ".bpng";
    if (!portrait.empty() && resources.has(path)) {
        const float portraitCentreX = kScreenCentreTopX - halfWidth - kDialoguePadWidth + kPortraitOffsetX;
        const float portraitCentreY = centreY + kPortraitOffsetY;
        for (const char* file : {kPortraitFrameTexture, path.c_str()}) {
            const assets::Image& image = resources.image(file);
            top.drawScaled(image, {0, 0, image.width, image.height}, portraitCentreX - float(image.width / 2), portraitCentreY - float(image.height / 2),
                           float(image.width), float(image.height), fade);
        }
    }
}

}
