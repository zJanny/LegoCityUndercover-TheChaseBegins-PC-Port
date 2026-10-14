#pragma once
// Script text on screen (docs/re/script-text.md): the tip text panel of the bottom screen and the dialogue box of the
// top screen. Every position and size is a STAND-IN (HUD-S8) fitted to oracle screenshots of mission 1-1; the drawing
// code 0x15b518 / 0x1cc550 of the original is only read for its alpha and timing. No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include <string>

namespace chase::frontend {

// Portrait name of a row of tablescharacters.bxls sheet 0 (column 1), "" for rows 0 and 147 (0x1b13d4).
std::string dialoguePortraitName(Resources& resources, unsigned character);
// Button icon file for a token of the hint texts ("<east>"), empty when the token is not a button.
std::string buttonIconFile(const std::string& token);

void drawTipPage(Canvas& bottom, Resources& resources, const std::string& text, float alpha);
void drawDialogueBackground(Canvas& bottom, Resources& resources);
void drawDialogueBox(Canvas& top, Resources& resources, unsigned character, const std::string& text, float alpha);

}
