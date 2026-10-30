#pragma once

#include <functional>
#include <string>
#include <vector>

#include "qol/ui_layout.hpp"

namespace qol::ui {

enum class Key {
    Up, Down, Left, Right, Enter, Escape, Q, E, PageUp, PageDown,
    PadUp, PadDown, PadLeft, PadRight, PadA, PadB, PadLB, PadRB
};

enum class Command { None, Up, Down, Left, Right, Confirm, Back, TabPrevious, TabNext };

Command commandOf(Key key);

enum class EventKind { Key, MouseMove, MouseDown, MouseUp, RightClick, Wheel };

struct Event {
    EventKind kind = EventKind::MouseMove;
    Key key = Key::Up;
    int x = 0;
    int y = 0;
    int notches = 0;
};

// Wheel: a positive count scrolls down (rows move up), a negative count scrolls up.
// Dialog: Enter or A confirms. SwapDialog: Enter or A cancels; Esc, B and right-click cancel in both.

Event keyEvent(Key key);
Event mouseMove(int x, int y);
Event mouseDown(int x, int y);
Event mouseUp(int x, int y);
Event rightClick(int x, int y);
Event wheel(int notches);

enum class ActionKind {
    ValueChanged,
    TabChanged,
    Activate,
    Close,
    DropdownOpened,
    DropdownSelected,
    DropdownClosed,
    PromptCancelled,
    DialogCancelled,
    DialogConfirmed,
    ModalClick,
    Scrolled
};

struct Action {
    ActionKind kind = ActionKind::Close;
    int index = -1;
    int item = -1;
    int direction = 0;
    double fraction = 0;
    int x = 0;
    int y = 0;
    int button = 0;
};

enum class Modal { None, Prompt, Dialog, SwapDialog };

struct DropdownInfo {
    int count = 0;
    int selected = 0;
};

class UiNav {
public:
    explicit UiNav(TextMeasure measure, Proportions proportions = {});

    void setWindow(int width, int height);
    void setTabs(std::vector<std::string> tabs);
    void selectTab(int tab);
    void setRows(std::vector<RowSpec> rows);
    void setModal(Modal modal);
    void setSliderSteps(int steps);
    void setDropdownInfo(std::function<DropdownInfo(int row)> info);

    const Screen& screen() const { return screen_; }
    Modal modal() const { return modal_; }
    int dropdownFocus() const { return dropdownFocus_; }
    bool dragging() const { return dragRow_ >= 0; }
    UiLayout layout() const;

    std::vector<Action> handle(const Event& event);

private:
    std::vector<Action> handleCommand(Command command);
    std::vector<Action> handleDropdownCommand(Command command);
    std::vector<Action> handleModal(const Event& event, Command command);
    std::vector<Action> handleMouse(const Event& event);
    std::vector<Action> pressOnRow(const Hit& hit, int x);
    std::vector<Action> changeValue(int row, int direction);
    std::vector<Action> activateRow(int row);
    std::vector<Action> switchTab(int tab);
    std::vector<Action> dragSlider(int x);
    std::vector<Action> closeDropdown(bool selectItem);
    int nextEnabledRow(int from, int step) const;
    void focusRow(int row, bool scroll);
    void focusTabs();
    bool openDropdown(int row);

    TextMeasure measure_;
    Proportions proportions_;
    Screen screen_;
    Modal modal_ = Modal::None;
    int sliderSteps_ = 10;
    int dragRow_ = -1;
    int dropdownFocus_ = 0;
    std::function<DropdownInfo(int)> dropdownInfo_;
};

}
