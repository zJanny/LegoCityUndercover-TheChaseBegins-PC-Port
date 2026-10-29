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

enum class ControlsResult { Ignored, Waiting, Changed, Conflict, Rejected, Cancelled };

using KeyValidator = std::function<bool(const std::string&)>;

class ControlsTab {
public:
    ControlsTab(std::vector<std::string> actions, std::vector<BindingEntry> bindings,
        std::vector<BindingEntry> defaults, KeyValidator validKey);

    std::size_t rowCount() const { return actions_.size(); }
    const std::string& rowAction(std::size_t row) const;
    std::optional<std::size_t> rowOf(const std::string& action) const;
    std::vector<std::string> rowKeys(std::size_t row) const;

    bool waiting() const { return waiting_; }
    std::size_t waitingRow() const { return waitingRow_; }
    std::size_t waitingSlot() const { return waitingSlot_; }
    const std::string& conflictAction() const { return conflictAction_; }

    ControlsResult activate(std::size_t row, std::size_t slot);
    ControlsResult assignKey(const std::string& keyName);
    ControlsResult cancel();
    ControlsResult reset();

    bool changed() const { return changed_; }
    const std::vector<BindingEntry>& bindings() const { return bindings_; }

private:
    std::vector<std::size_t> slotsOf(std::size_t row) const;

    std::vector<std::string> actions_;
    std::vector<BindingEntry> bindings_;
    std::vector<BindingEntry> original_;
    std::vector<BindingEntry> defaults_;
    KeyValidator validKey_;
    bool waiting_ = false;
    std::size_t waitingRow_ = 0;
    std::size_t waitingSlot_ = 0;
    std::string conflictAction_;
    bool changed_ = false;
};

}
