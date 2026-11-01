#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qol {

struct BindingEntry {
    std::string action;
    std::string source;
    std::string code;
    std::string mode;

    bool operator==(const BindingEntry& other) const {
        return action == other.action && source == other.source && code == other.code && mode == other.mode;
    }
};

enum class ControlsResult { Ignored, Waiting, Changed, Conflict, Rejected, Cancelled, Confirming };

using KeyValidator = std::function<bool(const std::string&)>;
using InputValidator = std::function<bool(const std::string& source, const std::string& code)>;

constexpr std::size_t slotsPerAction = 2;

struct AnyInput {};

class ControlsTab {
public:
    ControlsTab(std::vector<std::string> actions, std::vector<BindingEntry> bindings,
        std::vector<BindingEntry> defaults, KeyValidator validKey);
    ControlsTab(AnyInput, std::vector<std::string> actions, std::vector<BindingEntry> bindings,
        std::vector<BindingEntry> defaults, InputValidator validInput);

    std::size_t rowCount() const { return actions_.size(); }
    const std::string& rowAction(std::size_t row) const;
    std::optional<std::size_t> rowOf(const std::string& action) const;
    std::vector<std::string> rowKeys(std::size_t row) const;

    bool waiting() const { return waiting_; }
    std::size_t waitingRow() const { return waitingRow_; }
    std::size_t waitingSlot() const { return waitingSlot_; }
    const std::string& conflictAction() const { return conflictAction_; }
    const std::string& message() const { return message_; }
    bool confirming() const { return confirming_; }
    bool conflictPending() const { return pending_.has_value() && waiting_; }
    const std::string& pendingSource() const { return pendingSource_; }
    const std::string& pendingCode() const { return pendingCode_; }

    ControlsResult activate(std::size_t row, std::size_t slot);
    ControlsResult assignKey(const std::string& keyName);
    ControlsResult assignInput(const std::string& source, const std::string& code);
    ControlsResult toggleMode(std::size_t row, std::size_t slot);
    ControlsResult clearSlot(std::size_t row, std::size_t slot);
    ControlsResult cancel();
    ControlsResult confirmSwap();
    ControlsResult cancelConflict();
    ControlsResult requestReset();
    ControlsResult confirmReset();
    ControlsResult reset();

    bool changed() const { return changed_; }
    const std::vector<BindingEntry>& bindings() const { return bindings_; }

private:
    std::vector<std::size_t> slotsOf(std::size_t row) const;
    void refreshChanged();
    void clearConflict();

    std::vector<std::string> actions_;
    std::vector<BindingEntry> bindings_;
    std::vector<BindingEntry> original_;
    std::vector<BindingEntry> defaults_;
    InputValidator validInput_;
    bool waiting_ = false;
    std::size_t waitingRow_ = 0;
    std::size_t waitingSlot_ = 0;
    std::string conflictAction_;
    std::string message_;
    std::optional<std::size_t> pending_;
    std::string pendingSource_;
    std::string pendingCode_;
    bool confirming_ = false;
    bool changed_ = false;
};

}
