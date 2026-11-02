#include "qol/window_apply.hpp"

#include "qol/active_settings.hpp"

#include <algorithm>
#include <stdexcept>

namespace qol {

ResolutionChoices usableModes(const std::vector<Resolution>& reported) {
    ResolutionChoices modes;
    for (const Resolution& mode : reported)
        if (resolutionValid(mode) && std::find(modes.begin(), modes.end(), mode) == modes.end())
            modes.push_back(mode);
    std::sort(modes.begin(), modes.end(), [](const Resolution& a, const Resolution& b) {
        const long long areaA = static_cast<long long>(a.width) * a.height;
        const long long areaB = static_cast<long long>(b.width) * b.height;
        return areaA != areaB ? areaA < areaB : a.width < b.width;
    });
    return modes;
}

WindowController::WindowController(Settings settings, std::string path)
    : settings_(std::move(settings)), path_(std::move(path)) {}

WindowPlan WindowController::plan() const {
    const Resolution size = forced_ ? *forced_ : *settings_.effectiveResolution(desktop_);
    return {settings_.windowMode(), size};
}

WindowPlan WindowController::start(const std::vector<Resolution>& reported,
    const Resolution& desktop, std::optional<Resolution> forcedWindow) {
    if (!resolutionValid(desktop))
        throw std::runtime_error("desktop size " + resolutionText(desktop) +
            " is below the minimum " + resolutionText(minimumResolution));
    if (forcedWindow && !resolutionValid(*forcedWindow))
        throw std::runtime_error("--window " + resolutionText(*forcedWindow) +
            ": below the minimum " + resolutionText(minimumResolution));
    desktop_ = desktop;
    clampStoredResolution(settings_, usableModes(reported));
    forced_ = forcedWindow;
    if (forced_) settings_.setWindowMode(WindowMode::Windowed);
    return plan();
}

WindowPlan WindowController::start(const std::vector<std::pair<int, int>>& reported,
    int desktopWidth, int desktopHeight, std::optional<std::pair<int, int>> forcedWindow) {
    std::vector<Resolution> modes;
    for (const auto& mode : reported) modes.push_back({mode.first, mode.second});
    std::optional<Resolution> forced;
    if (forcedWindow) forced = Resolution{forcedWindow->first, forcedWindow->second};
    return start(modes, {desktopWidth, desktopHeight}, forced);
}

std::pair<int, int> WindowController::minimumSize() {
    return {minimumResolution.width, minimumResolution.height};
}

std::unique_ptr<WindowController> makeWindowController(const std::string& path) {
    auto controller = std::make_unique<WindowController>(loadSettings(path), path);
    setActiveSettings(controller->settings());
    return controller;
}

WindowPlan WindowController::toggle() {
    settings_.setWindowMode(settings_.windowMode() == WindowMode::Windowed
        ? WindowMode::Fullscreen : WindowMode::Windowed);
    saveSettings(path_, settings_);
    setActiveSettings(settings_);
    return plan();
}

WindowPlan WindowController::adopt(const Settings& settings) {
    if (!(settings.resolution() == settings_.resolution())) forced_.reset();
    settings_ = settings;
    return plan();
}

bool WindowController::pressed(bool keyActive) {
    const bool edge = keyActive && !keyWasActive_;
    keyWasActive_ = keyActive;
    return edge;
}

}
