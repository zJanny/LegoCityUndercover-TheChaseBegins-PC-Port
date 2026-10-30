#pragma once

#include "qol/settings.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace qol {

enum class DisplayRowKind { WindowMode, Resolution };

struct DisplayRow {
    DisplayRowKind kind;
    std::string label;
    std::string value;
    bool custom = false;
};

enum class DisplayResult { Ignored, Rejected, WindowModeChanged, ResolutionChanged };

using ResolutionChoices = std::vector<Resolution>;

bool choicesValid(const ResolutionChoices& choices);
std::optional<Resolution> largestMode(const ResolutionChoices& choices);
Resolution clampToLargest(const Resolution& resolution, const ResolutionChoices& choices);
bool clampStoredResolution(Settings& settings, const ResolutionChoices& choices);

std::vector<DisplayRow> displayRows(const Settings& settings,
    const ResolutionChoices& choices);
DisplayResult activateDisplayRow(Settings& settings, std::size_t row,
    const ResolutionChoices& choices, const std::optional<Resolution>& desktop = std::nullopt);

DisplayResult stepDisplayRow(Settings& settings, std::size_t row, int direction,
    const ResolutionChoices& choices, const std::optional<Resolution>& desktop = std::nullopt);

}
