#pragma once
#include "input.hpp"
#include "qol/controls_port.hpp"

#include <memory>
#include <string>
#include <vector>

namespace chase::runtime {

using qol::ControlsPort;

std::vector<Binding> readActiveBindings(const std::string& controlsFile);

std::unique_ptr<ControlsPort> makeActiveControls(const std::string& controlsFile);

}
