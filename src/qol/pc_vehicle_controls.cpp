#include "qol/pc_vehicle_controls.hpp"

#include "qol/controls_merge.hpp"

namespace qol {

std::vector<BindingEntry> pcVehicleDefaults() {
    return {
        {"Left", "key", "A", "hold"},
        {"Left", "key", "Left", "hold"},
        {"Right", "key", "D", "hold"},
        {"Right", "key", "Right", "hold"},
        {"A", "key", "W", "hold"},
        {"A", "key", "Up", "hold"},
        {"B", "key", "S", "hold"},
        {"B", "key", "Down", "hold"},
        {"B", "key", "Space", "hold"},
        {"X", "key", "F", "hold"},
        {"Y", "key", "H", "hold"},
        {"L", "key", "Z", "hold"},
        {"L", "mouse", "Middle", "hold"},
        {"R", "key", "C", "hold"},
        {"R", "mouse", "Middle", "hold"},
        {"Start", "key", "Keypad_Enter", "hold"},
        {"Select", "key", "Tab", "hold"},
        {"Walk", "key", "Left_Ctrl", "hold"},
        {"Walk", "key", "Left_Alt", "hold"},
        {"DebugOverlay", "key", "F3", "toggle"},
        {"Fullscreen", "key", "F11", "hold"},
    };
}

std::string pcVehiclePathFor(const std::string& controlsFile) {
    const std::size_t slash = controlsFile.find_last_of('/');
    return (slash == std::string::npos ? std::string() : controlsFile.substr(0, slash + 1)) + pcVehicleFileName;
}

std::string vehicleControlsPath(const std::string& controlsFile, const KeyResolver& resolveKey, const KeyNamer& nameKey) {
    const std::string path = pcVehiclePathFor(controlsFile);
    keepControlsFile(path, pcVehicleDefaults(), resolveKey, nameKey);
    return path;
}

std::optional<bool> SeatedControls::update(bool pcControls, bool seated) {
    const bool vehicle = pcControls && seated;
    if (vehicle == vehicle_) return std::nullopt;
    vehicle_ = vehicle;
    return vehicle;
}

}
