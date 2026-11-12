#include "input.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace chase {
namespace {
constexpr std::array<const char*, actionCount> names{
    "Up", "Down", "Left", "Right", "A", "B", "X", "Y", "L", "R", "Start", "Select", "Walk", "DebugOverlay", "Fullscreen"
};
constexpr std::array<const char*, 21> padNames{
    "a", "b", "x", "y", "back", "guide", "start", "leftstick", "rightstick", "leftshoulder", "rightshoulder",
    "dpup", "dpdown", "dpleft", "dpright", "misc1", "paddle1", "paddle2", "paddle3", "paddle4", "touchpad"
};
}
int padButtonCode(const std::string& name) {
    auto found = std::find_if(padNames.begin(), padNames.end(), [&](const char* entry) { return name == entry; });
    return found == padNames.end() ? -1 : int(found - padNames.begin());
}
const char* padButtonName(int code) {
    return code >= 0 && std::size_t(code) < padNames.size() ? padNames[std::size_t(code)] : nullptr;
}
const char* actionName(Action action) {
    auto index = static_cast<std::size_t>(action);
    return index < names.size() ? names[index] : "Unknown";
}
InputState::InputState(std::vector<Binding> bindings) : bindings_(std::move(bindings)) {}
void InputState::event(Source source, int code, bool down, bool repeat) {
    if (repeat) return;
    for (auto* group : {&bindings_, &fixed_})
        for (auto& binding : *group) {
            if (binding.source != source || binding.code != code) continue;
            if (down && !binding.down && binding.mode == Mode::Toggle)
                binding.toggled = !binding.toggled;
            binding.down = down;
        }
}
void InputState::reset() {
    for (auto* group : {&bindings_, &fixed_})
        for (auto& binding : *group) {
            binding.down = false;
            binding.toggled = false;
        }
}
void InputState::rebind(std::vector<Binding> bindings) { bindings_ = std::move(bindings); }
void InputState::rebindKeepingHeld(std::vector<Binding> bindings) {
    for (auto& next : bindings) {
        for (const auto& old : bindings_) {
            if (old.source != next.source || old.code != next.code) continue;
            next.down = next.down || old.down;
            if (next.mode == Mode::Toggle && old.mode == Mode::Toggle && old.action == next.action)
                next.toggled = next.toggled || old.toggled;
        }
    }
    bindings_ = std::move(bindings);
}
void InputState::fix(Action action, Source source, int code) {
    fixed_.push_back({action, source, code, Mode::Hold});
}
void InputState::retargetFixed(Source source, int code, Action action) {
    for (auto& binding : fixed_)
        if (binding.source == source && binding.code == code && !binding.down) binding.action = action;
}
bool InputState::active(Action action) const {
    for (const auto* group : {&bindings_, &fixed_})
        for (const auto& binding : *group)
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
        } else if (source == "pad") {
            kind = Source::Pad;
            value = padButtonCode(code);
            if (value < 0) fail();
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
