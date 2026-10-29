// Join of the Controls tab with the SDL key resolver and the file (src/runtime/qol_controls_bindings.hpp).
// Spec: docs/design/qol-settings.md entries 2026-10-29T11:06:51 (two slots, refuse, Reset all with confirm, write right after each
// change or reset) and the pad source decision of 2026-10-29. Key codes: SDL 2 SDL_GetKeyFromName, recorded 20261029-qolmenu-C.
#include "input.hpp"
#include "runtime/qol_controls_bindings.hpp"

#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

const char* const shipped =
    "# comment\n"
    "Up key W hold\n"
    "Up key Up hold\n"
    "A key Space hold\n"
    "A mouse Left hold\n"
    "Walk key Left_Alt hold\n";

const char* const shippedWritten =
    "Up key W hold\nUp key Up hold\nA key Space hold\nA mouse Left hold\nWalk key Left_Alt hold\n";

std::string slurp(const std::string& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

void spit(const std::string& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

template <class F>
bool throws(F&& call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void resolverAndNamer() {
    using namespace chase::runtime;
    require(sdlKeyResolver("Space") == SDLK_SPACE && sdlKeyResolver("Left Alt") == SDLK_LALT && sdlKeyResolver("F11") == SDLK_F11,
        "SDL key names resolve");
    require(sdlKeyResolver("Nonsense") == 0, "unknown name is 0");
    for (const char* name : {"Space", "Left Alt", "F3", "Return", "W"})
        require(sdlKeyNamer(sdlKeyResolver(name)) == name, std::string("namer inverts resolver for ") + name);
}

void changesAreWrittenAtOnce() {
    using namespace chase::runtime;
    auto directory = std::filesystem::temp_directory_path() / "chase-qol-controls-bindings-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    auto path = (directory / "controls.cfg").string();
    auto defaults = (directory / "shipped.cfg").string();
    spit(defaults, shipped);
    spit(path, shipped);
    QolControlsBindings bindings(path, defaults);
    auto up = *bindings.tab().rowOf("Up");
    auto a = *bindings.tab().rowOf("A");
    require(slurp(path) == shipped, "constructing writes nothing");

    require(bindings.activate(up, 1) == qol::ControlsResult::Waiting, "waiting for a key");
    require(bindings.assignKey("Space") == qol::ControlsResult::Conflict && slurp(path) == shipped,
        "a taken key is refused and nothing is written");
    require(bindings.assignKey("Left_Shift") == qol::ControlsResult::Changed, "rebind");
    require(slurp(path) == "Up key W hold\nUp key Left_Shift hold\nA key Space hold\nA mouse Left hold\nWalk key Left_Alt hold\n",
        "the file is written right after the change, comments dropped");

    require(bindings.toggleMode(a, 0) == qol::ControlsResult::Changed && slurp(path).find("A key Space toggle") != std::string::npos,
        "mode change written");
    require(bindings.clearSlot(a, 1) == qol::ControlsResult::Changed && slurp(path).find("A mouse") == std::string::npos,
        "clearing written");
    bindings.activate(a, 1);
    require(bindings.assignInput("pad", "leftshoulder") == qol::ControlsResult::Changed &&
            slurp(path).find("A pad leftshoulder hold\n") != std::string::npos,
        "a pad button is written");
    bindings.activate(a, 1);
    require(bindings.assignInput("pad", "South") == qol::ControlsResult::Rejected, "unknown pad name rejected while waiting");
    require(bindings.cancel() == qol::ControlsResult::Cancelled, "cancel");

    QolControlsBindings again(path, defaults);
    require(again.tab().bindings() == bindings.tab().bindings(), "the written file loads back as the same bindings");

    require(bindings.requestReset() == qol::ControlsResult::Confirming, "reset asks first");
    require(bindings.cancel() == qol::ControlsResult::Cancelled, "declined");
    require(slurp(path).find("pad leftshoulder") != std::string::npos, "declining keeps the file");
    bindings.requestReset();
    require(bindings.confirmReset() == qol::ControlsResult::Changed, "confirmed");
    require(slurp(path) == shippedWritten, "Reset all restores the shipped bindings and writes them");
    require(bindings.requestReset() == qol::ControlsResult::Ignored, "nothing to reset");
    std::filesystem::remove_all(directory);
}

void shippedCopyNextToTheFile() {
    using namespace chase::runtime;
    auto directory = std::filesystem::temp_directory_path() / "chase-qol-controls-bindings-test-next";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    auto path = (directory / "controls.cfg").string();
    spit((directory / "controls.default.cfg").string(), shipped);
    spit(path, "A key Space hold\n");
    QolControlsBindings bindings(path);
    bindings.requestReset();
    require(bindings.confirmReset() == qol::ControlsResult::Changed && slurp(path) == shippedWritten, "Reset all reads controls.default.cfg next to the file");
    require(slurp((directory / "controls.default.cfg").string()) == shipped, "the shipped copy is never written");
    std::filesystem::remove((directory / "controls.default.cfg"));
    require(throws([&] { QolControlsBindings again(path); }), "missing shipped copy");
    std::filesystem::remove_all(directory);
}

void rejections() {
    using namespace chase::runtime;
    auto directory = std::filesystem::temp_directory_path() / "chase-qol-controls-bindings-test-bad";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    auto good = (directory / "good.cfg").string();
    auto missing = (directory / "missing.cfg").string();
    spit(good, shipped);
    require(throws([&] { QolControlsBindings(missing, good); }), "missing controls file");
    require(throws([&] { QolControlsBindings(good, missing); }), "missing shipped file");
    for (const char* text : {"Up pad South hold\n", "Up pad A hold\n", "Up key Nonsense hold\n", "Up joystick a hold\n", "# nothing\n"}) {
        auto bad = (directory / "bad.cfg").string();
        spit(bad, text);
        require(throws([&] { QolControlsBindings(bad, good); }), std::string("bad controls file rejected: ") + text);
        require(throws([&] { QolControlsBindings(good, bad); }), std::string("bad shipped file rejected: ") + text);
    }
    std::filesystem::remove_all(directory);
}

}

int main() {
    resolverAndNamer();
    changesAreWrittenAtOnce();
    shippedCopyNextToTheFile();
    rejections();
    if (failures == 0) std::cout << "qol-controls-bindings: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
