#include "qol/ui_page_controls.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>

#include "qol/controls_file.hpp"

namespace qol::ui {

namespace {

const char* const promptMessage = "Press a key... (Esc cancel, Del clear)";
const char* const resetQuestionText = "Reset all controls to their defaults?";
const char* const unboundText = "Unbound";
const char* const menuKeyMessage = "That key is used by the menu";
const char* const padMessage = "No gamepad is connected";

std::string normalised(const std::string& name) {
    std::string out;
    for (char c : name)
        if (c != ' ' && c != '_') out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

std::string spaced(std::string code) {
    std::replace(code.begin(), code.end(), '_', ' ');
    return code;
}

std::string slotText(const BindingEntry& entry) {
    if (entry.source == "mouse") return "Mouse " + entry.code;
    if (entry.source == "pad") return "Pad " + entry.code;
    return spaced(entry.code);
}

std::vector<BindingEntry> bindingsOf(const ControlsTab& tab, std::size_t row) {
    std::vector<BindingEntry> out;
    for (const BindingEntry& entry : tab.bindings())
        if (entry.action == tab.rowAction(row)) out.push_back(entry);
    return out;
}

std::string modeText(const std::vector<BindingEntry>& slots) {
    return !slots.empty() && slots.front().mode == "toggle" ? "< Toggle >" : "< Hold >";
}

ControlsPageOutcome outcome(ControlsPageEvent event, std::string text = {}) { return {event, std::move(text)}; }

}

bool isMenuKey(const std::string& keyName) {
    static const char* const names[] = {"escape", "q", "e", "pageup", "pagedown", "up", "down", "left", "right",
        "return", "enter", "delete"};
    const std::string name = normalised(keyName);
    return std::find(std::begin(names), std::end(names), name) != std::end(names);
}

ControlsPage::ControlsPage(ControlsPort& port) : port_(port) {}

std::vector<std::string> ControlsPage::columnTitles() const { return {"Action", "Slot 1", "Slot 2", "Mode"}; }

std::vector<RowSpec> ControlsPage::rows() const {
    const ControlsTab& tab = port_.tab();
    std::vector<RowSpec> out;
    for (std::size_t row = 0; row < tab.rowCount(); ++row) {
        const auto slots = bindingsOf(tab, row);
        RowSpec spec;
        spec.label = tab.rowAction(row);
        spec.kind = RowKind::Button;
        for (std::size_t slot = 0; slot < slotsPerAction; ++slot)
            spec.cells.push_back(slot < slots.size() ? slotText(slots[slot]) : unboundText);
        spec.cells.push_back(modeText(slots));
        spec.value = spec.cells[0] + " | " + spec.cells[1] + " | " + spec.cells[2];
        out.push_back(std::move(spec));
    }
    RowSpec resetRow;
    resetRow.label = resetToDefaultsText;
    resetRow.value = resetToDefaultsText;
    resetRow.kind = RowKind::Button;
    out.push_back(std::move(resetRow));
    return out;
}

DropdownInfo ControlsPage::dropdown(int) const { return {}; }

std::string ControlsPage::promptText() const { return promptMessage; }

std::string ControlsPage::resetQuestion() const { return resetQuestionText; }

ControlsPageOutcome ControlsPage::cancel() {
    port_.cancel();
    return outcome(ControlsPageEvent::PromptClosed);
}

void ControlsPage::followFirstMode(std::size_t row) {
    const std::size_t count = bindingsOf(port_.tab(), row).size();
    for (std::size_t slot = 1; slot < count; ++slot) {
        const auto slots = bindingsOf(port_.tab(), row);
        if (slots[slot].mode != slots.front().mode) port_.toggleMode(row, slot);
    }
}

ControlsPageOutcome ControlsPage::toggleRowMode(std::size_t row) {
    if (bindingsOf(port_.tab(), row).empty()) return outcome(ControlsPageEvent::Nothing);
    port_.toggleMode(row, 0);
    followFirstMode(row);
    return outcome(ControlsPageEvent::Changed);
}

ControlsPageOutcome ControlsPage::activateCell(int row, int cell) {
    if (row == resetRow()) {
        if (port_.requestReset() == ControlsResult::Confirming) return outcome(ControlsPageEvent::NeedsConfirm, resetQuestionText);
        return outcome(ControlsPageEvent::Nothing);
    }
    if (row < 0 || row > resetRow()) return outcome(ControlsPageEvent::Nothing);
    const auto index = static_cast<std::size_t>(row);
    if (cell == controlsModeCell) return toggleRowMode(index);
    if (cell < 0 || cell >= controlsSlotCells) return outcome(ControlsPageEvent::Nothing);
    if (port_.activate(index, static_cast<std::size_t>(cell)) == ControlsResult::Waiting)
        return outcome(ControlsPageEvent::PromptOpen, promptMessage);
    return outcome(ControlsPageEvent::Rejected);
}

ControlsPageOutcome ControlsPage::assign(const std::string& source, const std::string& code) {
    if (!port_.tab().waiting()) return outcome(ControlsPageEvent::Nothing);
    if (source == "pad" && !padConnected_) return outcome(ControlsPageEvent::Rejected, padMessage);
    if (source == "key" && isMenuKey(code)) {
        if (normalised(code) == "escape") return cancel();
        return outcome(ControlsPageEvent::Rejected, menuKeyMessage);
    }
    const std::size_t row = port_.tab().waitingRow();
    switch (port_.assignInput(source, code)) {
    case ControlsResult::Changed:
        followFirstMode(row);
        return outcome(ControlsPageEvent::Changed);
    case ControlsResult::Conflict:
        return outcome(ControlsPageEvent::NeedsSwapDialog, port_.tab().conflictAction());
    case ControlsResult::Cancelled: return outcome(ControlsPageEvent::PromptClosed);
    case ControlsResult::Rejected: return outcome(ControlsPageEvent::Rejected);
    default: return outcome(ControlsPageEvent::Nothing);
    }
}

ControlsPageOutcome ControlsPage::clearPromptSlot() {
    if (!port_.tab().waiting()) return outcome(ControlsPageEvent::Nothing);
    const std::size_t row = port_.tab().waitingRow();
    const std::size_t slot = port_.tab().waitingSlot();
    if (port_.clearSlot(row, slot) == ControlsResult::Changed) {
        followFirstMode(row);
        return outcome(ControlsPageEvent::Changed);
    }
    return cancel();
}

ControlsPageOutcome ControlsPage::handle(const Action& action) {
    switch (action.kind) {
    case ActionKind::Activate: return activateCell(action.index, action.item < 0 ? 0 : action.item);
    case ActionKind::PromptInput: {
        const char* source = action.source == InputSource::Mouse ? "mouse" : action.source == InputSource::Pad ? "pad" : "key";
        return assign(source, action.code);
    }
    case ActionKind::ModalClick: {
        if (!port_.tab().waiting()) return outcome(ControlsPageEvent::Nothing);
        const std::string name = mouseButtonName(action.button);
        if (name.empty()) return outcome(ControlsPageEvent::Rejected);
        return assign("mouse", name);
    }
    case ActionKind::PromptClear: return clearPromptSlot();
    case ActionKind::PromptCancelled:
    case ActionKind::DialogCancelled:
        if (!port_.tab().waiting() && !port_.tab().confirming()) return outcome(ControlsPageEvent::Nothing);
        return cancel();
    default: return outcome(ControlsPageEvent::Nothing);
    }
}

ControlsPageOutcome ControlsPage::resolveSwap(bool swap) {
    if (!port_.tab().conflictPending()) return outcome(ControlsPageEvent::Nothing);
    if (!swap) {
        port_.cancelConflict();
        return outcome(ControlsPageEvent::PromptOpen, promptMessage);
    }
    const std::size_t row = port_.tab().waitingRow();
    if (port_.confirmSwap() != ControlsResult::Changed) return outcome(ControlsPageEvent::Nothing);
    followFirstMode(row);
    return outcome(ControlsPageEvent::Changed);
}

std::string ControlsPage::help(int row) const {
    if (row == resetRow()) return "Sets all bindings and modes back to the shipped controls.";
    if (row < 0 || row > resetRow()) return {};
    return "Choose a slot to bind a key, mouse or pad button; Del clears it. Default: the shipped controls";
}

ControlsPageOutcome ControlsPage::reset() {
    if (port_.confirmReset() == ControlsResult::Changed) return outcome(ControlsPageEvent::Changed);
    return outcome(ControlsPageEvent::Nothing);
}

}
