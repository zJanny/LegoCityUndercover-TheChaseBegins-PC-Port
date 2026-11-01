// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04, "Display changes" bullet and the answer "Instant + display confirm" (a window mode or resolution change asks "Keep these settings?" with a 15-second countdown, reverts on Esc or at the timeout, Enter confirms), answer 13 (the dialog keeps Enter = confirm) and answer 9 (no resolution change in full screen).
// Answer 20 ("1 second" singular) and answer 21 (no dialog when nothing changed). Countdown text, the restart rule (a second change keeps the first previous values) and rounding up of the seconds shown: brief of worker 20261101-qolui3-D, 2026-11-01.
#include "qol/keep_settings.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

using qol::DisplayValues;
using qol::KeepButton;
using qol::KeepOutcome;
using qol::Resolution;
using qol::WindowMode;

const Resolution desktop{1920, 1080};

struct Fixture {
    qol::Settings settings;
    std::vector<std::pair<Resolution, WindowMode>> applied;
    qol::KeepSettings keep;

    Fixture() : keep(settings, desktop, [this](const Resolution& size, WindowMode mode) { applied.emplace_back(size, mode); }) {
        settings.setWindowMode(WindowMode::Windowed);
        settings.setResolution({1280, 720});
    }

    DisplayValues values() const { return qol::displayValuesOf(settings); }
};

void countdownSeconds() {
    Fixture f;
    const auto before = f.values();
    f.settings.setResolution({1600, 900});
    f.keep.begin(before, f.values(), 100.0);
    require(f.keep.active(), "the dialog is open");
    require(f.keep.secondsLeft() == 15, "15 seconds at 0");
    require(f.keep.text() == "Keep these settings? Reverting in 15 seconds", "text at 0");
    require(f.keep.tick(100.0) == KeepOutcome::None && f.keep.secondsLeft() == 15, "no time passed");
    require(f.keep.tick(101.0) == KeepOutcome::None && f.keep.secondsLeft() == 14, "14 after one second");
    require(f.keep.tick(114.9) == KeepOutcome::None && f.keep.secondsLeft() == 1, "1 at 14.9");
    require(f.keep.text() == "Keep these settings? Reverting in 1 second", "singular at 1");
    require(f.keep.active() && f.applied.empty(), "still open, nothing reverted at 14.9");
    require(f.keep.tick(115.0) == KeepOutcome::Reverted, "reverted at 15");
    require(!f.keep.active() && f.keep.secondsLeft() == 0, "closed with 0 left");
}

void timeoutRevertsStoredValuesAndWindow() {
    Fixture f;
    const auto before = f.values();
    f.settings.setResolution({1600, 900});
    f.settings.setWindowMode(WindowMode::Fullscreen);
    f.keep.begin(before, f.values(), 0.0);
    require(f.keep.tick(15.0) == KeepOutcome::Reverted, "timeout reverts");
    require(f.values() == before, "stored mode and resolution are back");
    require(f.applied.size() == 1 && f.applied[0].first == Resolution{1280, 720} && f.applied[0].second == WindowMode::Windowed, "the window is set to the previous values once");
    require(f.keep.tick(30.0) == KeepOutcome::None && f.applied.size() == 1, "nothing more after the revert");
}

void escapeReverts() {
    Fixture f;
    const auto before = f.values();
    f.settings.setWindowMode(WindowMode::Fullscreen);
    f.keep.begin(before, f.values(), 5.0);
    require(f.keep.tick(8.0) == KeepOutcome::None, "open after 3 seconds");
    require(f.keep.cancel() == KeepOutcome::Reverted && f.values() == before && f.applied.size() == 1, "Esc reverts at once");
    require(f.keep.cancel() == KeepOutcome::None && f.applied.size() == 1, "second Esc does nothing");
}

void enterKeeps() {
    Fixture f;
    const auto before = f.values();
    f.settings.setResolution({1600, 900});
    f.keep.begin(before, f.values(), 0.0);
    require(f.keep.confirm() == KeepOutcome::Kept && !f.keep.active(), "Enter keeps");
    require(f.values().resolution == std::optional<Resolution>(Resolution{1600, 900}) && f.applied.empty(), "the new value is permanent, the window is untouched");
    require(f.keep.tick(60.0) == KeepOutcome::None && f.keep.cancel() == KeepOutcome::None && f.values().resolution == std::optional<Resolution>(Resolution{1600, 900}), "no revert later");
    require(f.keep.confirm() == KeepOutcome::None, "nothing left to confirm");
}

void secondChangeRestartsFromTheFirstPreviousValues() {
    Fixture f;
    const auto first = f.values();
    f.settings.setResolution({1600, 900});
    f.keep.begin(first, f.values(), 0.0);
    require(f.keep.tick(10.0) == KeepOutcome::None && f.keep.secondsLeft() == 5, "5 seconds left");
    const auto second = f.values();
    f.settings.setResolution({1024, 768});
    f.keep.begin(second, f.values(), 10.0);
    require(f.keep.secondsLeft() == 15, "the countdown restarts at 15");
    require(f.keep.previous() == first, "the previous values are still the first ones");
    require(f.keep.tick(24.9) == KeepOutcome::None, "the old deadline of 15 does not apply");
    require(f.keep.tick(25.0) == KeepOutcome::Reverted && f.values() == first, "revert goes to the values before the first change");
    require(f.applied.size() == 1 && f.applied[0].first == Resolution{1280, 720}, "applied once with the first previous values");
}

void unsetResolutionRevertsToTheDesktop() {
    qol::Settings settings;
    settings.setWindowMode(WindowMode::Windowed);
    std::vector<std::pair<Resolution, WindowMode>> applied;
    qol::KeepSettings keep(settings, desktop, [&](const Resolution& size, WindowMode mode) { applied.emplace_back(size, mode); });
    const auto before = qol::displayValuesOf(settings);
    settings.setWindowMode(WindowMode::Fullscreen);
    settings.setResolution({1600, 900});
    keep.begin(before, qol::displayValuesOf(settings), 0.0);
    keep.cancel();
    require(!settings.resolution() && settings.windowMode() == WindowMode::Windowed, "no stored resolution again");
    require(applied.size() == 1 && applied[0].first == desktop && applied[0].second == WindowMode::Windowed, "the window gets the desktop size");
}

void unchangedValuesOpenNoDialog() {
    Fixture f;
    f.keep.begin(f.values(), f.values(), 0.0);
    require(!f.keep.active() && !f.keep.dialog(), "nothing changed, nothing to keep");
    require(f.keep.tick(20.0) == KeepOutcome::None && f.applied.empty(), "no revert");
}

void dialogDescriptor() {
    Fixture f;
    require(!f.keep.dialog(), "no dialog while idle");
    const auto before = f.values();
    f.settings.setWindowMode(WindowMode::Fullscreen);
    f.keep.begin(before, f.values(), 0.0);
    f.keep.tick(2.5);
    auto dialog = f.keep.dialog();
    require(dialog.has_value(), "dialog while open");
    if (!dialog) return;
    require(dialog->title == "Keep these settings?" && dialog->body == "Reverting in 13 seconds", "title and body with the countdown");
    require(dialog->buttons.size() == 2 && dialog->buttons[0] == "Keep" && dialog->buttons[1] == "Revert", "buttons Keep and Revert");
    require(dialog->defaultFocus == KeepButton::Keep, "Enter keeps: the default focus is Keep (answer 13)");
    require(f.keep.choose(KeepButton::Revert) == KeepOutcome::Reverted && f.values() == before, "the Revert button reverts");
    f.settings.setWindowMode(WindowMode::Fullscreen);
    f.keep.begin(before, f.values(), 0.0);
    require(f.keep.choose(KeepButton::Keep) == KeepOutcome::Kept && f.values() != before, "the Keep button keeps");
}

void clockDoesNotRunBackwards() {
    Fixture f;
    const auto before = f.values();
    f.settings.setWindowMode(WindowMode::Fullscreen);
    f.keep.begin(before, f.values(), 50.0);
    f.keep.tick(60.0);
    f.keep.tick(40.0);
    require(f.keep.secondsLeft() == 5, "an earlier time does not add seconds");
}

void rejections() {
    auto throws = [](auto&& call) {
        try { call(); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    qol::Settings settings;
    require(throws([&] { qol::KeepSettings(settings, desktop, nullptr); }), "missing applier rejected");
    require(throws([&] { qol::KeepSettings(settings, {0, 0}, [](const Resolution&, WindowMode) {}); }), "invalid desktop rejected");
}

}

int main() {
    countdownSeconds();
    timeoutRevertsStoredValuesAndWindow();
    escapeReverts();
    enterKeeps();
    secondChangeRestartsFromTheFirstPreviousValues();
    unsetResolutionRevertsToTheDesktop();
    unchangedValuesOpenNoDialog();
    dialogDescriptor();
    clockDoesNotRunBackwards();
    rejections();
    if (failures == 0) std::cout << "qol-keep-settings: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
