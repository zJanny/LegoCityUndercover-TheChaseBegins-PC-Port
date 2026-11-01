#pragma once

#include <string>
#include <vector>

#include "qol/controls_port.hpp"
#include "qol/ui_nav.hpp"

namespace qol::ui {

enum class ControlsPageEvent {
    Nothing,
    PromptOpen,
    PromptClosed,
    Changed,
    Rejected,
    NeedsConfirm,
    NeedsSwapDialog
};

struct ControlsPageOutcome {
    ControlsPageEvent event = ControlsPageEvent::Nothing;
    std::string text;
};

constexpr int controlsSlotCells = 2;
constexpr int controlsModeCell = 2;

class ControlsPage {
public:
    explicit ControlsPage(ControlsPort& port);

    void setPadConnected(bool connected) { padConnected_ = connected; }
    bool padConnected() const { return padConnected_; }

    std::vector<std::string> columnTitles() const;
    std::vector<RowSpec> rows() const;
    DropdownInfo dropdown(int row) const;
    ControlsPageOutcome handle(const Action& action);
    ControlsPageOutcome reset();

    bool promptOpen() const { return port_.tab().waiting(); }
    std::string promptText() const;
    std::string resetQuestion() const;
    std::string swapOtherAction() const { return port_.tab().conflictAction(); }
    int resetRow() const { return static_cast<int>(port_.tab().rowCount()); }

private:
    ControlsPageOutcome activateCell(int row, int cell);
    ControlsPageOutcome assign(const std::string& source, const std::string& code);
    ControlsPageOutcome clearPromptSlot();
    ControlsPageOutcome cancel();
    ControlsPageOutcome toggleRowMode(std::size_t row);
    void followFirstMode(std::size_t row);

    ControlsPort& port_;
    bool padConnected_ = false;
};

bool isMenuKey(const std::string& keyName);

}
