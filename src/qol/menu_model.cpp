#include "qol/menu_model.hpp"

#include "qol/controls_file.hpp"

#include <utility>

namespace qol {

const char* const resetAllQuestion = "Reset all controls to their defaults?";

const char* tabName(Tab tab) {
    switch (tab) {
    case Tab::Game: return "Game";
    case Tab::Display: return "Display";
    case Tab::Controls: return "Controls";
    case Tab::QualityOfLife: return "Quality of life";
    }
    return "";
}

std::vector<Row> rowsOf(Tab tab) {
    auto placeholder = [](const char* label) { return Row{RowKind::Placeholder, label}; };
    switch (tab) {
    case Tab::Game:
        return {placeholder("SFX volume"), placeholder("Music volume"),
                placeholder("Invert camera"), placeholder("Language")};
    case Tab::Display:
    {
        std::vector<Row> rows;
        for (const DisplayRow& row : qol::displayRows(Settings{}, {}))
            rows.push_back(Row{RowKind::Display, row.label});
        return rows;
    }
    case Tab::Controls:
    {
        std::vector<Row> rows;
        for (const std::string& action : controlActions()) rows.push_back(Row{RowKind::Control, action});
        rows.push_back(Row{RowKind::ResetAll, "Reset all"});
        return rows;
    }
    case Tab::QualityOfLife: {
        std::vector<Row> rows{Row{RowKind::Master, "Master"}};
        for (const FeatureInfo& info : featureTable)
            rows.push_back(Row{RowKind::Feature, info.name, info.feature});
        return rows;
    }
    }
    return {};
}

Event eventForKey(Key key) {
    switch (key) {
    case Key::Up: case Key::PadUp: return {EventKind::Up};
    case Key::Down: case Key::PadDown: return {EventKind::Down};
    case Key::Left: case Key::PadLeft: return {EventKind::Left};
    case Key::Right: case Key::PadRight: return {EventKind::Right};
    case Key::Enter: case Key::PadA: return {EventKind::Activate};
    case Key::Escape: case Key::PadB: return {EventKind::Back};
    case Key::PageUp: case Key::PadL: return {EventKind::TabPrevious};
    case Key::PageDown: case Key::PadR: return {EventKind::TabNext};
    }
    return {EventKind::Back};
}

Event rowMove(std::size_t row) { return {EventKind::RowMove, row}; }
Event rowClick(std::size_t row) { return {EventKind::RowClick, row}; }
Event slotClick(std::size_t row, std::size_t slot) { return {EventKind::SlotClick, row, slot}; }
Event modeClick(std::size_t row, std::size_t slot) { return {EventKind::ModeClick, row, slot}; }
Event clearClick(std::size_t row, std::size_t slot) { return {EventKind::ClearClick, row, slot}; }
Event tabMove(std::size_t tab) { return {EventKind::TabMove, tab}; }
Event tabClick(std::size_t tab) { return {EventKind::TabClick, tab}; }

bool MenuModel::setDisplayModes(ResolutionChoices modes, const Resolution& desktop) {
    if (!choicesValid(modes) || !resolutionValid(desktop)) return false;
    modes_ = std::move(modes);
    desktop_ = desktop;
    return true;
}

std::vector<DisplayRow> MenuModel::displayRows() const {
    return qol::displayRows(settings_, modes_);
}

std::vector<Row> MenuModel::rows() const {
    if (tab_ != Tab::Controls || !controls_) return rowsOf(tab_);
    std::vector<Row> out;
    const ControlsTab& tab = controls_->tab();
    for (std::size_t i = 0; i < tab.rowCount(); ++i) out.push_back(Row{RowKind::Control, tab.rowAction(i)});
    out.push_back(Row{RowKind::ResetAll, "Reset all"});
    return out;
}

void MenuModel::setGame(GameOptions* game) {
    if (game) game_.emplace(*game);
    else game_.reset();
}

void MenuModel::setControls(ControlsPort* controls) {
    controls_ = controls;
    slot_ = 0;
}

std::vector<ControlsRowText> MenuModel::controlsRows() const {
    std::vector<ControlsRowText> out;
    if (!controls_) return out;
    const ControlsTab& tab = controls_->tab();
    for (std::size_t row = 0; row < tab.rowCount(); ++row) {
        ControlsRowText text{tab.rowAction(row), {}};
        std::size_t slot = 0;
        for (const BindingEntry& entry : tab.bindings()) {
            if (entry.action != text.action || slot >= slotsPerAction) continue;
            text.slots[slot++] = {true, entry.source, entry.code, entry.mode, entry.source + " " + entry.code};
        }
        out.push_back(std::move(text));
    }
    return out;
}

std::optional<WaitingSlot> MenuModel::controlsWaiting() const {
    if (!controls_ || !controls_->tab().waiting()) return std::nullopt;
    return WaitingSlot{controls_->tab().waitingRow(), controls_->tab().waitingSlot()};
}

std::string MenuModel::controlsMessage() const {
    return controls_ ? controls_->tab().message() : std::string();
}

std::string MenuModel::controlsConflictAction() const {
    return controls_ ? controls_->tab().conflictAction() : std::string();
}

bool MenuModel::controlsConfirming() const {
    return controls_ && controls_->tab().confirming();
}

Result MenuModel::fromControls(ControlsResult result) {
    switch (result) {
    case ControlsResult::Waiting: return Result::ControlsWaiting;
    case ControlsResult::Changed: return Result::ControlsChanged;
    case ControlsResult::Conflict: return Result::ControlsConflict;
    case ControlsResult::Cancelled: return Result::ControlsCancelled;
    case ControlsResult::Confirming: return Result::ControlsConfirming;
    case ControlsResult::Rejected: return Result::Rejected;
    case ControlsResult::Ignored: return Result::Ignored;
    }
    return Result::Ignored;
}

Result MenuModel::assignInput(const std::string& source, const std::string& code) {
    if (!open_ || !controls_) return Result::Ignored;
    return fromControls(controls_->assignInput(source, code));
}

void MenuModel::open() {
    open_ = true;
}

Result MenuModel::handle(const Event& event) {
    if (!open_) return Result::Ignored;
    if (controls_ && tab_ == Tab::Controls && (controls_->tab().waiting() || controls_->tab().confirming())) {
        if (event.kind == EventKind::Back) return fromControls(controls_->cancel());
        if (event.kind == EventKind::Activate && controls_->tab().confirming())
            return fromControls(controls_->confirmReset());
        return Result::Ignored;
    }
    switch (event.kind) {
    case EventKind::Up: return moveRow(-1);
    case EventKind::Down: return moveRow(1);
    case EventKind::Left: return moveSlot(-1);
    case EventKind::Right: return moveSlot(1);
    case EventKind::Activate: return activateRow();
    case EventKind::Back:
        open_ = false;
        return Result::Closed;
    case EventKind::TabNext: return moveTab(1);
    case EventKind::TabPrevious: return moveTab(-1);
    case EventKind::RowMove: return selectRow(event.index);
    case EventKind::RowClick: {
        Result selected = selectRow(event.index);
        if (selected == Result::Rejected) return selected;
        Result activated = activateRow();
        return activated == Result::Ignored ? selected : activated;
    }
    case EventKind::SlotClick: {
        Result selected = selectRow(event.index);
        if (selected == Result::Rejected) return selected;
        if (event.slot >= slotsPerAction) return Result::Rejected;
        slot_ = event.slot;
        Result activated = activateRow();
        slot_ = 0;
        return activated == Result::Ignored ? selected : activated;
    }
    case EventKind::ModeClick:
    case EventKind::ClearClick: {
        if (event.index >= rows().size()) return Result::Rejected;
        if (!controls_ || rows()[event.index].kind != RowKind::Control) return Result::Ignored;
        ControlsResult result = event.kind == EventKind::ModeClick ? controls_->toggleMode(event.index, event.slot)
                                                                  : controls_->clearSlot(event.index, event.slot);
        return fromControls(result);
    }
    case EventKind::TabMove:
        return event.index >= tabCount ? Result::Rejected : Result::Ignored;
    case EventKind::TabClick: {
        if (event.index >= tabCount) return Result::Rejected;
        Tab target = static_cast<Tab>(event.index);
        if (target == tab_) return Result::Ignored;
        tab_ = target;
        slot_ = 0;
        return Result::TabChanged;
    }
    }
    return Result::Rejected;
}

Result MenuModel::moveRow(std::ptrdiff_t delta) {
    std::size_t count = rows().size();
    if (count < 2) return Result::Ignored;
    std::size_t current = row();
    std::size_t target = delta < 0 ? (current + count - 1) % count : (current + 1) % count;
    rows_[static_cast<std::size_t>(tab_)] = target;
    slot_ = 0;
    return Result::RowChanged;
}

Result MenuModel::fromGame(GameResult result) {
    switch (result) {
    case GameResult::Changed: return Result::GameChanged;
    case GameResult::OpenedLanguages: return Result::LanguagesRequested;
    case GameResult::Rejected: return Result::Rejected;
    case GameResult::Ignored: return Result::Ignored;
    }
    return Result::Ignored;
}

Result MenuModel::activateGame() {
    if (!game_ || row() >= gameRowCount) return Result::Ignored;
    return fromGame(game_->activate(static_cast<GameRow>(row())));
}

Result MenuModel::stepGame(std::ptrdiff_t delta) {
    if (!game_ || row() >= gameRowCount) return Result::Ignored;
    return fromGame(game_->step(static_cast<GameRow>(row()), delta < 0 ? -1 : 1));
}

Result MenuModel::moveSlot(std::ptrdiff_t delta) {
    if (tab_ == Tab::Game) return stepGame(delta);
    if (tab_ != Tab::Controls || !controls_ || rows()[row()].kind != RowKind::Control) return Result::Ignored;
    if (delta < 0 ? slot_ == 0 : slot_ + 1 >= slotsPerAction) return Result::Ignored;
    slot_ = delta < 0 ? slot_ - 1 : slot_ + 1;
    return Result::SlotChanged;
}

Result MenuModel::moveTab(std::ptrdiff_t delta) {
    std::size_t count = tabCount;
    std::size_t current = static_cast<std::size_t>(tab_);
    slot_ = 0;
    tab_ = static_cast<Tab>(delta < 0 ? (current + count - 1) % count : (current + 1) % count);
    return Result::TabChanged;
}

Result MenuModel::selectRow(std::size_t row) {
    if (row >= rows().size()) return Result::Rejected;
    if (row == this->row()) return Result::Ignored;
    rows_[static_cast<std::size_t>(tab_)] = row;
    slot_ = 0;
    return Result::RowChanged;
}

Result MenuModel::activateRow() {
    const Row selected = rows()[row()];
    switch (selected.kind) {
    case RowKind::Master:
        settings_.cycleMaster();
        return Result::MasterCycled;
    case RowKind::Feature:
        if (settings_.master() != Master::Custom) {
            for (const FeatureInfo& info : featureTable)
                settings_.setSwitch(info.feature, settings_.enabled(info.feature));
            settings_.setSwitch(selected.feature, !settings_.customSwitch(selected.feature));
            settings_.setMaster(Master::Custom);
            return Result::SwitchedToCustom;
        }
        settings_.setSwitch(selected.feature, !settings_.customSwitch(selected.feature));
        return Result::FeatureToggled;
    case RowKind::Display:
        switch (activateDisplayRow(settings_, row(), modes_, desktop_)) {
        case DisplayResult::Ignored: return Result::Ignored;
        case DisplayResult::Rejected: return Result::Rejected;
        case DisplayResult::WindowModeChanged:
        case DisplayResult::ResolutionChanged: return Result::DisplayChanged;
        }
        return Result::Ignored;
    case RowKind::Control:
        if (!controls_) return Result::Ignored;
        return fromControls(controls_->activate(row(), slot_));
    case RowKind::ResetAll:
        if (!controls_) return Result::Ignored;
        return fromControls(controls_->requestReset());
    case RowKind::Placeholder:
        return tab_ == Tab::Game ? activateGame() : Result::Ignored;
    }
    return Result::Ignored;
}

}
