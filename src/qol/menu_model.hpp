#pragma once

#include "qol/controls_port.hpp"
#include "qol/display_tab.hpp"
#include "qol/features.hpp"
#include "qol/settings.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace qol {

enum class Tab { Game, Display, Controls, QualityOfLife };

inline constexpr std::size_t tabCount = 4;

const char* tabName(Tab tab);

enum class RowKind { Master, Feature, Display, Control, ResetAll, Placeholder };

struct Row {
    RowKind kind;
    std::string label;
    Feature feature = Feature::Count;
};

std::vector<Row> rowsOf(Tab tab);

enum class Key {
    Up, Down, Left, Right, Enter, Escape, PageUp, PageDown,
    PadUp, PadDown, PadLeft, PadRight, PadA, PadB, PadL, PadR
};

enum class EventKind {
    Up, Down, Left, Right, Activate, Back, TabNext, TabPrevious,
    RowMove, RowClick, SlotClick, ModeClick, ClearClick, TabMove, TabClick
};

struct Event {
    EventKind kind;
    std::size_t index = 0;
    std::size_t slot = 0;
};

Event eventForKey(Key key);
Event rowMove(std::size_t row);
Event rowClick(std::size_t row);
Event slotClick(std::size_t row, std::size_t slot);
Event modeClick(std::size_t row, std::size_t slot);
Event clearClick(std::size_t row, std::size_t slot);
Event tabMove(std::size_t tab);
Event tabClick(std::size_t tab);

enum class Result {
    Ignored,
    Rejected,
    Closed,
    TabChanged,
    RowChanged,
    SlotChanged,
    MasterCycled,
    FeatureToggled,
    SwitchedToCustom,
    DisplayChanged,
    ControlsWaiting,
    ControlsChanged,
    ControlsConflict,
    ControlsCancelled,
    ControlsConfirming
};

struct ControlsSlotText {
    bool bound = false;
    std::string source;
    std::string code;
    std::string mode;
    std::string text;
};

struct ControlsRowText {
    std::string action;
    std::array<ControlsSlotText, slotsPerAction> slots;
};

struct WaitingSlot {
    std::size_t row;
    std::size_t slot;
};

extern const char* const resetAllQuestion;

class MenuModel {
public:
    explicit MenuModel(Settings& settings) : settings_(settings) {}

    bool isOpen() const { return open_; }
    const Settings& settings() const { return settings_; }
    Tab tab() const { return tab_; }
    std::size_t row() const { return rows_[static_cast<std::size_t>(tab_)]; }
    std::vector<Row> rows() const;
    bool tabActive(Tab) const { return true; }

    bool setDisplayModes(ResolutionChoices modes, const Resolution& desktop);
    std::vector<DisplayRow> displayRows() const;

    void setControls(ControlsPort* controls);
    std::vector<ControlsRowText> controlsRows() const;
    std::size_t controlsSlot() const { return slot_; }
    std::optional<WaitingSlot> controlsWaiting() const;
    std::string controlsMessage() const;
    std::string controlsConflictAction() const;
    bool controlsConfirming() const;
    Result assignInput(const std::string& source, const std::string& code);

    void open();
    Result handle(const Event& event);

private:
    Result moveRow(std::ptrdiff_t delta);
    Result moveSlot(std::ptrdiff_t delta);
    Result moveTab(std::ptrdiff_t delta);
    Result selectRow(std::size_t row);
    Result activateRow();
    Result activateControl(std::size_t row, std::size_t slot);
    Result fromControls(ControlsResult result);

    Settings& settings_;
    ControlsPort* controls_ = nullptr;
    std::size_t slot_ = 0;
    ResolutionChoices modes_;
    std::optional<Resolution> desktop_;
    bool open_ = false;
    Tab tab_ = Tab::Game;
    std::array<std::size_t, tabCount> rows_{};
};

}
