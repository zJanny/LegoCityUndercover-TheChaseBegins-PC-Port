#include "runtime/script_text.hpp"

#include <algorithm>

namespace chase::runtime {

namespace {

struct Entry {
    const char* name;
    TextNativeKind kind;
    unsigned arguments;
};

constexpr Entry kEntries[] = {
    {"showtiptextonce", TextNativeKind::ShowTipTextOnce, 1},
    {"showtiptexttimed", TextNativeKind::ShowTipTextTimed, 2},
    {"showdialoguetext", TextNativeKind::ShowDialogueText, 3},
    {"showdialoguetexttimed", TextNativeKind::ShowDialogueTextTimed, 4},
    {"showdialogbg", TextNativeKind::ShowDialogBG, 1},
    {"setmissiontitle", TextNativeKind::SetMissionTitle, 1},
    {"setmissionsummary", TextNativeKind::SetMissionSummary, 1},
    {"showintrotext", TextNativeKind::ShowIntroText, 1},
    {"showleveltext", TextNativeKind::ShowLevelText, 0},
    {"showcharacterintro", TextNativeKind::ShowCharacterIntro, 1},
    {"showtally", TextNativeKind::ShowTally, 5},
    {"showbosshearts", TextNativeKind::ShowBossHearts, 2},
    {"showracestart", TextNativeKind::ShowRaceStart, 0},
    {"informationbox", TextNativeKind::InformationBox, 1},
    {"showportraitovervehicle", TextNativeKind::ShowPortraitOverVehicle, 2},
    {"showinfopointer", TextNativeKind::ShowInfoPointer, 2},
};

}

std::optional<TextNativeKind> textNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> textNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned textNativeArguments(TextNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

void ScriptTextState::start(TimedText& text, std::uint32_t key, float seconds) {
    text.active = true;
    text.key = key;
    text.duration = seconds == 0.0f ? kTipZeroDurationSeconds : seconds;
    text.elapsed = 0;
    text.fadingOut = false;
    text.released = false;
    text.fadeOutElapsed = 0;
}

void ScriptTextState::showTip(std::uint32_t key, float seconds) {
    start(tip_, key, seconds);
}

void ScriptTextState::showDialogue(std::uint32_t key, std::uint32_t character, const std::string& speaker, float seconds) {
    start(dialogue_, key, seconds);
    dialogue_.character = character;
    dialogue_.speaker = speaker;
}

void ScriptTextState::showHeading(std::uint32_t key) {
    heading_.active = true;
    heading_.key = key;
    heading_.elapsed = 0;
    heading_.alpha = 0;
}

void ScriptTextState::record(TextNativeKind kind, const std::vector<float>&) {
    ++recorded_[std::size_t(kind)];
}

unsigned ScriptTextState::recordedCalls(TextNativeKind kind) const {
    return recorded_[std::size_t(kind)];
}

void ScriptTextState::step(TimedText& text, float dt) {
    if (!text.active) return;
    text.elapsed += dt;
    if (text.released) {
        text.alpha = std::max(0.0f, text.alpha - kFadeStep);
        if (text.alpha <= 0) text.active = false;
        return;
    }
    text.alpha = std::min(1.0f, text.alpha + kFadeStep);
    if (text.fadingOut) {
        text.fadeOutElapsed += dt;
        if (text.fadeOutElapsed >= kTipFadeOutSeconds) text.released = true;
        return;
    }
    if (text.elapsed >= text.duration) text.fadingOut = true;
}

void ScriptTextState::stepDialogue(TimedText& text, float dt) {
    if (!text.active) return;
    text.elapsed += dt;
    if (text.fadingOut) {
        text.fadeOutElapsed += dt;
        text.alpha = std::max(0.0f, 1.0f - text.fadeOutElapsed / kDialogueFadeOutSeconds);
        if (text.alpha <= 0) text.active = false;
        return;
    }
    text.alpha = std::min(1.0f, text.alpha + dt * kDialogueFadeInRate);
    if (text.elapsed >= text.duration) text.fadingOut = true;
}

void ScriptTextState::update(float dt) {
    step(tip_, dt);
    stepDialogue(dialogue_, dt);
    if (!heading_.active) return;
    heading_.elapsed += dt;
    const float fadeOutStart = kHeadingFadeInSeconds + kHeadingHoldSeconds;
    if (heading_.elapsed < kHeadingFadeInSeconds) heading_.alpha = heading_.elapsed / kHeadingFadeInSeconds;
    else if (heading_.elapsed < fadeOutStart) heading_.alpha = 1.0f;
    else heading_.alpha = std::max(0.0f, 1.0f - (heading_.elapsed - fadeOutStart) / kHeadingFadeOutSeconds);
    if (heading_.elapsed >= fadeOutStart + kHeadingFadeOutSeconds) heading_.active = false;
}

}
