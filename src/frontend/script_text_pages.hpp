#pragma once
// Script text on screen (docs/re/script-text.md): the tip text panel of the bottom screen and the dialogue box of the
// top screen. Box, icon and wrap geometry come from the original (0x15b518, 0x1cc550, 0x23c680, 0x23c780, 0x229128); the
// line pitch, glyph centre shift and portrait position are fitted STAND-INS (HUD-S8). No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include <cstdint>
#include <string>

namespace chase::frontend {

// Portrait name of a row of tablescharacters.bxls sheet 0 (column 1), "" for rows 0 and 147 (0x1b13d4).
std::string dialoguePortraitName(Resources& resources, unsigned character);
// Gametext key of the character's name: tablescharacters.bxls sheet 0 column 3 (struct field +8 of the 40-byte rows at 0x372e10, read by 0x1b1314); 0 when the row does not exist.
std::uint32_t characterNameKey(Resources& resources, unsigned character);
// Button icon file for a token of the hint texts ("<east>"), empty when the token is not a button.
std::string buttonIconFile(const std::string& token);

// TablesLevels sheet 0 field +8 (column 11, the key of the level name in gametext_<lang>.loc) of the row whose scene name (column 21)
// equals `levelName` ("lvl_01_cherrytreehills"); 0 when the level has no row.
std::uint32_t levelHeadingKey(Resources& resources, const std::string& levelName);

void drawTipPage(Canvas& bottom, Resources& resources, const std::string& text, float alpha);
void drawDialogueBackground(Canvas& bottom, Resources& resources, float alpha);
// Level heading box above the tip panel of the bottom screen (0x193f00, ShowLevelText).
void drawHeadingBox(Canvas& bottom, Resources& resources, const std::string& text, float alpha);
void drawDialogueBox(Canvas& top, Resources& resources, unsigned character, const std::string& text, float alpha);
// InformationBox, the dialog mode with an empty cancel text: box from the draw call 0x193c4c (0x23c680 sizes, 0x229128 box) with
// the body plus an empty line, the confirm text right aligned and the A icon (screen_selectbutton) at the bottom right; B is not offered.
void drawInformationBox(Canvas& top, Resources& resources, const std::string& body, const std::string& confirm);

}
