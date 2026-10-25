#include "frontend/pages.hpp"
#include <algorithm>
#include <cmath>

namespace chase::frontend {

namespace {

constexpr const char* kAlertLayout = "sprites/blend/savegameflow/page_alerttext.bxaml";
constexpr const char* kSaveIconLayout = "sprites/blend/savegameflow/page_saveicon.bxaml";
// STAND-IN: position and size of the spinner measured on the oracle screenshots (64 px, top left (128, 180)); the
// code that places it was not read.
constexpr float kSpinnerX = 128, kSpinnerY = 180, kSpinnerSize = 64;
// STAND-IN: line pitch of the body text (27 px for one or two lines, 23 px for three) and its wrap width, fitted to
// the oracle screenshots; the rule of the original (box height 110, font size field) is not read.
constexpr int kBodyWrapWidth = 296, kPitchShort = 27, kPitchLong = 23;
// STAND-IN: top-left of the title layers on the 400x240 screen, fitted to the oracle (mean abs diff 0.26).
constexpr int kTitleX = -56, kTitleY = -8;
// STAND-IN FE-S32: the Delete label is orange in the oracle (sl0.png brightest texel 255, 200, 100); the code that tints it is not read.
constexpr float kDeleteTintGreen = 200.0f / 255.0f, kDeleteTintBlue = 100.0f / 255.0f;
constexpr const char* kExtraButtonImage = "sprites/blend/ui_frontend/images/screen_extrabutton.bpng";
constexpr const char* kShortcutsLayout = "sprites/blend/ui_frontend/shortcuts.bxaml";
constexpr float kShortcutsX = -80, kShortcutsY = -16;
constexpr const char* kDialogBoxTexture = "sprites/ui_dialog/ui_dialog_box.btga";
// STAND-INS fitted to oracle screenshots of the movie bubble, loading tip and mission card.
constexpr int kBoxEdge = 16;
constexpr int kBubbleX = 6, kBubbleWidth = 308;
constexpr float kBubblePitch = 23, kBubbleTextY = 33.5f;
// STAND-IN (FE-S40): the bubble box height follows the line count like the generic text box 0x229128 (half height =
// lines * pitch / 2, rounded to an even number); matches the oracle for one and two lines, extrapolated for more.
// The centre 26.5 + 11.5 * lines is fitted to those two boxes (the code that places the box is not recovered).
constexpr float kBubbleCentreBase = 26.5f, kBubbleCentreStep = 11.5f;
constexpr int kBoxMarginX = 8, kBoxMarginY = 4;
constexpr int kTipX = 8, kTipY = 78, kTipWidth = 304, kTipHeight = 152, kTipWrapWidth = 270;
constexpr int kTipHexX = 32, kTipHexY = -10;
constexpr float kTipBadgeX = 102, kTipBadgeY = 8, kTipBadgeSize = 120, kTipTextY = 160, kTipPitch = 22;
constexpr int kTipIconSize = 16;
constexpr int kCardPadding = 12, kCardHeight = 40;
constexpr float kCardY = 4;
constexpr const char* kEastToken = "<EAST>";
// STAND-INS fitted to the oracle screenshot of the WarningDialogBox (the save state outside the police station after mission 1-1, docs/re/scripts.md): box 296x92
// at (52, 126) on the top screen, orange text (255, 169, 22), button row at y 203.
constexpr int kWarnX = 52, kWarnY = 126, kWarnWidth = 296, kWarnHeight = 92, kWarnWrapWidth = 280;
constexpr float kWarnTextCentreX = 200, kWarnFirstLineY = 141, kWarnPitch = 17.5f, kWarnButtonY = 203;
constexpr float kWarnCancelIconX = 63, kWarnCancelTextX = 86, kWarnConfirmTextRightX = 316, kWarnConfirmIconX = 333, kWarnIconSize = 28;
constexpr Colour kWarnTextColour{1.0f, 169.0f / 255.0f, 22.0f / 255.0f, 1.0f};
constexpr float kSlotConfirmedSeconds = 0.3f;
// STAND-IN FE-S31: the percent's right edge is 37 px left of its layout element's (oracle sl0.png: glyphs end at x 276).
constexpr int kSlotPercentShiftX = -37;
constexpr const char* kSlotLayout = "sprites/blend/savegameflow/page_selectslot.bxaml";
constexpr const char* kButtonA = "sprites/buttons/dsbuttona.btga";
constexpr const char* kButtonB = "sprites/buttons/dsbuttonb.btga";

TextSlot centred(const std::string& text, bool wrap = false) {
    TextSlot slot;
    slot.text = text;
    slot.align = Align::Centre;
    slot.wrap = wrap;
    return slot;
}

}

void drawAlertPage(Canvas& bottom, Resources& resources, const AlertPage& page) {
    LayoutScene scene;
    scene.storyboard = page.secondTransition ? "TransitionOn2" : "TransitionOn";
    scene.time = page.seconds;
    scene.textSpacing = 0;
    scene.hidden = {"FontPlaceholder", "FontPlaceholder_Small"};
    const bool hasHeader = !page.header.empty();
    scene.hidden.insert(hasHeader ? "Text_Info_NoHeader" : "Text_Info");
    if (!hasHeader) scene.hidden.insert("Text_Header");
    TextSlot body = centred(page.body, true);
    const std::size_t lines = wrapText(resources.font("frontendfont"), resources.extendedCharacters(), page.body,
                                       kBodyWrapWidth, 0).size();
    body.pitch = lines <= 2 ? kPitchShort : kPitchLong;
    scene.text[hasHeader ? "Text_Info" : "Text_Info_NoHeader"] = body;
    if (hasHeader) scene.text["Text_Header"] = centred(page.header);
    const char* optionNames[2] = {"Option1", "Option2"};
    const char* textNames[2] = {"Text_Option1", "Text_Option2"};
    const char* iconNames[2] = {"ButtonIcon1", "ButtonIcon2"};
    for (std::size_t i = 0; i < 2; ++i) {
        if (i >= page.buttons.size()) {
            scene.hidden.insert(optionNames[i]);
            continue;
        }
        TextSlot label;
        label.text = page.buttons[i].text;
        label.align = i == 0 ? Align::Right : Align::Left;
        scene.text[textNames[i]] = label;
        scene.images[iconNames[i]] = page.buttons[i].symbol == PadSymbol::A ? kButtonA : kButtonB;
    }
    drawLayout(bottom, resources, kAlertLayout, scene, 0, 0);
}

void drawSaveSpinner(Canvas& bottom, Resources& resources, float phase, float opacity) {
    const assets::Image& icon = resources.image("sprites/ui_saveindicator/ui_saveindicator.btga");
    float rotation = 0, alpha = 1;
    for (const auto& board : resources.layout(kSaveIconLayout).storyboards) {
        if (board.name != "Activity") continue;
        const float time = std::fmod(phase, board.tracks.front().duration);
        for (const auto& track : board.tracks) {
            if (track.property == 5) rotation = evaluateTrack(track, time);
            else if (track.property == 6) alpha = evaluateTrack(track, time);
        }
    }
    const float size = kSpinnerSize;
    bottom.drawRotated(icon, {0, 0, icon.width, icon.height}, kSpinnerX + size / 2, kSpinnerY + size / 2, size, size,
                       rotation, {1, 1, 1, opacity * alpha});
}

void drawSelectShortcut(Canvas& bottom, Resources& resources, const std::string& selectLabel, const std::string& extraLabel) {
    LayoutScene shortcuts;
    shortcuts.hidden = {"BackButton_DS", "SelectButton_DS", "ExtraButton_DS", "BackButton_3DS"};
    if (extraLabel.empty()) shortcuts.hidden.insert("ExtraButton_3DS");
    else {
        TextSlot extra;
        extra.text = extraLabel;
        extra.align = Align::Left;
        extra.spacing = 0;
        extra.tinted = true;
        extra.tint[1] = kDeleteTintGreen;
        extra.tint[2] = kDeleteTintBlue;
        shortcuts.text["ExtraButton_3DS/Text_Title"] = extra;
        shortcuts.images["ExtraButton_3DS/Button_Image"] = kExtraButtonImage;
    }
    shortcuts.textSpacing = 1;
    TextSlot select;
    select.text = selectLabel;
    select.align = Align::Right;
    shortcuts.text["SelectButton_3DS/Text_Title"] = select;
    drawLayout(bottom, resources, kShortcutsLayout, shortcuts, kShortcutsX, kShortcutsY);
}

void drawBackSelectShortcuts(Canvas& bottom, Resources& resources, const ShortcutView& back, const ShortcutView& select) {
    LayoutScene shortcuts;
    shortcuts.hidden = {"BackButton_DS", "SelectButton_DS", "ExtraButton_DS", "ExtraButton_3DS"};
    shortcuts.textSpacing = 1;
    const auto place = [&](const ShortcutView& view, const std::string& element, Align align) {
        if (!view.drawn) {
            shortcuts.hidden.insert(element);
            return;
        }
        TextSlot text;
        text.text = view.label;
        text.align = align;
        shortcuts.text[element + "/Text_Title"] = text;
        shortcuts.referenceStoryboards[element] = view.storyboard;
        shortcuts.referenceTimes[element] = view.time;
    };
    place(back, "BackButton_3DS", Align::Left);
    place(select, "SelectButton_3DS", Align::Right);
    drawLayout(bottom, resources, kShortcutsLayout, shortcuts, kShortcutsX, kShortcutsY);
}

void drawSlotPage(Canvas& bottom, Resources& resources, const std::string& header, const std::string& selectLabel,
                  const std::string& deleteLabel, const std::vector<SlotRow>& rows, unsigned selected, float seconds,
                  float selectedSeconds, float confirmSeconds) {
    const assets::Image& background = resources.image("sprites/blend/savegameflow/images/ui_lcu_bg.bpng");
    bottom.draw(background, {0, 0, kBottomWidth, kScreenHeight}, 0, 0);
    LayoutScene scene;
    scene.storyboard = confirmSeconds >= 0 ? (selected == 0 ? "Select_Slot1" : "Select_Slot2") : "TransitionOn";
    scene.time = confirmSeconds >= 0 ? confirmSeconds : seconds;
    const bool cleared = confirmSeconds >= kSlotConfirmedSeconds;
    scene.textSpacing = 0;
    scene.hidden = {"Text_Header", "Text_SubInfo", "Text_Confirm", "Text_Infoold", "NoticePanelBg", "ListItem3"};
    TextSlot headerSlot;
    headerSlot.text = header;
    headerSlot.align = Align::Left;
    headerSlot.spacing = -1;
    headerSlot.offsetY = -2;
    scene.text["Text_Info"] = headerSlot;
    if (cleared) {
        scene.hidden.insert("ListItem1");
        scene.hidden.insert("ListItem2");
        scene.hidden.insert("canvas");
    }
    for (std::size_t i = 0; i < rows.size() && i < 2; ++i) {
        const std::string name = "ListItem" + std::to_string(i + 1);
        TextSlot number;
        number.text = std::to_string(rows[i].number);
        number.align = Align::Left;
        scene.text[name + "/Text_SlotNum"] = number;
        if (rows[i].used) {
            scene.hidden.insert(name + "/Text_NoData");
            TextSlot time, percent, location;
            time.text = rows[i].time;
            time.align = Align::Left;
            percent.text = rows[i].percent;
            percent.align = Align::Right;
            percent.offsetX = kSlotPercentShiftX;
            location.text = rows[i].location;
            location.align = Align::Left;
            scene.text[name + "/Text_Time"] = time;
            scene.text[name + "/Text_Percent"] = percent;
            scene.text[name + "/Text_Location"] = location;
        } else {
            scene.hidden.insert(name + "/Text_Time");
            scene.hidden.insert(name + "/Text_Percent");
            scene.hidden.insert(name + "/Text_Location");
            TextSlot label;
            label.text = rows[i].text;
            label.align = Align::Left;
            scene.text[name + "/Text_NoData"] = label;
        }
        scene.referenceStoryboards[name] = "ButtonHighlight";
        scene.referenceTimes[name] = i == selected ? selectedSeconds : 0.0f;
    }
    drawLayout(bottom, resources, kSlotLayout, scene, 0, 0);
    if (!cleared) drawSelectShortcut(bottom, resources, selectLabel, selected < rows.size() && rows[selected].used ? deleteLabel : std::string());
}

void drawTitleTop(Canvas& top, Resources& resources, const std::string& suffix, float level) {
    const Colour tint{level, level, level, 1};
    for (int layer = 1; layer <= 5; ++layer) {
        const std::string path = "sprites/ui_frontend/topscreen/ui_3dstopscreenlogobg_0" + std::to_string(layer) + "_" + suffix + ".btga";
        const assets::Image& image = resources.image(path);
        top.draw(image, {0, 0, image.width, image.height}, kTitleX, kTitleY, tint);
    }
}

void drawDialogBackground(Canvas& bottom, Resources& resources, float alpha) {
    const assets::Image& left = resources.image("sprites/ui_dialog/ui_dialog_a.btga");
    const assets::Image& right = resources.image("sprites/ui_dialog/ui_dialog_b.btga");
    const Colour tint{1, 1, 1, alpha};
    bottom.draw(left, {0, 0, left.width, kScreenHeight}, 0, 0, tint);
    bottom.draw(right, {0, 0, kBottomWidth - left.width, kScreenHeight}, left.width, 0, tint);
}

void drawSubtitleBubble(Canvas& bottom, Resources& resources, const std::vector<std::string>& cueLines) {
    drawDialogBackground(bottom, resources);
    if (cueLines.empty()) return;
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const std::vector<std::string> lines = assets::wrapSubtitleLines(
        cueLines, assets::subtitleWrapWidthStandIn(kBottomWidth), [&](const std::string& text) { return textWidth(font, extended, text, 0); });
    const auto evenRounded = [](float value) {
        const int rounded = int(std::floor(value + 0.5f));
        return rounded + (rounded & 1);
    };
    const float count = float(lines.size());
    const int half = evenRounded(count * kBubblePitch / 2);
    const int top = evenRounded(kBubbleCentreBase + kBubbleCentreStep * count) - half - kBoxEdge;
    bottom.drawNineSlice(resources.image(kDialogBoxTexture), kBoxEdge, kBubbleX - kBoxMarginX, top,
                         kBubbleWidth + 2 * kBoxMarginX, 2 * (half + kBoxEdge));
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawText(bottom, font, extended, lines[i], kBubbleX + kBubbleWidth / 2.0f, kBubbleTextY + kBubblePitch * float(i),
                 Align::Centre, {1, 1, 1, 1}, 0, true);
}

void drawLoadingTop(Canvas& top, Resources& resources) {
    const assets::Image& image = resources.image("sprites/ui_loadingscreens/ui_ls_generic1.btga");
    top.draw(image, {0, 0, kTopWidth, kScreenHeight}, 0, 0);
}

namespace {

struct RichWord {
    std::string text;
    bool icon = false;
    int width = 0;
};

}

void drawLoadingBottom(Canvas& bottom, Resources& resources, const std::string& tip) {
    drawDialogBackground(bottom, resources);
    const assets::Image& hex = resources.image("sprites/blend/awardviewertop/images/bighexbg.bpng");
    bottom.draw(hex, {0, 0, hex.width, hex.height}, kTipHexX, kTipHexY);
    bottom.drawNineSlice(resources.image(kDialogBoxTexture), kBoxEdge, kTipX - kBoxMarginX, kTipY - kBoxMarginY,
                         kTipWidth + 2 * kBoxMarginX, kTipHeight + 2 * kBoxMarginY);
    const assets::Image& badge = resources.image("sprites/ui_dialog/ui_dialog_badge.btga");
    bottom.drawScaled(badge, {0, 0, badge.width, badge.height}, kTipBadgeX, kTipBadgeY, kTipBadgeSize, kTipBadgeSize);

    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const assets::Image& icon = resources.image("sprites/buttons/dsbuttona.btga");
    const int space = textWidth(font, extended, "a a", 0) - textWidth(font, extended, "aa", 0);
    std::vector<RichWord> words;
    std::size_t start = 0;
    while (start <= tip.size()) {
        std::size_t end = tip.find(' ', start);
        if (end == std::string::npos) end = tip.size();
        RichWord word;
        word.text = tip.substr(start, end - start);
        word.icon = word.text == kEastToken;
        word.width = word.icon ? kTipIconSize : textWidth(font, extended, word.text, 0);
        words.push_back(word);
        start = end + 1;
    }
    std::vector<std::vector<RichWord>> lines(1);
    int lineWidth = 0;
    for (const RichWord& word : words) {
        const int added = lines.back().empty() ? word.width : space + word.width;
        if (!lines.back().empty() && lineWidth + added > kTipWrapWidth) {
            lines.emplace_back();
            lineWidth = 0;
        }
        lineWidth += lines.back().empty() ? word.width : space + word.width;
        lines.back().push_back(word);
    }
    const float first = kTipTextY - kTipPitch * float(lines.size() - 1) / 2;
    for (std::size_t row = 0; row < lines.size(); ++row) {
        int total = 0;
        for (std::size_t i = 0; i < lines[row].size(); ++i) total += lines[row][i].width + (i ? space : 0);
        int x = int(std::floor(kTipX + kTipWidth / 2.0f - total / 2.0f + 0.5f));
        const float y = first + kTipPitch * float(row);
        for (std::size_t i = 0; i < lines[row].size(); ++i) {
            if (i) x += space;
            const RichWord& word = lines[row][i];
            if (word.icon) {
                bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, float(x), y - 2, float(kTipIconSize), float(kTipIconSize));
            } else {
                drawText(bottom, font, extended, word.text, float(x), y, Align::Left, {1, 1, 1, 1}, 0, true);
            }
            x += word.width;
        }
    }
}

void drawSceneChangeBottom(Canvas& bottom, Resources& resources, const std::string& continueText) {
    constexpr int kLineY = 33;
    constexpr std::uint8_t kLine[5][3] = {{1, 85, 162}, {104, 191, 255}, {207, 235, 255}, {57, 170, 255}, {0, 57, 115}};
    constexpr std::uint8_t kPanel[3] = {0, 3, 7};
    constexpr float kBadgeCentreX = 40, kBadgeCentreY = 35, kBadgeSize = 64;
    constexpr float kContinueRightX = 290, kContinueY = 223, kContinueIconX = 293, kContinueIconSize = 22;
    bottom.clear(0, 0, 0);
    for (int y = kLineY; y < kScreenHeight - 5; ++y)
        for (int x = 0; x < kBottomWidth; ++x) {
            const std::uint8_t* colour = y < kLineY + 5 ? kLine[y - kLineY] : kPanel;
            std::uint8_t* pixel = bottom.pixel(x, y);
            pixel[0] = colour[0];
            pixel[1] = colour[1];
            pixel[2] = colour[2];
            pixel[3] = 255;
        }
    const assets::Image& badge = resources.image("sprites/ui_dialog/ui_dialog_badge.btga");
    bottom.drawScaled(badge, {0, 0, badge.width, badge.height}, kBadgeCentreX - kBadgeSize / 2, kBadgeCentreY - kBadgeSize / 2, kBadgeSize, kBadgeSize);
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    drawText(bottom, font, extended, continueText, kContinueRightX, kContinueY, Align::Right, {1, 1, 1, 1}, 0, true);
    const assets::Image& icon = resources.image(kButtonA);
    bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, kContinueIconX, kContinueY - kContinueIconSize / 2, kContinueIconSize, kContinueIconSize);
}

void drawMissionCard(Canvas& bottom, Resources& resources, const std::string& title) {
    drawDialogBackground(bottom, resources);
    const assets::Image& hex = resources.image("sprites/blend/awardviewertop/images/bighexbg.bpng");
    bottom.draw(hex, {0, 0, hex.width, hex.height}, kTipHexX, kTipHexY);
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const int width = textWidth(font, extended, title, 0) + 2 * kCardPadding;
    bottom.drawNineSlice(resources.image(kDialogBoxTexture), kBoxEdge, (kBottomWidth - width) / 2, kCardY, width, kCardHeight);
    drawText(bottom, font, extended, title, kBottomWidth / 2.0f, kCardY + kCardHeight / 2.0f, Align::Centre, {1, 1, 1, 1}, 0, true);
}

void drawWarningDialog(Canvas& top, Resources& resources, const std::string& body, const std::string& confirm, const std::string& cancel) {
    top.drawNineSlice(resources.image(kDialogBoxTexture), kBoxEdge, kWarnX - kBoxMarginX, kWarnY - kBoxMarginY, kWarnWidth + 2 * kBoxMarginX, kWarnHeight + 2 * kBoxMarginY);
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const std::vector<std::string> lines = wrapText(font, extended, body, kWarnWrapWidth, 0);
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawText(top, font, extended, lines[i], kWarnTextCentreX, kWarnFirstLineY + kWarnPitch * float(i), Align::Centre, kWarnTextColour, 0, true);
    const assets::Image& iconA = resources.image(kButtonA);
    const assets::Image& iconB = resources.image(kButtonB);
    if (!cancel.empty())
        top.drawScaled(iconB, {0, 0, iconB.width, iconB.height}, kWarnCancelIconX - kWarnIconSize / 2, kWarnButtonY - kWarnIconSize / 2, kWarnIconSize, kWarnIconSize);
    drawText(top, font, extended, cancel, kWarnCancelTextX, kWarnButtonY, Align::Left, kWarnTextColour, 0, true);
    drawText(top, font, extended, confirm, kWarnConfirmTextRightX, kWarnButtonY, Align::Right, kWarnTextColour, 0, true);
    top.drawScaled(iconA, {0, 0, iconA.width, iconA.height}, kWarnConfirmIconX - kWarnIconSize / 2, kWarnButtonY - kWarnIconSize / 2, kWarnIconSize, kWarnIconSize);
}

void drawBlackVeil(Canvas& screen, float opacity) {
    if (opacity <= 0.0f) return;
    assets::Image black;
    black.width = 1;
    black.height = 1;
    black.rgba = {0, 0, 0, 255};
    screen.drawScaled(black, {0, 0, 1, 1}, 0, 0, float(screen.width()), float(screen.height()), {1, 1, 1, std::min(opacity, 1.0f)});
}

}
