#include "qol/controls_merge.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>

namespace qol {

namespace {

std::string identity(const BindingEntry& entry, const KeyResolver& resolveKey) {
    if (entry.source != "key") return entry.source + ":" + entry.code;
    std::string spaced = entry.code;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    return "key:" + std::to_string(resolveKey(spaced));
}

}

std::vector<BindingEntry> addMissingActions(const std::vector<BindingEntry>& user, const std::vector<BindingEntry>& defaults,
    const KeyResolver& resolveKey) {
    std::set<std::string> userActions, userInputs;
    for (const auto& entry : user) {
        userActions.insert(entry.action);
        userInputs.insert(identity(entry, resolveKey));
    }
    std::vector<BindingEntry> merged = user;
    for (const auto& entry : defaults) {
        if (userActions.count(entry.action) != 0) continue;
        if (userInputs.count(identity(entry, resolveKey)) != 0) continue;
        merged.push_back(entry);
    }
    return merged;
}

void keepControlsFile(const std::string& path, const std::vector<BindingEntry>& defaults, const KeyResolver& resolveKey,
    const KeyNamer& nameKey) {
    if (!std::filesystem::exists(path)) {
        writeBindingFile(path, defaults, resolveKey);
        return;
    }
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open controls: " + path);
    const auto user = readBindingEntries(file, resolveKey, nameKey);
    const auto merged = addMissingActions(user, defaults, resolveKey);
    if (merged.size() != user.size()) writeBindingFile(path, merged, resolveKey);
}

}
