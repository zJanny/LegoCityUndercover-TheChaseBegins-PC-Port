#include "qol/menu_texts.hpp"

#include <algorithm>
#include <iostream>

namespace {

int failures = 0;

void require(bool ok, const char* what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

bool has(const std::vector<qol::MenuText>& texts, const std::string& source, const std::string& text, int box) {
    return std::any_of(texts.begin(), texts.end(), [&](const qol::MenuText& item) {
        return item.source == source && item.text == text && item.box == box;
    });
}

}

int main() {
    // Pad names: SDL_GameControllerGetStringForButton / ForAxis of SDL 2.32, recorded 2026-10-29 by 20261029-qolmenu-B
    // (user decision 2026-10-29: pad button names are SDL game-controller names).
    require(qol::padInputNames().size() == 27, "SDL has 21 controller buttons and 6 axes");
    require(qol::padInputNames().front() == "a" && qol::padInputNames()[11] == "dpup" &&
            qol::padInputNames()[20] == "touchpad" && qol::padInputNames().back() == "righttrigger", "pad name order");
    require(qol::mouseButtonNames() == std::vector<std::string>({"Left", "Middle", "Right", "X1", "X2"}),
        "mouse buttons of docs/controls.md");
    require(qol::actionNames().size() == 15 && qol::actionNames().front() == "Up" &&
            qol::actionNames().back() == "Fullscreen", "actions of docs/controls.md");

    qol::MenuTextInputs inputs;
    inputs.keyNames = {"Left Shift"};
    inputs.languageNames = {"English"};
    inputs.resolutions = {"1280x720"};
    const auto texts = qol::menuTexts(inputs);

    require(has(texts, "title", "PC Settings", 0), "title");
    require(has(texts, "tab name", "Quality of life", 0), "tab name");
    require(has(texts, "tab place", "3/4", 0), "tab place of the Controls tab");
    require(has(texts, "Game tab", "SFX volume: 10", qol::namePlateWidth), "volume row");
    require(has(texts, "Game tab", "Invert camera: On", qol::namePlateWidth), "invert camera as On/Off");
    require(has(texts, "Game tab", "Invert camera: Yes", qol::namePlateWidth), "invert camera as the model words it");
    require(has(texts, "Game tab", "Language: English", qol::namePlateWidth), "language row");
    require(has(texts, "Display tab", "Window mode: fullscreen", qol::namePlateWidth), "window mode row");
    require(has(texts, "Display tab", "Resolution: 1280x720", qol::namePlateWidth), "resolution row");
    require(has(texts, "Display tab", "Resolution: desktop", qol::namePlateWidth), "resolution before a size is picked");
    require(has(texts, "Quality of life tab", "Master: Custom", qol::namePlateWidth), "master row");
    require(has(texts, "Quality of life tab", "widened_view: Off", qol::namePlateWidth), "feature row");
    require(has(texts, "Controls key", "Left Shift", qol::listBarWidth), "key name on a list bar");
    require(has(texts, "Controls mouse button", "X2", qol::listBarWidth), "mouse name on a list bar");
    require(has(texts, "Controls pad input", "leftshoulder", qol::listBarWidth), "pad name on a list bar");
    require(has(texts, "Controls action", "DebugOverlay", qol::listBarWidth), "action name on a list bar");
    require(texts.size() == 95, "number of texts");
    return failures == 0 ? 0 : 1;
}
