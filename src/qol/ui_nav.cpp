#include "qol/ui_nav.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace qol::ui {

Command commandOf(Key key) {
    switch (key) {
    case Key::Up:
    case Key::PadUp: return Command::Up;
    case Key::Down:
    case Key::PadDown: return Command::Down;
    case Key::Left:
    case Key::PadLeft: return Command::Left;
    case Key::Right:
    case Key::PadRight: return Command::Right;
    case Key::Enter:
    case Key::PadA: return Command::Confirm;
    case Key::Escape:
    case Key::PadB: return Command::Back;
    case Key::Q:
    case Key::PageUp:
    case Key::PadLB: return Command::TabPrevious;
    case Key::E:
    case Key::PageDown:
    case Key::PadRB: return Command::TabNext;
    }
    return Command::None;
}

Event keyEvent(Key key) {
    Event e;
    e.kind = EventKind::Key;
    e.key = key;
    return e;
}

namespace {

Event pointer(EventKind kind, int x, int y) {
    Event e;
    e.kind = kind;
    e.x = x;
    e.y = y;
    return e;
}

constexpr double fractionEpsilon = 1e-9;

Action make(ActionKind kind, int index = -1) {
    Action a;
    a.kind = kind;
    a.index = index;
    return a;
}

}

Event mouseMove(int x, int y) { return pointer(EventKind::MouseMove, x, y); }
Event mouseDown(int x, int y) { return pointer(EventKind::MouseDown, x, y); }
Event mouseUp(int x, int y) { return pointer(EventKind::MouseUp, x, y); }
Event rightClick(int x, int y) { return pointer(EventKind::RightClick, x, y); }

Event wheel(int notches) {
    Event e;
    e.kind = EventKind::Wheel;
    e.notches = notches;
    return e;
}

UiNav::UiNav(TextMeasure measure, Proportions proportions) : measure_(std::move(measure)), proportions_(proportions) {}

void UiNav::setWindow(int width, int height) {
    screen_.windowWidth = width;
    screen_.windowHeight = height;
    screen_.scrollOffset = clampScroll(screen_, screen_.scrollOffset, proportions_);
}

void UiNav::setTabs(std::vector<std::string> tabs) {
    screen_.tabs = std::move(tabs);
    const int count = static_cast<int>(screen_.tabs.size());
    screen_.selectedTab = count == 0 ? 0 : std::clamp(screen_.selectedTab, 0, count - 1);
}

void UiNav::selectTab(int tab) {
    const int count = static_cast<int>(screen_.tabs.size());
    if (tab >= 0 && tab < count) screen_.selectedTab = tab;
}

void UiNav::setRows(std::vector<RowSpec> rows) {
    screen_.rows = std::move(rows);
    const int count = static_cast<int>(screen_.rows.size());
    dragRow_ = -1;
    screen_.openDropdownRow = -1;
    screen_.dropdownItemCount = 0;
    screen_.dropdownFirstItem = 0;
    if (screen_.focusOnTabs || count == 0) screen_.focusRow = -1;
    else {
        screen_.focusRow = std::clamp(screen_.focusRow, 0, count - 1);
        if (!screen_.rows[screen_.focusRow].enabled) {
            const int next = nextEnabledRow(screen_.focusRow, 1);
            if (next >= 0) screen_.focusRow = next;
        }
    }
    screen_.scrollOffset = clampScroll(screen_, screen_.scrollOffset, proportions_);
}

void UiNav::setModal(Modal modal) {
    modal_ = modal;
    dragRow_ = -1;
}

void UiNav::setSliderSteps(int steps) { sliderSteps_ = steps; }

void UiNav::setDropdownInfo(std::function<DropdownInfo(int row)> info) { dropdownInfo_ = std::move(info); }

UiLayout UiNav::layout() const { return computeLayout(screen_, measure_, proportions_); }

void UiNav::focusRow(int row, bool scroll) {
    screen_.focusRow = row;
    screen_.focusOnTabs = false;
    if (scroll) screen_.scrollOffset = scrollToShowRow(screen_, row, proportions_);
}

int UiNav::nextEnabledRow(int from, int step) const {
    const int count = static_cast<int>(screen_.rows.size());
    for (int row = from + step; row >= 0 && row < count; row += step)
        if (screen_.rows[row].enabled) return row;
    return -1;
}

void UiNav::focusTabs() {
    screen_.focusRow = -1;
    screen_.focusOnTabs = true;
}

std::vector<Action> UiNav::switchTab(int tab) {
    const int count = static_cast<int>(screen_.tabs.size());
    if (count == 0) return {};
    const int wrapped = ((tab % count) + count) % count;
    if (wrapped == screen_.selectedTab) return {};
    screen_.selectedTab = wrapped;
    screen_.scrollOffset = 0;
    screen_.focusRow = screen_.focusOnTabs ? -1 : 0;
    return {make(ActionKind::TabChanged, wrapped)};
}

std::vector<Action> UiNav::changeValue(int row, int direction) {
    if (row < 0 || row >= static_cast<int>(screen_.rows.size())) return {};
    RowSpec& spec = screen_.rows[row];
    if (!spec.enabled) return {};
    if (spec.kind == RowKind::Selector || spec.kind == RowKind::Toggle) {
        Action a = make(ActionKind::ValueChanged, row);
        a.direction = direction;
        return {a};
    }
    if (spec.kind == RowKind::Slider) {
        const int steps = std::max(sliderSteps_, 1);
        const double target = std::clamp(std::round(spec.fraction * steps) + direction, 0.0, static_cast<double>(steps)) / steps;
        if (std::fabs(target - spec.fraction) < fractionEpsilon) return {};
        spec.fraction = target;
        Action a = make(ActionKind::ValueChanged, row);
        a.direction = direction;
        a.fraction = target;
        return {a};
    }
    return {};
}

bool UiNav::openDropdown(int row) {
    if (!dropdownInfo_) return false;
    const DropdownInfo info = dropdownInfo_(row);
    if (info.count <= 0) return false;
    screen_.openDropdownRow = row;
    screen_.dropdownItemCount = info.count;
    dropdownFocus_ = std::clamp(info.selected, 0, info.count - 1);
    screen_.dropdownFirstItem = 0;
    const int visible = static_cast<int>(layout().dropdown.items.size());
    if (visible > 0 && dropdownFocus_ >= visible) screen_.dropdownFirstItem = dropdownFocus_ - visible + 1;
    return true;
}

std::vector<Action> UiNav::activateRow(int row) {
    if (row < 0 || row >= static_cast<int>(screen_.rows.size())) return {};
    const RowSpec& spec = screen_.rows[row];
    if (!spec.enabled) return {};
    switch (spec.kind) {
    case RowKind::Toggle: return changeValue(row, 1);
    case RowKind::Button: return {make(ActionKind::Activate, row)};
    case RowKind::Dropdown:
        if (openDropdown(row)) return {make(ActionKind::DropdownOpened, row)};
        return {make(ActionKind::Activate, row)};
    default: return {};
    }
}

std::vector<Action> UiNav::closeDropdown(bool selectItem) {
    const int row = screen_.openDropdownRow;
    const int item = dropdownFocus_;
    screen_.openDropdownRow = -1;
    screen_.dropdownItemCount = 0;
    screen_.dropdownFirstItem = 0;
    if (selectItem) {
        Action a = make(ActionKind::DropdownSelected, row);
        a.item = item;
        return {a};
    }
    return {make(ActionKind::DropdownClosed, row)};
}

std::vector<Action> UiNav::handleDropdownCommand(Command command) {
    const int last = screen_.dropdownItemCount - 1;
    const int visible = std::max(static_cast<int>(layout().dropdown.items.size()), 1);
    switch (command) {
    case Command::Up:
    case Command::Down:
        dropdownFocus_ = std::clamp(dropdownFocus_ + (command == Command::Up ? -1 : 1), 0, last);
        if (dropdownFocus_ < screen_.dropdownFirstItem) screen_.dropdownFirstItem = dropdownFocus_;
        if (dropdownFocus_ >= screen_.dropdownFirstItem + visible) screen_.dropdownFirstItem = dropdownFocus_ - visible + 1;
        return {};
    case Command::Confirm: return closeDropdown(true);
    case Command::Back: return closeDropdown(false);
    default: return {};
    }
}

std::vector<Action> UiNav::handleCommand(Command command) {
    if (screen_.openDropdownRow >= 0) return handleDropdownCommand(command);
    const int rowCount = static_cast<int>(screen_.rows.size());
    switch (command) {
    case Command::Back: return {make(ActionKind::Close)};
    case Command::TabPrevious: return switchTab(screen_.selectedTab - 1);
    case Command::TabNext: return switchTab(screen_.selectedTab + 1);
    case Command::Up: {
        if (screen_.focusOnTabs || rowCount == 0) return {};
        const int row = nextEnabledRow(screen_.focusRow, -1);
        if (row < 0) focusTabs();
        else focusRow(row, true);
        return {};
    }
    case Command::Down: {
        const int row = nextEnabledRow(screen_.focusOnTabs ? -1 : screen_.focusRow, 1);
        if (row >= 0) focusRow(row, true);
        return {};
    }
    case Command::Left:
    case Command::Right: {
        const int direction = command == Command::Left ? -1 : 1;
        if (screen_.focusOnTabs) return switchTab(screen_.selectedTab + direction);
        return changeValue(screen_.focusRow, direction);
    }
    case Command::Confirm:
        if (screen_.focusOnTabs) return {};
        return activateRow(screen_.focusRow);
    case Command::None: return {};
    }
    return {};
}

std::vector<Action> UiNav::handleModal(const Event& event, Command command) {
    const ActionKind cancel = modal_ == Modal::Prompt ? ActionKind::PromptCancelled : ActionKind::DialogCancelled;
    if (event.kind == EventKind::Key) {
        if (event.key == Key::Escape) return {make(cancel)};
        if (modal_ == Modal::Dialog && command == Command::Confirm) return {make(ActionKind::DialogConfirmed)};
        if (modal_ != Modal::Prompt && (command == Command::Back || command == Command::Confirm)) return {make(cancel)};
        return {};
    }
    const bool mouseButton = event.kind == EventKind::MouseDown || event.kind == EventKind::RightClick;
    if (!mouseButton) return {};
    if (modal_ != Modal::Prompt && event.kind == EventKind::RightClick) return {make(cancel)};
    Action a = make(ActionKind::ModalClick);
    a.x = event.x;
    a.y = event.y;
    a.button = event.kind == EventKind::RightClick ? 3 : 1;
    return {a};
}

std::vector<Action> UiNav::dragSlider(int x) {
    const UiLayout lay = layout();
    if (dragRow_ < 0 || dragRow_ >= static_cast<int>(lay.rows.size())) return {};
    RowSpec& spec = screen_.rows[dragRow_];
    const double target = sliderFractionAt(lay.rows[dragRow_], x, std::max(sliderSteps_, 1));
    if (std::fabs(target - spec.fraction) < fractionEpsilon) return {};
    spec.fraction = target;
    Action a = make(ActionKind::ValueChanged, dragRow_);
    a.fraction = target;
    return {a};
}

std::vector<Action> UiNav::pressOnRow(const Hit& hit, int x) {
    const int row = hit.index;
    focusRow(row, false);
    const RowSpec& spec = screen_.rows[row];
    if (!spec.enabled) return {};
    switch (spec.kind) {
    case RowKind::Slider:
        if (hit.part == Part::Track) {
            dragRow_ = row;
            return dragSlider(x);
        }
        return {};
    case RowKind::Selector:
    case RowKind::Toggle:
        if (hit.part == Part::LeftArrow) return changeValue(row, -1);
        if (hit.part == Part::RightArrow || hit.part == Part::Value) return changeValue(row, 1);
        return {};
    case RowKind::Button:
    case RowKind::Dropdown: return activateRow(row);
    }
    return {};
}

std::vector<Action> UiNav::handleMouse(const Event& event) {
    const UiLayout lay = layout();
    const Hit hit = hitTest(lay, screen_, event.x, event.y);
    if (screen_.openDropdownRow >= 0) {
        if (event.kind == EventKind::MouseMove) {
            if (hit.target == Target::DropdownItem) dropdownFocus_ = hit.index;
            return {};
        }
        if (event.kind == EventKind::RightClick) return closeDropdown(false);
        if (event.kind == EventKind::MouseDown) {
            if (hit.target == Target::DropdownItem) {
                dropdownFocus_ = hit.index;
                return closeDropdown(true);
            }
            return closeDropdown(false);
        }
        return {};
    }
    switch (event.kind) {
    case EventKind::MouseMove:
        if (dragRow_ >= 0) return dragSlider(event.x);
        if (hit.target == Target::Row) focusRow(hit.index, false);
        else if (hit.target == Target::Tab) focusTabs();
        return {};
    case EventKind::MouseUp:
        dragRow_ = -1;
        return {};
    case EventKind::RightClick: return {make(ActionKind::Close)};
    case EventKind::MouseDown:
        if (hit.target == Target::Back) return {make(ActionKind::Close)};
        if (hit.target == Target::Tab) {
            focusTabs();
            return switchTab(hit.index);
        }
        if (hit.target == Target::Row) return pressOnRow(hit, event.x);
        return {};
    default: return {};
    }
}

std::vector<Action> UiNav::handle(const Event& event) {
    const Command command = event.kind == EventKind::Key ? commandOf(event.key) : Command::None;
    if (modal_ != Modal::None) return handleModal(event, command);
    switch (event.kind) {
    case EventKind::Key: return handleCommand(command);
    case EventKind::Wheel: {
        if (screen_.openDropdownRow >= 0) return {};
        const int offset = wheelScroll(screen_, event.notches, proportions_);
        if (offset == screen_.scrollOffset) return {};
        screen_.scrollOffset = offset;
        Action a = make(ActionKind::Scrolled);
        a.item = offset;
        return {a};
    }
    default: return handleMouse(event);
    }
}

}
