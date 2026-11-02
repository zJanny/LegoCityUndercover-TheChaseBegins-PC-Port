#include "runtime/qol_pc_controls.hpp"

#include "qol/controls_reload.hpp"
#include "qol/pc_controls.hpp"
#include "qol/pc_vehicle_controls.hpp"
#include "runtime/qol_controls_bindings.hpp"

#include <fstream>
#include <stdexcept>

namespace chase::runtime {

using qol::ControlsPort, qol::ControlsSet, qol::ReloadingControls, qol::activeControls, qol::activeControlsForSet, qol::vehicleControlsPath;

namespace {

std::vector<Binding> readFrom(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open controls: " + path);
    return readBindings(file, sdlKeyResolver);
}

}

std::vector<Binding> readActiveBindings(const std::string& controlsFile) {
    return readFrom(activeControls(controlsFile, sdlKeyResolver, sdlKeyNamer).path);
}

std::vector<Binding> readVehicleBindings(const std::string& controlsFile) {
    return readFrom(vehicleControlsPath(controlsFile, sdlKeyResolver, sdlKeyNamer));
}

std::unique_ptr<ControlsPort> makeActiveControls(const std::string& controlsFile) {
    return std::make_unique<ReloadingControls>([controlsFile](ControlsSet set) -> std::unique_ptr<ControlsPort> {
        auto active = activeControlsForSet(controlsFile, set, sdlKeyResolver, sdlKeyNamer);
        if (!active.defaults) return std::make_unique<QolControlsBindings>(active.path);
        return std::make_unique<QolControlsBindings>(active.path, std::move(*active.defaults));
    });
}

}
