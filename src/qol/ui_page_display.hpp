#pragma once

#include "qol/display_tab.hpp"
#include "qol/settings.hpp"
#include "qol/ui_nav.hpp"

#include <optional>
#include <string>
#include <vector>

namespace qol::ui {

extern const char* const displayResetQuestion;
extern const char* const desktopResolutionText;

struct DisplayState {
    WindowMode windowMode = defaultWindowMode;
    std::optional<Resolution> resolution;
    bool operator==(const DisplayState& other) const {
        return windowMode == other.windowMode && resolution == other.resolution;
    }
};

struct KeepSettingsChange {
    DisplayState previous;
    DisplayState current;
};

struct DisplayPageResult {
    bool changed = false;
    std::optional<KeepSettingsChange> needsKeepSettings;
    std::optional<std::string> needsConfirm;
};

class DisplayPage {
public:
    DisplayPage(Settings& settings, ResolutionChoices choices, Resolution desktop);

    std::vector<RowSpec> rows() const;
    DropdownInfo dropdown(int row) const;
    std::vector<std::string> dropdownItems(int row) const;
    DisplayPageResult handle(const Action& action);
    DisplayPageResult reset();
    std::string help(int row) const;

    DisplayState state() const;

private:
    DisplayPageResult changeValue(const Action& action);
    DisplayPageResult chooseResolution(int item);
    DisplayPageResult keepSettings(const DisplayState& previous) const;
    std::optional<Resolution> effectiveResolution() const;

    Settings& settings_;
    ResolutionChoices choices_;
    Resolution desktop_;
};

}
