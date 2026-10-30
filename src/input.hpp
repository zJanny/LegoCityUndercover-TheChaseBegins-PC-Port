#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <istream>
#include <string>
#include <vector>

namespace chase {
enum class Action { Up, Down, Left, Right, A, B, X, Y, L, R, Start, Select, Walk, DebugOverlay, Fullscreen, Count };
constexpr std::size_t actionCount = static_cast<std::size_t>(Action::Count);
const char* actionName(Action action);
enum class Source { Key, Mouse, Pad };
enum class Mode { Hold, Toggle };
struct Binding {
    Action action;
    Source source;
    int code;
    Mode mode;
    bool down = false;
    bool toggled = false;
};
class InputState {
public:
    explicit InputState(std::vector<Binding> bindings);
    void event(Source source, int code, bool down, bool repeat = false);
    void reset();
    void rebind(std::vector<Binding> bindings);
    bool active(Action action) const;
    int horizontal() const;
    int vertical() const;
private:
    std::vector<Binding> bindings_;
};
// Key resolver is supplied by the platform adapter; the input core needs no SDL.
std::vector<Binding> readBindings(std::istream& stream,
    const std::function<int(const std::string&)>& resolveKey);
// Pad button codes are the SDL game-controller button numbers; names are SDL's own, e.g. leftshoulder.
int padButtonCode(const std::string& name);
const char* padButtonName(int code);
struct Viewport { int x; int y; int width; int height; };
struct TouchPoint { int x; int y; };
Viewport fitViewport(int x, int y, int width, int height);
bool mapTouch(const Viewport& viewport, int x, int y, TouchPoint& point);
} // namespace chase
