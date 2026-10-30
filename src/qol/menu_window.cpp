#include "qol/menu_window.hpp"

#include "qol/controls_file.hpp"

#include <algorithm>
#include <utility>

namespace qol {

namespace {

struct NamedKey {
    const char* name;
    Key key;
};

constexpr NamedKey keyNames[] = {
    {"Up", Key::Up}, {"Down", Key::Down}, {"Left", Key::Left}, {"Right", Key::Right}, {"Return", Key::Enter},
    {"Escape", Key::Escape}, {"PageUp", Key::PageUp}, {"PageDown", Key::PageDown},
};

constexpr NamedKey padNames[] = {
    {"dpup", Key::PadUp}, {"dpdown", Key::PadDown}, {"dpleft", Key::PadLeft}, {"dpright", Key::PadRight}, {"a", Key::PadA},
    {"b", Key::PadB}, {"leftshoulder", Key::PadL}, {"rightshoulder", Key::PadR},
};

template <std::size_t N>
std::optional<Key> lookUp(const NamedKey (&table)[N], const std::string& name) {
    for (const NamedKey& entry : table)
        if (name == entry.name) return entry.key;
    return std::nullopt;
}

std::string underscored(std::string name) {
    std::replace(name.begin(), name.end(), ' ', '_');
    return name;
}

}

std::optional<Key> menuKeyForName(const std::string& keyName) {
    return lookUp(keyNames, keyName);
}

std::optional<Key> menuKeyForPad(const std::string& padName) {
    return lookUp(padNames, padName);
}

MenuWindow::MenuWindow(MenuModel& model, MenuPageSetup setup, ListForm list)
    : model_(model), setup_(std::move(setup)), list_(std::move(list)) {}

void MenuWindow::open() {
    model_.open();
    pointer_.reset();
}

bool MenuWindow::assigning() const {
    return model_.controlsWaiting().has_value();
}

Result MenuWindow::handle(const std::optional<Event>& event) {
    if (!event) return Result::Ignored;
    return model_.handle(*event);
}

std::optional<std::array<float, 2>> MenuWindow::pageAt(int windowWidth, int windowHeight, float x, float y) const {
    if (windowWidth <= 0 || windowHeight <= 0) return std::nullopt;
    return pagePoint(menuPageRect(windowWidth, windowHeight), x, y);
}

Result MenuWindow::keyDown(const std::string& keyName) {
    if (!isOpen()) return Result::Ignored;
    if (assigning()) return model_.assignInput("key", underscored(keyName));
    const std::optional<Key> key = menuKeyForName(keyName);
    return key ? handle(eventForKey(*key)) : Result::Ignored;
}

Result MenuWindow::padDown(const std::string& padName) {
    if (!isOpen()) return Result::Ignored;
    if (assigning()) return model_.assignInput("pad", padName);
    const std::optional<Key> key = menuKeyForPad(padName);
    return key ? handle(eventForKey(*key)) : Result::Ignored;
}

Result MenuWindow::mouseDown(int button, int windowWidth, int windowHeight, float x, float y) {
    if (!isOpen()) return Result::Ignored;
    if (assigning()) {
        const std::string name = mouseButtonName(button);
        return name.empty() ? Result::Ignored : model_.assignInput("mouse", name);
    }
    if (button != leftMouseButton) return Result::Ignored;
    const auto point = pageAt(windowWidth, windowHeight, x, y);
    if (!point) return Result::Ignored;
    return handle(pointer_.click(pageHitAreas(model_, setup_, list_), (*point)[0], (*point)[1]));
}

Result MenuWindow::mouseMoved(int windowWidth, int windowHeight, float x, float y) {
    if (!isOpen()) return Result::Ignored;
    const auto point = pageAt(windowWidth, windowHeight, x, y);
    if (!point) {
        pointer_.reset();
        return Result::Ignored;
    }
    return handle(pointer_.move(pageHitAreas(model_, setup_, list_), (*point)[0], (*point)[1]));
}

Result MenuWindow::wheel(int steps) {
    if (!isOpen()) return Result::Ignored;
    return handle(eventForWheel(steps));
}

void MenuWindow::draw(chase::frontend::Canvas& window, chase::frontend::Resources& resources) const {
    drawMenuPage(window, resources, model_, setup_, list_, game_);
}

RedrawnPage MenuWindow::page(int windowWidth, int windowHeight, chase::frontend::Resources& resources) {
    return redraw_.draw(windowWidth, windowHeight, resources, model_, setup_, list_, game_);
}

}
