#include "qol/menu_model.hpp"

namespace qol {

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
        return {placeholder("Window mode")};
    case Tab::Controls:
        return {placeholder("Key bindings")};
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
Event tabMove(std::size_t tab) { return {EventKind::TabMove, tab}; }
Event tabClick(std::size_t tab) { return {EventKind::TabClick, tab}; }

void MenuModel::open() {
    open_ = true;
}

Result MenuModel::handle(const Event& event) {
    if (!open_) return Result::Ignored;
    switch (event.kind) {
    case EventKind::Up: return moveRow(-1);
    case EventKind::Down: return moveRow(1);
    case EventKind::Left:
    case EventKind::Right:
        return Result::Ignored;
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
    case EventKind::TabMove:
        return event.index >= tabCount ? Result::Rejected : Result::Ignored;
    case EventKind::TabClick: {
        if (event.index >= tabCount) return Result::Rejected;
        Tab target = static_cast<Tab>(event.index);
        if (target == tab_) return Result::Ignored;
        tab_ = target;
        return Result::TabChanged;
    }
    }
    return Result::Rejected;
}

Result MenuModel::moveRow(std::ptrdiff_t delta) {
    std::size_t count = rowsOf(tab_).size();
    if (count < 2) return Result::Ignored;
    std::size_t current = row();
    std::size_t target = delta < 0 ? (current + count - 1) % count : (current + 1) % count;
    rows_[static_cast<std::size_t>(tab_)] = target;
    return Result::RowChanged;
}

Result MenuModel::moveTab(std::ptrdiff_t delta) {
    std::size_t count = tabCount;
    std::size_t current = static_cast<std::size_t>(tab_);
    tab_ = static_cast<Tab>(delta < 0 ? (current + count - 1) % count : (current + 1) % count);
    return Result::TabChanged;
}

Result MenuModel::selectRow(std::size_t row) {
    if (row >= rowsOf(tab_).size()) return Result::Rejected;
    if (row == this->row()) return Result::Ignored;
    rows_[static_cast<std::size_t>(tab_)] = row;
    return Result::RowChanged;
}

Result MenuModel::activateRow() {
    const Row selected = rowsOf(tab_)[row()];
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
    case RowKind::Placeholder:
        return Result::Ignored;
    }
    return Result::Ignored;
}

}
