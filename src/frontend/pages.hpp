#pragma once
// The bottom-screen pages of the front end (docs/re/front-end.md): the alert pages of savegameflow/page_alerttext
// (StreetPass, HOME, autosave, saving), the save spinner and the slot list. No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/resources.hpp"
#include <string>
#include <vector>

namespace chase::frontend {

enum class PadSymbol { A, B };

struct PageButton {
    std::string text;
    PadSymbol symbol = PadSymbol::A;
};

struct AlertPage {
    std::string header;
    std::string body;
    std::vector<PageButton> buttons;
    float seconds = 0;
    bool secondTransition = true;
};

void drawAlertPage(Canvas& bottom, Resources& resources, const AlertPage& page);

void drawSaveSpinner(Canvas& bottom, Resources& resources, float phase, float opacity);

void drawSelectShortcut(Canvas& bottom, Resources& resources, const std::string& selectLabel);

struct SlotRow {
    int number = 1;
    std::string text;
};

void drawSlotPage(Canvas& bottom, Resources& resources, const std::string& header, const std::string& selectLabel,
                  const std::vector<SlotRow>& rows, unsigned selected, float seconds, float selectedSeconds,
                  float confirmSeconds = -1);

void drawTitleTop(Canvas& top, Resources& resources, const std::string& suffix, float level);

// Screens drawn from textures by code rather than from a .bxaml layout. Every size and position here is a STAND-IN
// fitted to oracle screenshots (docs/re/front-end.md): the drawing code of the original is not read.
void drawDialogBackground(Canvas& bottom, Resources& resources);
void drawSubtitleBubble(Canvas& bottom, Resources& resources, const std::vector<std::string>& lines);
void drawLoadingTop(Canvas& top, Resources& resources);
void drawLoadingBottom(Canvas& bottom, Resources& resources, const std::string& tip);
// Bottom screen of the level switch loading page (manager mode 6, vtable 0x361fb4): a black page with a blue line, the
// badge on its left and `continueText` with the A icon bottom right. STAND-IN TRANS-S3: sizes and colours are fitted to
// the oracle screenshot of state station-exit-loadingpage; the drawing code was not read. The top screen is drawLoadingTop.
void drawSceneChangeBottom(Canvas& bottom, Resources& resources, const std::string& continueText);
// WarningDialogBox (script native 0x2700d4): a box on the top screen over the level with the body text and the two
// buttons (B cancel left, A confirm right). Drawn by game code that the port has not read: positions are STAND-INS.
void drawWarningDialog(Canvas& top, Resources& resources, const std::string& body, const std::string& confirm, const std::string& cancel);
// Black over the whole screen with the given opacity (FadeScreen: opacity = 1 - brightness).
void drawBlackVeil(Canvas& screen, float opacity);
void drawMissionCard(Canvas& bottom, Resources& resources, const std::string& title);

}
