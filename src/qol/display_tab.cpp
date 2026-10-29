#include "qol/display_tab.hpp"

#include <algorithm>

namespace qol {

bool choicesValid(const ResolutionChoices& choices) {
    return std::all_of(choices.begin(), choices.end(),
        [](const Resolution& choice) { return resolutionValid(choice); });
}

std::optional<Resolution> largestMode(const ResolutionChoices& choices) {
    if (choices.empty()) return std::nullopt;
    return *std::max_element(choices.begin(), choices.end(),
        [](const Resolution& a, const Resolution& b) {
            const long long areaA = static_cast<long long>(a.width) * a.height;
            const long long areaB = static_cast<long long>(b.width) * b.height;
            return areaA != areaB ? areaA < areaB : a.width < b.width;
        });
}

Resolution clampToLargest(const Resolution& resolution, const ResolutionChoices& choices) {
    const std::optional<Resolution> largest = largestMode(choices);
    if (!largest) return resolution;
    if (resolution.width > largest->width || resolution.height > largest->height) return *largest;
    return resolution;
}

bool clampStoredResolution(Settings& settings, const ResolutionChoices& choices) {
    if (!settings.resolution() || !choicesValid(choices)) return false;
    const Resolution clamped = clampToLargest(*settings.resolution(), choices);
    if (clamped == *settings.resolution()) return false;
    settings.setResolution(clamped);
    return true;
}

std::vector<DisplayRow> displayRows(const Settings& settings,
    const ResolutionChoices& choices) {
    const std::optional<Resolution>& stored = settings.resolution();
    const bool listed = stored && std::find(choices.begin(), choices.end(), *stored) != choices.end();
    return {
        DisplayRow{DisplayRowKind::WindowMode, "Window mode",
            windowModeName(settings.windowMode())},
        DisplayRow{DisplayRowKind::Resolution, "Resolution",
            stored ? resolutionText(*stored) : "desktop", stored && !listed},
    };
}

DisplayResult activateDisplayRow(Settings& settings, std::size_t row,
    const ResolutionChoices& choices, const std::optional<Resolution>& desktop) {
    const std::vector<DisplayRow> rows = displayRows(settings, choices);
    if (row >= rows.size() || !choicesValid(choices)) return DisplayResult::Rejected;
    switch (rows[row].kind) {
    case DisplayRowKind::WindowMode:
        settings.setWindowMode(settings.windowMode() == WindowMode::Windowed
            ? WindowMode::Fullscreen : WindowMode::Windowed);
        return DisplayResult::WindowModeChanged;
    case DisplayRowKind::Resolution: {
        if (choices.empty()) return DisplayResult::Ignored;
        const std::optional<Resolution> reference =
            settings.resolution() ? settings.resolution() : desktop;
        const auto current = reference
            ? std::find(choices.begin(), choices.end(), *reference) : choices.end();
        const auto next = current == choices.end() || current + 1 == choices.end()
            ? choices.begin() : current + 1;
        settings.setResolution(*next);
        return DisplayResult::ResolutionChanged;
    }
    }
    return DisplayResult::Rejected;
}

}
