#pragma once

#include "qol/controls_tab.hpp"

#include <optional>
#include <string>
#include <vector>

namespace qol {

enum class SwapButton { Swap, Cancel };

struct SwapDialog {
    std::string title;
    std::string body;
    std::vector<std::string> buttons;
    SwapButton defaultFocus;
};

std::optional<SwapDialog> swapDialog(const ControlsTab& tab);
ControlsResult resolveSwap(ControlsTab& tab, SwapButton choice);

}
