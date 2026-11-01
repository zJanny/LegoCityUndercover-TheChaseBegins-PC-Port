#include "runtime/qol_pc_controls.hpp"

#include "qol/controls_reload.hpp"
#include "qol/pc_controls.hpp"
#include "runtime/qol_controls_bindings.hpp"

#include <fstream>
#include <stdexcept>

namespace chase::runtime {

using qol::ControlsPort, qol::ReloadingControls, qol::activeControls;

std::vector<Binding> readActiveBindings(const std::string& controlsFile) {
    const auto active = activeControls(controlsFile, sdlKeyResolver);
    std::ifstream file(active.path);
    if (!file) throw std::runtime_error("cannot open controls: " + active.path);
    return readBindings(file, sdlKeyResolver);
}

std::unique_ptr<ControlsPort> makeActiveControls(const std::string& controlsFile) {
    return std::make_unique<ReloadingControls>([controlsFile]() -> std::unique_ptr<ControlsPort> {
        auto active = activeControls(controlsFile, sdlKeyResolver);
        if (!active.defaults) return std::make_unique<QolControlsBindings>(active.path);
        return std::make_unique<QolControlsBindings>(active.path, std::move(*active.defaults));
    });
}

}
