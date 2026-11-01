#pragma once

#include "qol/controls_file.hpp"
#include "qol/controls_tab.hpp"

#include <optional>
#include <string>
#include <vector>

namespace qol {

inline constexpr const char* pcControlsFileName = "controls.pc.cfg";

std::vector<BindingEntry> pcControlsDefaults();

std::string pcControlsPathFor(const std::string& controlsFile);

struct ActiveControls {
    std::string path;
    std::optional<std::vector<BindingEntry>> defaults;
};

ActiveControls activeControls(const std::string& controlsFile, bool pcControls, const KeyResolver& resolveKey);

ActiveControls activeControls(const std::string& controlsFile, const KeyResolver& resolveKey);

}
