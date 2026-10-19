#pragma once
// The mission ticker of the pause top screen (draw 0x29ec00, called by 0x300204 pass 6; docs/re/ui-screens.md): the text "<level name> / <mission
// title> / <mission summary>" scrolls left by 35 x 0.03 pixels per drawn frame (63 pixels per second at 60 draws) through the clip rectangle (25, 0, 350, 240) and restarts at x = 350. Nothing is
// drawn and the scroll does not advance while no mission title is set. No SDL dependency.
// STAND-INS (SCRIPT-S282, SCRIPT-S283): the frame time 0.03 s is inferred from the measured 1.05 pixels per vblank (0x23602c reads a clock object +0x78 that is not read); the font is the port's frontendfont (the original asks 0x220674 for font size 12 of a three-font table whose sizes are not
// read), the line top is the y 17 of 0x254d80 and the glyph spacing is 0.
#include <cstdint>
#include <string>

namespace chase::runtime {

constexpr float kPauseTickerPixelsPerSecond = 35.0f;
constexpr float kPauseTickerFrameSeconds = 0.03f;
constexpr float kPauseTickerDrawsPerSecond = 60.0f;
constexpr float kPauseTickerSpeed = kPauseTickerPixelsPerSecond * kPauseTickerFrameSeconds * kPauseTickerDrawsPerSecond;
constexpr float kPauseTickerStart = -350.0f;
constexpr float kPauseTickerY = 17.0f;
constexpr float kPauseTickerClip[4] = {25.0f, 0.0f, 350.0f, 240.0f};
constexpr std::uint32_t kPauseTickerEmptyKey = 0xefed6b90;
constexpr const char* kPauseTickerSeparator = " / ";

inline bool pauseTickerKeySet(std::uint32_t key) { return key != 0 && key != kPauseTickerEmptyKey; }

inline std::string pauseTickerText(const std::string& levelName, std::uint32_t titleKey, const std::string& title, std::uint32_t summaryKey,
                                   const std::string& summary) {
    if (!pauseTickerKeySet(titleKey)) return {};
    std::string text = levelName + kPauseTickerSeparator + title;
    if (pauseTickerKeySet(summaryKey)) text += kPauseTickerSeparator + summary;
    return text;
}

class PauseTicker {
public:
    void reset() { scroll_ = kPauseTickerStart; }
    void advance(float seconds, float textWidth) {
        scroll_ += seconds * kPauseTickerSpeed;
        if (textWidth < scroll_) scroll_ = kPauseTickerStart;
    }
    float x() const { return -scroll_; }

private:
    float scroll_ = kPauseTickerStart;
};

}
