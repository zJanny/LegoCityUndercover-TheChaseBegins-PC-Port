// Spec: docs/design/qol-settings.md 2026-10-28T22:38:57 (window_mode, resolution), 2026-10-29T09:23:03 (fullscreen at the
// desktop size by default, display modes of at least 400x240), 2026-10-29T10:31:01 (clamp to the largest mode,
// no resolution line while the desktop size is used); F11 binding: 2026-10-28T09:37:29.
// Cases written by 20261029-qolview-B on 2026-10-29. Display mode lists and sizes are test fixtures.
#include "input.hpp"
#include "qol/active_settings.hpp"
#include "qol/window_apply.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

const std::vector<qol::Resolution> reported{
    {1920, 1080}, {640, 480}, {1280, 720}, {320, 200}, {1280, 720}, {400, 239}, {400, 240}};
const qol::Resolution desktop{1920, 1080};

std::string slurp(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

void modesAreFilteredSortedAndUnique() {
    const qol::ResolutionChoices modes = qol::usableModes(reported);
    require(modes == qol::ResolutionChoices({{400, 240}, {640, 480}, {1280, 720}, {1920, 1080}}),
        "modes below 400x240 and duplicates are dropped, the rest ascending");
    require(qol::usableModes({}).empty(), "no reported modes give no choices");
}

void defaultsAreFullscreenAtTheDesktopSize() {
    qol::WindowController controller(qol::Settings{}, "unused.cfg");
    const qol::WindowPlan plan = controller.start(reported, desktop);
    require(plan.mode == qol::WindowMode::Fullscreen && plan.size == desktop,
        "no keys: fullscreen at the desktop size (user, 2026-10-29)");
    require(!controller.settings().resolution(), "the desktop size is not stored as a resolution");
}

void storedValuesAreUsedAndClamped() {
    std::istringstream stored("master Original\nwindow_mode windowed\nresolution 1280x720\n");
    qol::WindowController own(qol::readSettings(stored), "unused.cfg");
    const qol::WindowPlan plan = own.start(reported, desktop);
    require(plan.mode == qol::WindowMode::Windowed && plan.size == qol::Resolution{1280, 720},
        "stored windowed 1280x720 is used");

    std::istringstream odd("master Original\nwindow_mode windowed\nresolution 1000x600\n");
    qol::WindowController custom(qol::readSettings(odd), "unused.cfg");
    require(custom.start(reported, desktop).size == qol::Resolution{1000, 600},
        "a stored size outside the list is kept (user, 2026-10-29)");

    std::istringstream big("master Original\nwindow_mode windowed\nresolution 3840x2160\n");
    qol::WindowController clamped(qol::readSettings(big), "unused.cfg");
    const qol::WindowPlan limited = clamped.start(reported, desktop);
    require(limited.size == desktop && *clamped.settings().resolution() == desktop,
        "a stored size above the largest mode is limited to it (user, 2026-10-29)");

    std::istringstream wide("master Original\nresolution 1920x2000\n");
    qol::WindowController oneSide(qol::readSettings(wide), "unused.cfg");
    require(oneSide.start(reported, desktop).size == desktop,
        "one side above the largest mode is enough to clamp (user, 2026-10-29)");
}

void forcedSizeWinsAndIsWindowed() {
    qol::WindowController controller(qol::Settings{}, "unused.cfg");
    const qol::WindowPlan plan = controller.start(reported, desktop, qol::Resolution{960, 600});
    require(plan.mode == qol::WindowMode::Windowed && plan.size == qol::Resolution{960, 600},
        "an explicit size starts windowed at that size");
    qol::Settings modeOnly = controller.settings();
    modeOnly.setWindowMode(qol::WindowMode::Windowed);
    require(controller.adopt(modeOnly).size == qol::Resolution{960, 600}, "a window mode change keeps the explicit size");
    qol::Settings picked = modeOnly;
    picked.setResolution({1280, 720});
    const qol::WindowPlan chosen = controller.adopt(picked);
    require(chosen.mode == qol::WindowMode::Windowed && chosen.size == qol::Resolution{1280, 720},
        "the resolution chosen in the menu wins over the explicit size (user, 2026-10-30, QOLSET-56)");
    require(controller.adopt(picked).size == qol::Resolution{1280, 720}, "and it stays");
}

void toggleSwitchesAndWritesOnlyTheModeOnce() {
    const auto path = std::filesystem::temp_directory_path() / "qol-window-apply-test.cfg";
    std::filesystem::remove(path);
    qol::WindowController controller(qol::Settings{}, path.string());
    controller.start(reported, desktop);
    const qol::WindowPlan windowed = controller.toggle();
    require(windowed.mode == qol::WindowMode::Windowed && windowed.size == desktop,
        "F11 from fullscreen gives windowed at the effective size");
    const std::string first = slurp(path);
    require(first.find("window_mode windowed") != std::string::npos &&
        first.find("resolution") == std::string::npos,
        "window_mode is written, no resolution line while the desktop size is used");
    require(controller.toggle().mode == qol::WindowMode::Fullscreen &&
        slurp(path).find("window_mode fullscreen") != std::string::npos,
        "F11 again returns to fullscreen and writes it");
    std::filesystem::remove(path);

    qol::WindowController forced(qol::Settings{}, path.string());
    forced.start(reported, desktop, qol::Resolution{960, 600});
    const qol::WindowPlan toFull = forced.toggle();
    require(toFull.mode == qol::WindowMode::Fullscreen && toFull.size == qol::Resolution{960, 600},
        "from an explicit windowed start F11 gives fullscreen");
    std::filesystem::remove(path);
}

void unwritablePathIsAnError() {
    qol::WindowController controller(qol::Settings{}, "/nonexistent-directory/qol.cfg");
    controller.start(reported, desktop);
    bool rejected = false;
    try {
        controller.toggle();
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "a failed write of the setting is an error");
}

void desktopBelowMinimumIsRejected() {
    qol::WindowController controller(qol::Settings{}, "unused.cfg");
    bool rejected = false;
    try {
        controller.start(reported, {399, 240});
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "a desktop below 400x240 stops with an error (spec 2026-10-28T22:39:39)");
}

void minimumWindowIs400x240() {
    require(qol::minimumResolution == qol::Resolution{400, 240},
        "minimum window 400x240 (user, docs/design/qol-screen-layout.md 2026-10-28T22:25:37)");
}

void keyPressIsARisingEdge() {
    qol::WindowController controller(qol::Settings{}, "unused.cfg");
    require(!controller.pressed(false), "no press while the key is up");
    require(controller.pressed(true), "the first active frame is a press");
    require(!controller.pressed(true), "a held key is not a second press");
    require(!controller.pressed(false), "release is not a press");
    require(controller.pressed(true), "the next press counts");
}

void fullscreenBindingParsesStrictly() {
    const auto resolve = [](const std::string& name) { return name == "F11" ? 1073741892 : name == "Space" ? 32 : 0; };
    std::istringstream good("Fullscreen key F11 hold\nA key Space hold\n");
    const auto bindings = chase::readBindings(good, resolve);
    require(bindings.size() == 2 && bindings[0].action == chase::Action::Fullscreen &&
        bindings[0].code == 1073741892 && bindings[0].mode == chase::Mode::Hold,
        "Fullscreen key F11 hold parses (SDLK_F11 = 1073741892)");
    for (const char* line : {"Fullscreen key F11 latch\n", "Fullscreen key F11\n", "Fullscreen key Nope hold\n",
             "FullScreen key F11 hold\n"}) {
        std::istringstream bad(line);
        bool rejected = false;
        try {
            chase::readBindings(bad, [](const std::string& name) { return name == "F11" ? 1073741892 : 0; });
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected, std::string("malformed Fullscreen line accepted: ") + line);
    }
}

}

void loadedConfigReachesTheActiveStore() {
    const auto dir = std::filesystem::temp_directory_path() / "qol-window-apply-store";
    std::filesystem::create_directories(dir);
    const auto path = dir / "qol.cfg";
    std::filesystem::remove(path);

    qol::setActiveSettings(qol::Settings{});
    qol::makeWindowController(path.string());
    require(!qol::enabled(qol::Feature::WidenedView), "a missing qol.cfg keeps Original");
    require(qol::activeSettings().master() == qol::Master::Original, "a missing qol.cfg keeps the Original master");

    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    qol::saveSettings(path.string(), improved);
    qol::makeWindowController(path.string());
    require(qol::enabled(qol::Feature::WidenedView), "a qol.cfg with the widened view on enables it");

    qol::Settings custom;
    custom.setSwitch(qol::Feature::WidenedView, true);
    custom.setMaster(qol::Master::Custom);
    qol::saveSettings(path.string(), custom);
    auto controller = qol::makeWindowController(path.string());
    require(qol::enabled(qol::Feature::WidenedView), "Custom with the switch on enables the widened view");
    controller->start(reported, desktop);
    qol::setActiveSettings(qol::Settings{});
    controller->toggle();
    require(qol::enabled(qol::Feature::WidenedView), "a saved change reaches the active store");

    std::filesystem::remove_all(dir);
    qol::setActiveSettings(qol::Settings{});
}

int main() {
    loadedConfigReachesTheActiveStore();
    modesAreFilteredSortedAndUnique();
    defaultsAreFullscreenAtTheDesktopSize();
    storedValuesAreUsedAndClamped();
    forcedSizeWinsAndIsWindowed();
    toggleSwitchesAndWritesOnlyTheModeOnce();
    unwritablePathIsAnError();
    desktopBelowMinimumIsRejected();
    minimumWindowIs400x240();
    keyPressIsARisingEdge();
    fullscreenBindingParsesStrictly();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
