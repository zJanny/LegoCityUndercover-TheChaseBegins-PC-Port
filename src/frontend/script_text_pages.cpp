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
constexpr int kBoxEdge = 16, kBoxMarginX = 8, kBoxMarginY = 4;
// STAND-INS fitted to the oracle screenshots of the tips of mission 1-1 (bottom screen, 320x240).
constexpr int kPanelX = 21, kPanelY = 72, kPanelWidth = 280, kPanelHeight = 96;
constexpr float kInfoIconCentreX = 271, kInfoIconCentreY = 73, kInfoIconSize = 28;
constexpr float kBadgeCentreX = 271, kBadgeCentreY = 168, kBadgeWidth = 62;
constexpr float kTextCentreX = 160, kTextCentreY = 123.5f, kTextPitch = 17.0f;
constexpr int kTextWrapWidth = 266, kIconSize = 16;
constexpr Colour kTipTextColour{74.0f / 255.0f, 173.0f / 255.0f, 1.0f, 1.0f};
// STAND-INS fitted to the dialogue box of the oracle screenshot (top screen, 400x240).
constexpr int kDialogueX = 44, kDialogueY = 155, kDialogueWidth = 332, kDialogueHeight = 62;
constexpr float kPortraitCentreX = 36, kPortraitCentreY = 186, kPortraitSize = 56;
constexpr float kDialogueTextCentreX = 210, kDialogueFirstLineY = 180, kDialoguePitch = 17.0f;
constexpr int kDialogueWrapWidth = 290;
constexpr Colour kDialogueTextColour{1.0f, 169.0f / 255.0f, 22.0f / 255.0f, 1.0f};

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

void drawDialogueBackground(Canvas& bottom, Resources& resources) {
    drawDialogBackground(bottom, resources);
}

void drawTipPage(Canvas& bottom, Resources& resources, const std::string& text, float alpha) {
    const Colour fade{1, 1, 1, alpha};
    bottom.drawNineSlice(resources.image(kBoxTexture), kBoxEdge, kPanelX - kBoxMarginX, kPanelY - kBoxMarginY, kPanelWidth + 2 * kBoxMarginX, kPanelHeight + 2 * kBoxMarginY, fade);
    const assets::Image& info = resources.image("sprites/ui_dialog/ui_dialog_infoicon.btga");
    bottom.drawScaled(info, {0, 0, info.width, info.height}, kInfoIconCentreX - kInfoIconSize / 2, kInfoIconCentreY - kInfoIconSize / 2,
                      kInfoIconSize, kInfoIconSize, fade);
    const assets::Image& badge = resources.image("sprites/ui_dialog/ui_dialog_badge.btga");
    const float badgeHeight = kBadgeWidth * float(badge.height) / float(badge.width);
    bottom.drawScaled(badge, {0, 0, badge.width, badge.height}, kBadgeCentreX - kBadgeWidth / 2, kBadgeCentreY - badgeHeight / 2, kBadgeWidth,
                      badgeHeight, fade);

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
        word.width = word.icon.empty() ? textWidth(font, extended, word.text, 0) : kIconSize;
        words.push_back(word);
        start = end + 1;
    }
    std::vector<std::vector<Word>> lines(1);
    int lineWidth = 0;
    for (const Word& word : words) {
        const int added = lines.back().empty() ? word.width : space + word.width;
        if (!lines.back().empty() && lineWidth + added > kTextWrapWidth) {
            lines.emplace_back();
            lineWidth = 0;
        }
        lineWidth += lines.back().empty() ? word.width : space + word.width;
        lines.back().push_back(word);
    }
    const float first = kTextCentreY - kTextPitch * float(lines.size() - 1) / 2;
    for (std::size_t row = 0; row < lines.size(); ++row) {
        int total = 0;
        for (std::size_t i = 0; i < lines[row].size(); ++i) total += lines[row][i].width + (i ? space : 0);
        int x = int(std::floor(kTextCentreX - total / 2.0f + 0.5f));
        const float y = first + kTextPitch * float(row);
        for (std::size_t i = 0; i < lines[row].size(); ++i) {
            if (i) x += space;
            const Word& word = lines[row][i];
            if (!word.icon.empty()) {
                const assets::Image& icon = resources.image(word.icon);
                bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, float(x), y - kIconSize / 2.0f, float(kIconSize), float(kIconSize), fade);
            } else {
                drawText(bottom, font, extended, word.text, float(x), y, Align::Left, withAlpha(kTipTextColour, alpha), 0, true);
            }
            x += word.width;
        }
    }
}

void drawDialogueBox(Canvas& top, Resources& resources, unsigned character, const std::string& text, float alpha) {
    const Colour fade{1, 1, 1, alpha};
    top.drawNineSlice(resources.image(kBoxTexture), kBoxEdge, kDialogueX - kBoxMarginX, kDialogueY - kBoxMarginY, kDialogueWidth + 2 * kBoxMarginX, kDialogueHeight + 2 * kBoxMarginY, fade);
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const std::vector<std::string> lines = wrapText(font, extended, text, kDialogueWrapWidth, 0);
    const float first = kDialogueFirstLineY + kDialoguePitch * (2 - float(std::min<std::size_t>(lines.size(), 2))) / 2;
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawText(top, font, extended, lines[i], kDialogueTextCentreX, first + kDialoguePitch * float(i), Align::Centre,
                 withAlpha(kDialogueTextColour, alpha), 0, true);
    const std::string portrait = dialoguePortraitName(resources, character);
    const std::string path = "sprites/portraits/" + portrait + ".bpng";
    if (!portrait.empty() && resources.has(path)) {
        const assets::Image& image = resources.image(path);
        top.drawScaled(image, {0, 0, image.width, image.height}, kPortraitCentreX - kPortraitSize / 2, kPortraitCentreY - kPortraitSize / 2,
                       kPortraitSize, kPortraitSize, fade);
    }
}

}
