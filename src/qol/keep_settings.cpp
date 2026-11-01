#include "qol/keep_settings.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace qol {

namespace {

std::string secondsText(int seconds) {
    return std::to_string(seconds) + (seconds == 1 ? " second" : " seconds");
}

}

DisplayValues displayValuesOf(const Settings& settings) {
    return {settings.windowMode(), settings.resolution()};
}

KeepSettings::KeepSettings(Settings& stored, Resolution desktop, WindowApplier applier)
    : stored_(stored), desktop_(desktop), applier_(std::move(applier)) {
    if (!applier_) throw std::invalid_argument("Keep settings needs a window applier");
    if (!resolutionValid(desktop_)) throw std::invalid_argument("Keep settings needs a valid desktop resolution");
}

void KeepSettings::begin(const DisplayValues& previous, const DisplayValues& current, double nowSeconds) {
    if (!active_) {
        if (previous == current) return;
        previous_ = previous;
        active_ = true;
    }
    deadline_ = nowSeconds + keepSettingsSeconds;
    lastNow_ = nowSeconds;
}

KeepOutcome KeepSettings::tick(double nowSeconds) {
    if (!active_) return KeepOutcome::None;
    if (nowSeconds > lastNow_) lastNow_ = nowSeconds;
    return lastNow_ >= deadline_ ? revert() : KeepOutcome::None;
}

KeepOutcome KeepSettings::confirm() {
    if (!active_) return KeepOutcome::None;
    active_ = false;
    return KeepOutcome::Kept;
}

KeepOutcome KeepSettings::cancel() {
    return active_ ? revert() : KeepOutcome::None;
}

KeepOutcome KeepSettings::choose(KeepButton button) {
    return button == KeepButton::Keep ? confirm() : cancel();
}

KeepOutcome KeepSettings::revert() {
    active_ = false;
    stored_.setWindowMode(previous_.mode);
    if (previous_.resolution) stored_.setResolution(*previous_.resolution);
    else stored_.clearResolution();
    applier_(previous_.resolution ? *previous_.resolution : desktop_, previous_.mode);
    return KeepOutcome::Reverted;
}

int KeepSettings::secondsLeft() const {
    if (!active_) return 0;
    const double left = deadline_ - lastNow_;
    if (left <= 0) return 0;
    return static_cast<int>(std::ceil(left));
}

std::string KeepSettings::text() const {
    return "Keep these settings? Reverting in " + secondsText(secondsLeft());
}

std::optional<KeepDialog> KeepSettings::dialog() const {
    if (!active_) return std::nullopt;
    return KeepDialog{"Keep these settings?", "Reverting in " + secondsText(secondsLeft()),
        {"Keep", "Revert"}, KeepButton::Keep};
}

}
