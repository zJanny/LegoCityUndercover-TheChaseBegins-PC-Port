#pragma once

#include "qol/settings.hpp"

#include <cstddef>
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

std::vector<DisplayRow> displayRows(const Settings& settings,
    const ResolutionChoices& choices);
DisplayResult activateDisplayRow(Settings& settings, std::size_t row,
    const ResolutionChoices& choices);

}
