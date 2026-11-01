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

bool mouseButton(const std::string& code) {
    return code == "Left" || code == "Middle" || code == "Right" || code == "X1" || code == "X2";
}

InputValidator keyAndMouse(KeyValidator validKey) {
    if (!validKey) throw std::invalid_argument("Controls tab needs a key validator");
    return [validKey = std::move(validKey)](const std::string& source, const std::string& code) {
        if (source == "key") return validKey(code);
        return source == "mouse" && mouseButton(code);
    };
}

}

ControlsTab::ControlsTab(std::vector<std::string> actions, std::vector<BindingEntry> bindings,
    std::vector<BindingEntry> defaults, KeyValidator validKey)
    : ControlsTab(AnyInput{}, std::move(actions), std::move(bindings), std::move(defaults), keyAndMouse(std::move(validKey))) {}

ControlsTab::ControlsTab(AnyInput, std::vector<std::string> actions, std::vector<BindingEntry> bindings,
    std::vector<BindingEntry> defaults, InputValidator validInput)
    : actions_(std::move(actions)), bindings_(std::move(bindings)), original_(bindings_),
      defaults_(std::move(defaults)), validInput_(std::move(validInput)) {
    if (!validInput_) throw std::invalid_argument("Controls tab needs an input validator");
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

void ControlsTab::refreshChanged() {
    changed_ = bindings_ != original_;
}

void ControlsTab::clearConflict() {
    conflictAction_.clear();
    message_.clear();
    pending_.reset();
    pendingSource_.clear();
    pendingCode_.clear();
}

std::vector<std::string> ControlsTab::rowKeys(std::size_t row) const {
    if (row >= actions_.size()) throw std::out_of_range("Controls tab row out of range: " + std::to_string(row));
    std::vector<std::string> keys;
    for (auto index : slotsOf(row)) keys.push_back(bindings_[index].source + " " + bindings_[index].code);
    return keys;
}

ControlsResult ControlsTab::activate(std::size_t row, std::size_t slot) {
    confirming_ = false;
    if (row >= actions_.size()) return ControlsResult::Rejected;
    auto count = slotsOf(row).size();
    if (slot > count || slot >= std::max(slotsPerAction, count)) return ControlsResult::Rejected;
    waiting_ = true;
    waitingRow_ = row;
    waitingSlot_ = slot;
    clearConflict();
    return ControlsResult::Waiting;
}

ControlsResult ControlsTab::assignKey(const std::string& keyName) {
    return assignInput("key", keyName);
}

ControlsResult ControlsTab::assignInput(const std::string& source, const std::string& code) {
    if (!waiting_) return ControlsResult::Ignored;
    if (source == "key" && code == "Escape") return cancel();
    std::string checked = code;
    if (source == "key") std::replace(checked.begin(), checked.end(), '_', ' ');
    if (code.empty() || !validInput_(source, checked)) return ControlsResult::Rejected;
    clearConflict();
    auto slots = slotsOf(waitingRow_);
    for (std::size_t index = 0; index < bindings_.size(); ++index) {
        const auto& entry = bindings_[index];
        if (entry.source == source && entry.code == code) {
            conflictAction_ = entry.action;
            message_ = source + " " + code + " is already used by " + entry.action;
            if (waitingSlot_ >= slots.size() || slots[waitingSlot_] != index) {
                pending_ = index;
                pendingSource_ = source;
                pendingCode_ = code;
            }
            return ControlsResult::Conflict;
        }
    }
    if (waitingSlot_ < slots.size()) {
        bindings_[slots[waitingSlot_]].source = source;
        bindings_[slots[waitingSlot_]].code = code;
    } else bindings_.push_back({actions_[waitingRow_], source, code, "hold"});
    waiting_ = false;
    refreshChanged();
    return ControlsResult::Changed;
}

ControlsResult ControlsTab::toggleMode(std::size_t row, std::size_t slot) {
    if (row >= actions_.size()) return ControlsResult::Rejected;
    auto slots = slotsOf(row);
    if (slot >= slots.size()) return ControlsResult::Rejected;
    waiting_ = false;
    confirming_ = false;
    pending_.reset();
    auto& mode = bindings_[slots[slot]].mode;
    mode = mode == "hold" ? "toggle" : "hold";
    refreshChanged();
    return ControlsResult::Changed;
}

ControlsResult ControlsTab::clearSlot(std::size_t row, std::size_t slot) {
    if (row >= actions_.size()) return ControlsResult::Rejected;
    auto slots = slotsOf(row);
    if (slot >= slots.size()) return ControlsResult::Rejected;
    waiting_ = false;
    confirming_ = false;
    pending_.reset();
    bindings_.erase(bindings_.begin() + static_cast<std::ptrdiff_t>(slots[slot]));
    refreshChanged();
    return ControlsResult::Changed;
}

ControlsResult ControlsTab::cancel() {
    if (!waiting_ && !confirming_) return ControlsResult::Ignored;
    waiting_ = false;
    confirming_ = false;
    clearConflict();
    return ControlsResult::Cancelled;
}

ControlsResult ControlsTab::confirmSwap() {
    if (!conflictPending()) return ControlsResult::Ignored;
    const auto other = *pending_;
    const auto slots = slotsOf(waitingRow_);
    if (waitingSlot_ < slots.size()) {
        auto& target = bindings_[slots[waitingSlot_]];
        auto& taken = bindings_[other];
        std::swap(target.source, taken.source);
        std::swap(target.code, taken.code);
    } else {
        bindings_.erase(bindings_.begin() + static_cast<std::ptrdiff_t>(other));
        bindings_.push_back({actions_[waitingRow_], pendingSource_, pendingCode_, "hold"});
    }
    waiting_ = false;
    clearConflict();
    refreshChanged();
    return ControlsResult::Changed;
}

ControlsResult ControlsTab::cancelConflict() {
    if (!conflictPending()) return ControlsResult::Ignored;
    clearConflict();
    return ControlsResult::Cancelled;
}

ControlsResult ControlsTab::requestReset() {
    waiting_ = false;
    clearConflict();
    if (bindings_ == defaults_) {
        confirming_ = false;
        return ControlsResult::Ignored;
    }
    confirming_ = true;
    return ControlsResult::Confirming;
}

ControlsResult ControlsTab::confirmReset() {
    if (!confirming_) return ControlsResult::Ignored;
    return reset();
}

ControlsResult ControlsTab::reset() {
    waiting_ = false;
    confirming_ = false;
    clearConflict();
    if (bindings_ == defaults_) return ControlsResult::Ignored;
    bindings_ = defaults_;
    refreshChanged();
    return ControlsResult::Changed;
}

}
