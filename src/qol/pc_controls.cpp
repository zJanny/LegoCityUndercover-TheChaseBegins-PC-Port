#include "qol/pc_controls.hpp"

#include "qol/active_settings.hpp"
#include "qol/controls_merge.hpp"

namespace qol {

std::vector<BindingEntry> pcControlsDefaults() {
    return {
        {"Up", "key", "W", "hold"},
        {"Up", "key", "Up", "hold"},
        {"Down", "key", "S", "hold"},
        {"Down", "key", "Down", "hold"},
        {"Left", "key", "A", "hold"},
        {"Left", "key", "Left", "hold"},
        {"Right", "key", "D", "hold"},
        {"Right", "key", "Right", "hold"},
        {"A", "key", "E", "hold"},
        {"A", "key", "Return", "hold"},
        {"B", "key", "Space", "hold"},
        {"X", "key", "F", "hold"},
        {"Y", "mouse", "Left", "hold"},
        {"Y", "key", "R", "hold"},
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

std::string pcControlsPathFor(const std::string& controlsFile) {
    const std::size_t slash = controlsFile.find_last_of('/');
    return (slash == std::string::npos ? std::string() : controlsFile.substr(0, slash + 1)) + pcControlsFileName;
}

ActiveControls activeControls(const std::string& controlsFile, bool pcControls, const KeyResolver& resolveKey, const KeyNamer& nameKey) {
    if (!pcControls) return {controlsFile, std::nullopt};
    const std::string path = pcControlsPathFor(controlsFile);
    keepControlsFile(path, pcControlsDefaults(), resolveKey, nameKey);
    return {path, pcControlsDefaults()};
}

ActiveControls activeControls(const std::string& controlsFile, const KeyResolver& resolveKey, const KeyNamer& nameKey) {
    return activeControls(controlsFile, enabled(Feature::PcControls), resolveKey, nameKey);
}

}
