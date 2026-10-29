#include "qol/controls_tab.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace qol {

namespace {

bool knownAction(const std::vector<std::string>& actions, const std::string& name) {
    return std::find(actions.begin(), actions.end(), name) != actions.end();
}

void requireKnown(const std::vector<std::string>& actions, const std::vector<BindingEntry>& list, const char* what) {
    for (const auto& entry : list)
        if (!knownAction(actions, entry.action))
            throw std::invalid_argument(std::string("Unknown action in ") + what + ": " + entry.action);
}

}

ControlsTab::ControlsTab(std::vector<std::string> actions, std::vector<BindingEntry> bindings,
    std::vector<BindingEntry> defaults, KeyValidator validKey)
    : actions_(std::move(actions)), bindings_(std::move(bindings)), original_(bindings_),
      defaults_(std::move(defaults)), validKey_(std::move(validKey)) {
    if (!validKey_) throw std::invalid_argument("Controls tab needs a key validator");
    requireKnown(actions_, bindings_, "bindings");
    requireKnown(actions_, defaults_, "defaults");
}

const std::string& ControlsTab::rowAction(std::size_t row) const {
    if (row >= actions_.size()) throw std::out_of_range("Controls tab row out of range: " + std::to_string(row));
    return actions_[row];
}

std::optional<std::size_t> ControlsTab::rowOf(const std::string& action) const {
    auto found = std::find(actions_.begin(), actions_.end(), action);
    if (found == actions_.end()) return std::nullopt;
    return static_cast<std::size_t>(found - actions_.begin());
}

std::vector<std::size_t> ControlsTab::slotsOf(std::size_t row) const {
    std::vector<std::size_t> slots;
    for (std::size_t i = 0; i < bindings_.size(); ++i)
        if (bindings_[i].action == actions_[row]) slots.push_back(i);
    return slots;
}

std::vector<std::string> ControlsTab::rowKeys(std::size_t row) const {
    if (row >= actions_.size()) throw std::out_of_range("Controls tab row out of range: " + std::to_string(row));
    std::vector<std::string> keys;
    for (auto index : slotsOf(row)) keys.push_back(bindings_[index].source + " " + bindings_[index].code);
    return keys;
}

ControlsResult ControlsTab::activate(std::size_t row, std::size_t slot) {
    if (row >= actions_.size() || slot > slotsOf(row).size()) return ControlsResult::Rejected;
    waiting_ = true;
    waitingRow_ = row;
    waitingSlot_ = slot;
    conflictAction_.clear();
    return ControlsResult::Waiting;
}

ControlsResult ControlsTab::assignKey(const std::string& keyName) {
    if (!waiting_) return ControlsResult::Ignored;
    std::string spaced = keyName;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    if (keyName.empty() || !validKey_(spaced)) return ControlsResult::Rejected;
    conflictAction_.clear();
    for (const auto& entry : bindings_)
        if (entry.source == "key" && entry.code == keyName) {
            conflictAction_ = entry.action;
            return ControlsResult::Conflict;
        }
    auto slots = slotsOf(waitingRow_);
    if (waitingSlot_ < slots.size()) bindings_[slots[waitingSlot_]].code = keyName;
    else bindings_.push_back({actions_[waitingRow_], "key", keyName, "hold"});
    waiting_ = false;
    changed_ = bindings_ != original_;
    return ControlsResult::Changed;
}

ControlsResult ControlsTab::cancel() {
    if (!waiting_) return ControlsResult::Ignored;
    waiting_ = false;
    conflictAction_.clear();
    return ControlsResult::Cancelled;
}

ControlsResult ControlsTab::reset() {
    waiting_ = false;
    conflictAction_.clear();
    if (bindings_ == defaults_) return ControlsResult::Ignored;
    bindings_ = defaults_;
    changed_ = bindings_ != original_;
    return ControlsResult::Changed;
}

}
