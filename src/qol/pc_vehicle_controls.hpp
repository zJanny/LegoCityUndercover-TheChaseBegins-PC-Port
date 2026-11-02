#pragma once

#include "qol/controls_file.hpp"
#include "qol/controls_tab.hpp"

#include <optional>
#include <string>
#include <vector>

namespace qol {

inline constexpr const char* pcVehicleFileName = "controls.pc.vehicle.cfg";

std::vector<BindingEntry> pcVehicleDefaults();

std::string pcVehiclePathFor(const std::string& controlsFile);

std::string vehicleControlsPath(const std::string& controlsFile, const KeyResolver& resolveKey, const KeyNamer& nameKey);

class SeatedControls {
public:
    std::optional<bool> update(bool pcControls, bool seated);

private:
    bool vehicle_ = false;
};

}
