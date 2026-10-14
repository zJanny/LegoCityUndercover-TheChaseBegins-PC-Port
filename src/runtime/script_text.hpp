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
// Fade measured on the oracle: the draw alpha (float at 0x51a224 + 0xb0) steps by 0.2 per game update in and out.
constexpr float kFadeStep = 0.2f;

struct TimedText {
    bool active = false;
    std::uint32_t key = 0;
    std::uint32_t character = 0;   // dialogue: row of the character table 0x372e10 (portrait and name)
    std::string speaker;           // dialogue: object the line belongs to (lower case), may be empty
    float duration = 0;
    float elapsed = 0;
    float alpha = 0;
    bool fadingOut = false;
};

class ScriptTextState {
public:
    // The tip / dialogue object of the original is restarted by every call: the text is replaced, the timer restarts and
    // an already visible text keeps its alpha (measured: no cross fade between consecutive tips).
    void showTip(std::uint32_t key, float seconds);
    void showDialogue(std::uint32_t key, std::uint32_t character, const std::string& speaker, float seconds);
    void setDialogueBackground(bool on) { dialogueBackground_ = on; }
    void setMissionTitle(std::uint32_t key) { missionTitle_ = key; }
    void setMissionSummary(std::uint32_t key) { missionSummary_ = key; }
    // STAND-IN: natives whose effect is not drawn only count their calls (see SCRIPT-S23).
    void record(TextNativeKind kind, const std::vector<float>& values);

    void update(float dt);

    const TimedText& tip() const { return tip_; }
    const TimedText& dialogue() const { return dialogue_; }
    bool dialogueBackground() const { return dialogueBackground_; }
    std::uint32_t missionTitle() const { return missionTitle_; }
    std::uint32_t missionSummary() const { return missionSummary_; }
    unsigned recordedCalls(TextNativeKind kind) const;

private:
    static void start(TimedText& text, std::uint32_t key, float seconds);
    static void step(TimedText& text, float dt);

    TimedText tip_, dialogue_;
    bool dialogueBackground_ = false;
    std::uint32_t missionTitle_ = 0, missionSummary_ = 0;
    unsigned recorded_[16] = {};
};

}
