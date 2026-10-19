#pragma once
// The panel alpha of the bottom screen (float 0x51a2d4 = HUD object 0x519e24 + 0x4b0, docs/re/script-text.md): the HUD
// update 0x2de264..0x2de2a0 adds 0.2 (literal 0x2de44c) per update while the tip object's state byte (0x517f08, test
// 0x1ee790) is not 0 or the BlackoutMap byte (0x519e24 + 0x484, written by 0x19464c) is set, otherwise it subtracts 0.2,
// and clamps the result to 0..1. The dialogue background draw 0x2d9410 (UI_Dialog_A / UI_Dialog_B, handles +0x47c / +0x480)
// returns at alpha 0 and otherwise draws both halves white with the alpha byte (int)(alpha * 255), blended source alpha /
// one minus source alpha (record bytes 8 = 4, 9 = 5 through the factor table 0x37628c). The ShowDialogBG byte 0x51a2bd is
// not read here. No SDL dependency.

namespace chase::runtime {

constexpr float kPanelAlphaStep = 0.2f;

class DialogBackgroundFade {
public:
    void update(bool tipShown, bool mapBlackout);
    float alpha() const { return alpha_; }
    // 0x2d94a4: the alpha byte of the quad colour is truncated, as in the tip draw 0x15b564.
    float drawnAlpha() const;
    bool drawn() const { return alpha_ != 0.0f; }

private:
    float alpha_ = 0;
};

}
