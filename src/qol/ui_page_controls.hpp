#pragma once

#include <string>
#include <vector>

#include "qol/controls_port.hpp"
#include "qol/settings.hpp"
#include "qol/ui_nav.hpp"

namespace qol::ui {

enum class ControlsPageEvent {
    Nothing,
    PromptOpen,
    PromptClosed,
    Changed,
    Rejected,
    NeedsConfirm,
    NeedsSwapDialog,
    SetChanged,
    SettingsChanged
};

struct ControlsPageOutcome {
    ControlsPageEvent event = ControlsPageEvent::Nothing;
    std::string text;
    bool settingsChanged = false;
};

constexpr int controlsSlotCells = 2;
constexpr int controlsModeCell = 2;

class ControlsPage {
public:
    explicit ControlsPage(ControlsPort& port, Settings* settings = nullptr);

    void setPadConnected(bool connected) { padConnected_ = connected; }
    bool padConnected() const { return padConnected_; }

    std::vector<std::string> columnTitles() const;
    std::vector<RowSpec> rows() const;
    DropdownInfo dropdown(int row) const;
    ControlsPageOutcome handle(const Action& action);
    ControlsPageOutcome reset();
    std::string help(int row) const;
    ControlsPageOutcome resolveSwap(bool swap);

    bool promptOpen() const { return port_.tab().waiting(); }
    std::string promptText() const;
    std::string resetQuestion() const;
    std::string swapOtherAction() const { return port_.tab().conflictAction(); }
    bool hasSensitivityRow() const;
    int sensitivityRow() const { return 0; }
    int setRow() const { return hasSensitivityRow() ? 1 : 0; }
    bool hasSetRow() const { return port_.hasSets(); }
    int firstActionRow() const { return setRow() + (port_.hasSets() ? 1 : 0); }
    int resetRow() const { return firstActionRow() + static_cast<int>(port_.tab().rowCount()); }
    std::string setHint() const;

private:
    ControlsPageOutcome activateCell(int row, int cell);
    ControlsPageOutcome stepSet();
    ControlsPageOutcome changeSensitivity(const Action& action);
    RowSpec sensitivityRowSpec() const;
    ControlsPageOutcome assign(const std::string& source, const std::string& code);
    ControlsPageOutcome clearPromptSlot();
    ControlsPageOutcome cancel();
    ControlsPageOutcome toggleRowMode(std::size_t row);
    void followFirstMode(std::size_t row);

    ControlsPort& port_;
    Settings* settings_;
    bool padConnected_ = false;
};

bool isMenuKey(const std::string& keyName);

}
