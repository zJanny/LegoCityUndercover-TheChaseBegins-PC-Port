#include "qol/display_tab.hpp"

#include <algorithm>

namespace qol {

bool choicesValid(const ResolutionChoices& choices) {
    return std::all_of(choices.begin(), choices.end(),
        [](const Resolution& choice) { return resolutionValid(choice); });
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
    const ResolutionChoices& choices) {
    const std::vector<DisplayRow> rows = displayRows(settings, choices);
    if (row >= rows.size() || !choicesValid(choices)) return DisplayResult::Rejected;
    switch (rows[row].kind) {
    case DisplayRowKind::WindowMode:
        settings.setWindowMode(settings.windowMode() == WindowMode::Windowed
            ? WindowMode::Fullscreen : WindowMode::Windowed);
        return DisplayResult::WindowModeChanged;
    case DisplayRowKind::Resolution: {
        if (choices.empty()) return DisplayResult::Ignored;
        const auto current = settings.resolution()
            ? std::find(choices.begin(), choices.end(), *settings.resolution()) : choices.end();
        const auto next = current == choices.end() || current + 1 == choices.end()
            ? choices.begin() : current + 1;
        settings.setResolution(*next);
        return DisplayResult::ResolutionChanged;
    }
    }
    return DisplayResult::Rejected;
}

}
