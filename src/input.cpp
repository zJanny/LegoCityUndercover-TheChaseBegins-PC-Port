#include "input.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace chase {
namespace {
constexpr std::array<const char*, actionCount> names{
    "Up", "Down", "Left", "Right", "A", "B", "X", "Y", "L", "R", "Start", "Select", "Walk", "DebugOverlay"
};
}
const char* actionName(Action action) {
    auto index = static_cast<std::size_t>(action);
    return index < names.size() ? names[index] : "Unknown";
}
InputState::InputState(std::vector<Binding> bindings) : bindings_(std::move(bindings)) {}
void InputState::event(Source source, int code, bool down, bool repeat) {
    if (repeat) return;
    for (auto& binding : bindings_) {
        if (binding.source != source || binding.code != code) continue;
        if (down && !binding.down && binding.mode == Mode::Toggle)
            binding.toggled = !binding.toggled;
        binding.down = down;
    }
}
void InputState::reset() {
    for (auto& binding : bindings_) {
        binding.down = false;
        binding.toggled = false;
    }
}
bool InputState::active(Action action) const {
    for (const auto& binding : bindings_)
        if (binding.action == action &&
            (binding.mode == Mode::Toggle ? binding.toggled : binding.down)) return true;
    return false;
}
int InputState::horizontal() const {
    return int(active(Action::Right)) - int(active(Action::Left));
}
int InputState::vertical() const {
    return int(active(Action::Down)) - int(active(Action::Up));
}
std::vector<Binding> readBindings(std::istream& stream,
    const std::function<int(const std::string&)>& resolveKey) {
    std::vector<Binding> result;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;
        line = line.substr(0, line.find('#'));
        std::istringstream row(line);
        std::string action, source, code, mode, extra;
        if (!(row >> action)) continue;
        auto fail = [&]() { throw std::runtime_error(
            "Invalid controls configuration at line " + std::to_string(lineNumber)); };
        if (!(row >> source >> code >> mode) || (row >> extra)) fail();
        auto found = std::find_if(names.begin(), names.end(),
            [&](const char* name) { return action == name; });
        if (found == names.end() || (mode != "hold" && mode != "toggle")) fail();
        int value = 0;
        Source kind = Source::Key;
        if (source == "key") {
            // Underscores represent spaces in SDL names, e.g. Left_Shift.
            std::replace(code.begin(), code.end(), '_', ' ');
            value = resolveKey(code);
            if (value == 0) fail();
        } else if (source == "mouse") {
            kind = Source::Mouse;
            if (code == "Left") value = 1;
            else if (code == "Middle") value = 2;
            else if (code == "Right") value = 3;
            else if (code == "X1") value = 4;
            else if (code == "X2") value = 5;
            else fail();
        } else fail();
        result.push_back({static_cast<Action>(found - names.begin()), kind, value,
            mode == "hold" ? Mode::Hold : Mode::Toggle});
    }
    if (result.empty()) throw std::runtime_error("Controls configuration has no bindings");
    return result;
}
Viewport fitViewport(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) return {x, y, 0, 0};
    // Nintendo 3DS lower display: 320 x 240.
    int fittedWidth = width;
    int fittedHeight = int(static_cast<long long>(width) * 240 / 320);
    if (fittedHeight > height) {
        fittedHeight = height;
        fittedWidth = int(static_cast<long long>(height) * 320 / 240);
    }
    return {x + (width - fittedWidth) / 2, y + (height - fittedHeight) / 2,
        fittedWidth, fittedHeight};
}
bool mapTouch(const Viewport& viewport, int x, int y, TouchPoint& point) {
    if (viewport.width <= 0 || viewport.height <= 0 ||
        x < viewport.x || y < viewport.y ||
        static_cast<long long>(x) >= static_cast<long long>(viewport.x) + viewport.width ||
        static_cast<long long>(y) >= static_cast<long long>(viewport.y) + viewport.height)
        return false;
    point.x = std::clamp(int(static_cast<long long>(x - viewport.x) * 320 /
        viewport.width), 0, 319);
    point.y = std::clamp(int(static_cast<long long>(y - viewport.y) * 240 /
        viewport.height), 0, 239);
    return true;
}
} // namespace chase
