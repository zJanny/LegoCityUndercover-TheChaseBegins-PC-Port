#pragma once
// On-screen text of the level scripts (batch SCRIPT-30, docs/re/script-text.md): tip text, dialogue lines, the
// dialogue background, mission title and summary. The two timed objects have the layout of the original (state byte,
// fade, duration, text); the drawing is in frontend/pages. No SDL dependency.
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class TextNativeKind {
    ShowTipTextOnce,
    ShowTipTextTimed,
    ShowDialogueText,
    ShowDialogueTextTimed,
    ShowDialogBG,
    SetMissionTitle,
    SetMissionSummary,
    ShowIntroText,
    ShowLevelText,
    ShowCharacterIntro,
    ShowTally,
    ShowBossHearts,
    ShowRaceStart,
    InformationBox,
    ShowPortraitOverVehicle,
    ShowInfoPointer,
};

std::optional<TextNativeKind> textNativeKind(const std::string& lowerCaseName);
std::vector<std::string> textNativeNames();

// Number of leading script arguments of each native, an object argument counts as one.
unsigned textNativeArguments(TextNativeKind kind);

// Tip text 0x235f30: colour 0xffffad4a (r 74, g 173, b 255, a 255), default duration 5.0 s for ShowTipTextOnce (literal
// 0x2638e0) and 4.0 s when a duration of 0 is passed (0x235f30 replaces 0 by 4.0). ShowDialogueText uses 4.0 (0x26ffe4).
constexpr float kTipOnceSeconds = 5.0f;
constexpr float kTipZeroDurationSeconds = 4.0f;
constexpr float kDialogueDefaultSeconds = 4.0f;
constexpr std::uint32_t kTipColour = 0xffffad4a;
// The tip's draw alpha (float 0x51a2d4) is written by the update at 0x2de284 / 0x2de2a0: 0.2 (literal 0x2de44c) per update up while the tip
// object's state byte (0x517f08) is not 0, down while it is 0, clamped to 0..1. State 3 lasts the object's fade-out time 0.25 s (+0x18).
constexpr float kFadeStep = 0.2f;
constexpr float kTipFadeOutSeconds = 0.25f;
// Dialogue draw code 0x1cc550: alpha rises by dt * 5.0 (literal 0x1cc878) per update up to 1.0; in state 3 the alpha is the
// remaining fraction of the fade-out time +0x18 = 0.25 s (0x235e2c).
constexpr float kDialogueFadeInRate = 5.0f;
constexpr float kDialogueFadeOutSeconds = 0.25f;

// Level heading object 0x517ec8 (ShowLevelText, 0x1cdc54 -> 0x20a4a8 -> 0x23cc00): fade in +0x14 = 0.5 s, fade out +0x18 = 0.5 s, duration
// +0x1c = 5.0 s (literal 0x20a4cc), measured on the oracle: state 1 for 0.5 s, state 2 for 5.0 s of game clock, state 3, then 0.
constexpr float kHeadingFadeInSeconds = 0.5f;
constexpr float kHeadingHoldSeconds = 5.0f;
constexpr float kHeadingFadeOutSeconds = 0.5f;

struct HeadingText {
    bool active = false;
    std::uint32_t key = 0;
    float elapsed = 0;
    float alpha = 0;
};

struct TimedText {
    bool active = false;
    std::uint32_t key = 0;
    std::uint32_t character = 0;   // dialogue: row of the character table 0x372e10 (portrait and name)
    std::string speaker;           // dialogue: object the line belongs to (lower case), may be empty
    float duration = 0;
    float elapsed = 0;
    float alpha = 0;
    bool fadingOut = false;
    bool released = false;
    float fadeOutElapsed = 0;
};

class ScriptTextState {
public:
    // The tip / dialogue object of the original is restarted by every call: the text is replaced, the timer restarts and
    // an already visible text keeps its alpha (measured: no cross fade between consecutive tips).
    void showTip(std::uint32_t key, float seconds);
    void showDialogue(std::uint32_t key, std::uint32_t character, const std::string& speaker, float seconds);
    void showHeading(std::uint32_t key);
    // STAND-IN (HUD-S13): the startup scripts call ShowLevelText while the scene starts; the original shows the heading with the first tip.
    void restartHeading() { heading_.elapsed = 0; heading_.alpha = 0; }
    void setDialogueBackground(bool on) { dialogueBackground_ = on; }
    void setMissionTitle(std::uint32_t key) { missionTitle_ = key; }
    void setMissionSummary(std::uint32_t key) { missionSummary_ = key; }
    // STAND-IN: natives whose effect is not drawn only count their calls (see SCRIPT-S23).
    void record(TextNativeKind kind, const std::vector<float>& values);

    void update(float dt);

    const TimedText& tip() const { return tip_; }
    const TimedText& dialogue() const { return dialogue_; }
    const HeadingText& heading() const { return heading_; }
    bool dialogueBackground() const { return dialogueBackground_; }
    std::uint32_t missionTitle() const { return missionTitle_; }
    std::uint32_t missionSummary() const { return missionSummary_; }
    unsigned recordedCalls(TextNativeKind kind) const;

private:
    static void start(TimedText& text, std::uint32_t key, float seconds);
    static void step(TimedText& text, float dt);
    static void stepDialogue(TimedText& text, float dt);

    TimedText tip_, dialogue_;
    HeadingText heading_;
    bool dialogueBackground_ = false;
    std::uint32_t missionTitle_ = 0, missionSummary_ = 0;
    unsigned recorded_[16] = {};
};

}
