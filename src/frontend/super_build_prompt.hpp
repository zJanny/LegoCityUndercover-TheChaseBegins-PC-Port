#pragma once
// The super build-it prompt on the top screen: sprites/blend/breakAndBuildMenu (setup 0x290b1c, filled by 0x213df4, shown by 0x1cc994 and
// hidden by 0x1cc4d4; docs/re/super-buildit.md). No SDL dependency.
// STAND-IN HUD-S73: the name and the cost are drawn white, as the oracle pictures show them; the colour call 0x224608 with 0xffffad4a on those two
// elements is not decoded. The two super brick models of the brick placeholders are not drawn (HUD-74).
#include "frontend/layout.hpp"

namespace chase::frontend {

constexpr const char* kSuperBuildPromptLayout = "sprites/blend/breakandbuildmenu/breakandbuildmenu.bxaml";
// Prompt text colour when the bricks are not enough (0x224608 with 0xff828282); white (0xffffffff) when they are.
constexpr int kSuperBuildNotEnoughGrey = 0x82;
constexpr float kSuperBuildButtonGap = 20.0f;  // 0x213df4: the A icon ends 20 pixels left of the centred prompt text

struct SuperBuildPromptView {
    std::string name;      // TextPopup text
    std::string cost;      // "%u" of the value, or "FREE"
    std::string prompt;    // "Build" or "Not enough Super Bricks!"
    std::string picture;   // tablessuperbuildit name, e.g. "objecthelipad.png"
    bool enough = false;   // A icon shown, prompt white; else grey 0x82 and no icon
    bool on = false;       // TransitionOn since `time`, else TransitionOff
    float time = 0;
};

// False once the TransitionOff storyboard has ended (nothing drawn).
bool superBuildPromptVisible(Resources& resources, const SuperBuildPromptView& view);
// The scene the layout is drawn with (texts, the picture, hidden placeholders, the A icon placed left of the prompt text).
LayoutScene superBuildPromptScene(Resources& resources, const SuperBuildPromptView& view);
void drawSuperBuildPrompt(Canvas& top, Resources& resources, const SuperBuildPromptView& view);

}
