#include "qol/ui_page_display.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace qol::ui {

const char* const displayResetQuestion = "Reset the Display settings?";
const char* const desktopResolutionText = "Desktop resolution";

namespace {

enum Row { WindowModeRow, ResolutionRow, ViewDistanceRow, ResetRow };

const char* windowModeLabel(WindowMode mode) {
    return mode == WindowMode::Fullscreen ? "Fullscreen" : "Windowed";
}

}

DisplayPage::DisplayPage(Settings& settings, ResolutionChoices choices, Resolution desktop)
    : settings_(settings), choices_(std::move(choices)), desktop_(desktop) {
    if (!choicesValid(choices_)) throw std::invalid_argument("the Display page got an invalid resolution list");
}

DisplayState DisplayPage::state() const {
    return {settings_.windowMode(), settings_.resolution()};
}

std::optional<Resolution> DisplayPage::effectiveResolution() const {
    return settings_.effectiveResolution(desktop_);
}

std::vector<RowSpec> DisplayPage::rows() const {
    const bool fullscreen = settings_.windowMode() == WindowMode::Fullscreen;
    const std::optional<Resolution> effective = effectiveResolution();
    const std::string resolution = fullscreen || !effective
        ? desktopResolutionText : resolutionText(*effective);
    return {
        {"Window mode", RowKind::Selector, windowModeLabel(settings_.windowMode()), 0, true},
        {"Resolution", RowKind::Dropdown, resolution, 0, !fullscreen},
        viewDistanceRow(),
        {resetToDefaultsText, RowKind::Button, resetToDefaultsText, 0, true},
    };
}

DropdownInfo DisplayPage::dropdown(int row) const {
    if (row != ResolutionRow) return {};
    const std::optional<Resolution> effective = effectiveResolution();
    int selected = 0;
    if (effective) {
        const auto found = std::find(choices_.begin(), choices_.end(), *effective);
        if (found != choices_.end()) selected = static_cast<int>(found - choices_.begin());
    }
    return {static_cast<int>(choices_.size()), selected};
}

std::vector<std::string> DisplayPage::dropdownItems(int row) const {
    std::vector<std::string> items;
    if (row != ResolutionRow) return items;
    for (const Resolution& choice : choices_) items.push_back(resolutionText(choice));
    return items;
}

DisplayPageResult DisplayPage::keepSettings(const DisplayState& previous) const {
    DisplayPageResult result;
    const DisplayState current = state();
    if (previous == current) return result;
    result.changed = true;
    result.needsKeepSettings = KeepSettingsChange{previous, current};
    return result;
}

DisplayPageResult DisplayPage::chooseResolution(int item) {
    if (item < 0 || item >= static_cast<int>(choices_.size())) return {};
    const Resolution chosen = choices_[static_cast<std::size_t>(item)];
    if (effectiveResolution() == std::optional<Resolution>{chosen}) return {};
    const DisplayState previous = state();
    settings_.setResolution(chosen);
    return keepSettings(previous);
}

DisplayPageResult DisplayPage::changeValue(const Action& action) {
    if (action.index != WindowModeRow || action.direction == 0) return {};
    const DisplayState previous = state();
    activateDisplayRow(settings_, WindowModeRow, choices_);
    return keepSettings(previous);
}

DisplayPageResult DisplayPage::handle(const Action& action) {
    switch (action.kind) {
    case ActionKind::ValueChanged: return action.index == ViewDistanceRow ? changeViewDistance(action) : changeValue(action);
    case ActionKind::DropdownSelected:
        return action.index == ResolutionRow ? chooseResolution(action.item) : DisplayPageResult{};
    case ActionKind::Activate:
        if (action.index != ResetRow) return {};
        {
            DisplayPageResult result;
            result.needsConfirm = displayResetQuestion;
            return result;
        }
    default: return {};
    }
}

std::string DisplayPage::help(int row) const {
    switch (row) {
    case WindowModeRow: return "Windowed or full screen. F11 also switches. Default: Fullscreen";
    case ResolutionRow: return "Window size in Windowed mode; full screen uses the desktop. Default: Desktop resolution";
    case ViewDistanceRow: return "How far the world is drawn. Higher values load more rooms and cost speed.";
    case ResetRow: return "Sets every setting on this tab back to its default.";
    default: return {};
    }
}

DisplayPageResult DisplayPage::reset() {
    const DisplayState previous = state();
    settings_.setWindowMode(defaultWindowMode);
    settings_.clearResolution();
    const bool viewDistanceReset = resetViewDistance();
    DisplayPageResult result = keepSettings(previous);
    result.settingsChanged = viewDistanceReset;
    return result;
}

}
