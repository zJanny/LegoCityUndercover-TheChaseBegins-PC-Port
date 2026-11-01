#include "qol/controls_swap.hpp"

namespace qol {

std::optional<SwapDialog> swapDialog(const ControlsTab& tab) {
    if (!tab.conflictPending()) return std::nullopt;
    return SwapDialog{"Key already in use",
        tab.message() + ". Swap the two bindings?",
        {"Swap", "Cancel"},
        SwapButton::Cancel};
}

ControlsResult resolveSwap(ControlsTab& tab, SwapButton choice) {
    return choice == SwapButton::Swap ? tab.confirmSwap() : tab.cancelConflict();
}

}
