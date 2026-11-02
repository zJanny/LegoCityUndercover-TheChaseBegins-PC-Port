#pragma once

#include "qol/controls_file.hpp"
#include "qol/controls_tab.hpp"

#include <string>
#include <vector>

namespace qol {

std::vector<BindingEntry> addMissingActions(const std::vector<BindingEntry>& user, const std::vector<BindingEntry>& defaults,
    const KeyResolver& resolveKey);

void keepControlsFile(const std::string& path, const std::vector<BindingEntry>& defaults, const KeyResolver& resolveKey,
    const KeyNamer& nameKey);

}
