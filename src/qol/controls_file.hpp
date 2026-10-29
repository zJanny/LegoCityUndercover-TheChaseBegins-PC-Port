#pragma once

#include <functional>
#include <istream>
#include <string>
#include <vector>

#include "qol/controls_tab.hpp"

namespace qol {

using KeyResolver = std::function<int(const std::string&)>;
using KeyNamer = std::function<std::string(int)>;

std::vector<std::string> controlActions();

InputValidator fileInputValidator(KeyResolver resolveKey);

std::vector<BindingEntry> readBindingEntries(std::istream& stream, const KeyResolver& resolveKey, const KeyNamer& nameKey);

std::string formatBindingEntries(const std::vector<BindingEntry>& entries, const KeyResolver& resolveKey);

void writeBindingFile(const std::string& path, const std::vector<BindingEntry>& entries, const KeyResolver& resolveKey);

}
