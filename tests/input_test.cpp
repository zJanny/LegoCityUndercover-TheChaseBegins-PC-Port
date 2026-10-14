#include "input.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace chase;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        InputState state({
            {Action::A, Source::Key, 1, Mode::Hold},
            {Action::A, Source::Mouse, 1, Mode::Hold},
            {Action::B, Source::Key, 2, Mode::Toggle},
            {Action::Left, Source::Key, 3, Mode::Hold},
            {Action::Right, Source::Key, 4, Mode::Hold},
            {Action::Up, Source::Key, 5, Mode::Hold},
            {Action::Down, Source::Key, 6, Mode::Hold},
            {Action::X, Source::Key, 7, Mode::Hold},
            {Action::Walk, Source::Key, 8, Mode::Hold}
        });
        state.event(Source::Key, 1, true);
        state.event(Source::Mouse, 1, true);
        state.event(Source::Key, 1, false);
        require(state.active(Action::A), "One release cleared another held binding");
        state.event(Source::Mouse, 1, false);
        require(!state.active(Action::A), "Final release failed");
        state.event(Source::Key, 2, true);
        state.event(Source::Key, 2, true, true);
        state.event(Source::Key, 2, true);
        require(state.active(Action::B), "Repeat toggled a latched action");
        state.event(Source::Key, 2, false);
        require(state.active(Action::B), "Release cleared toggle");
        state.event(Source::Key, 2, true);
        require(!state.active(Action::B), "Second press did not unlatch");
        state.event(Source::Key, 2, false);
        state.event(Source::Key, 2, true);
        state.event(Source::Key, 1, true);
        state.reset();
        require(!state.active(Action::A) && !state.active(Action::B), "Focus reset failed");
        state.event(Source::Key, 8, true);
        require(state.active(Action::Walk) && !state.active(Action::X), "Walk is a separate action from X");
        state.event(Source::Key, 8, false);
        state.event(Source::Key, 7, true);
        require(state.active(Action::X) && !state.active(Action::Walk), "X does not trigger Walk");
        state.reset();
        require(!state.active(Action::Walk) && !state.active(Action::X), "Focus reset cleared Walk and X");
        state.event(Source::Key, 3, true);
        require(state.horizontal() == -1, "Left axis");
        state.event(Source::Key, 4, true);
        require(state.horizontal() == 0, "Opposing horizontal inputs did not cancel");
        state.event(Source::Key, 5, true);
        state.event(Source::Key, 6, true);
        require(state.vertical() == 0, "Opposing vertical inputs did not cancel");
        auto view = fitViewport(0, 0, 800, 500);
        require(view.width == 666 && view.height == 500 && view.x == 67, "Aspect fit");
        TouchPoint point{};
        require(mapTouch(view, view.x, view.y, point) && point.x == 0 && point.y == 0,
            "Touch origin");
        require(mapTouch(view, view.x + view.width - 1, view.y + view.height - 1, point)
            && point.x == 319 && point.y == 239, "Touch final pixel");
        require(!mapTouch(view, view.x - 1, view.y, point), "Outside touch accepted");
        require(!mapTouch(view, view.x + view.width, view.y, point), "Right boundary accepted");
        require(!mapTouch({0, 0, 0, 0}, 0, 0, point), "Empty viewport accepted");
        std::istringstream config("A key Space hold\nB mouse X1 toggle # comment\n");
        auto bindings = readBindings(config, [](const std::string& key) {
            return key == "Space" ? 32 : 0;
        });
        require(bindings.size() == 2 && bindings[1].code == 4, "Configuration parsing");
        std::istringstream walk("Walk key Left_Alt hold\nX key E hold\n");
        auto walkBindings = readBindings(walk, [](const std::string& key) { return key == "Left Alt" ? 226 : key == "E" ? 101 : 0; });
        require(walkBindings.size() == 2 && walkBindings[0].action == Action::Walk && walkBindings[0].code == 226 &&
            walkBindings[1].action == Action::X, "Walk configuration parsing");
        std::istringstream overlay("DebugOverlay key F3 toggle\nA key Space hold\n");
        auto overlayBindings = readBindings(overlay, [](const std::string& key) { return key == "F3" ? 1073741884 : key == "Space" ? 32 : 0; });
        require(overlayBindings.size() == 2 && overlayBindings[0].action == Action::DebugOverlay && overlayBindings[0].mode == Mode::Toggle &&
            overlayBindings[0].code == 1073741884, "DebugOverlay configuration parsing");
        InputState overlayState(overlayBindings);
        overlayState.event(Source::Key, 1073741884, true);
        overlayState.event(Source::Key, 1073741884, false);
        require(overlayState.active(Action::DebugOverlay) && !overlayState.active(Action::A), "DebugOverlay toggles on the first press");
        overlayState.event(Source::Key, 1073741884, true);
        require(!overlayState.active(Action::DebugOverlay), "DebugOverlay toggles off on the second press");
        for (const char* line : {"DebugOverlay key F3 latch\n", "DebugOverlay key F3\n", "Debug key F3 toggle\n", "DebugOverlay key Nope toggle\n"}) {
            std::istringstream bad(line);
            bool badRejected = false;
            try { readBindings(bad, [](const std::string& key) { return key == "F3" ? 1073741884 : 0; }); }
            catch (const std::runtime_error&) { badRejected = true; }
            require(badRejected, "Malformed DebugOverlay line was accepted");
        }
        std::istringstream invalid("A key Unknown hold\n");
        bool rejected = false;
        try { readBindings(invalid, [](const std::string&) { return 0; }); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Unknown key was accepted");
        std::cout << "Input core tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
