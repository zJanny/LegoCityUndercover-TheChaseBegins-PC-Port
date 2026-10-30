#include "runtime/qol_window.hpp"

#include "qol/window_apply.hpp"
#include "runtime/qol_controls_bindings.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

namespace chase::runtime {

using qol::StoresPorts, qol::Settings, qol::ControlsPort, qol::Resolution, qol::usableModes;

namespace {

std::vector<std::pair<int, int>> displayModes() {
    std::vector<std::pair<int, int>> modes;
    const int count = SDL_GetNumDisplayModes(0);
    for (int i = 0; i < count; ++i) {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) == 0) modes.emplace_back(mode.w, mode.h);
    }
    return modes;
}

}

QolWindow::QolWindow(const std::string& path, std::optional<std::pair<int, int>> explicitSize) {
    SDL_DisplayMode desktop;
    if (SDL_GetDesktopDisplayMode(0, &desktop) != 0) throw std::runtime_error(SDL_GetError());
    controller_ = qol::makeWindowController(path);
    path_ = path;
    modes_ = displayModes();
    desktop_ = {desktop.w, desktop.h};
    const auto plan = controller_->start(modes_, desktop.w, desktop.h, explicitSize);
    width_ = plan.width();
    height_ = plan.height();
    if (plan.fullscreen()) flags_ = SDL_WINDOW_FULLSCREEN_DESKTOP;
}

QolWindow::~QolWindow() = default;

void QolWindow::attach(SDL_Window* window) {
    window_ = window;
    const auto minimum = controller_->minimumSize();
    SDL_SetWindowMinimumSize(window_, minimum.first, minimum.second);
}

void QolWindow::apply(bool fullscreen, int width, int height) {
    if (fullscreen) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
        return;
    }
    SDL_SetWindowFullscreen(window_, 0);
    SDL_SetWindowSize(window_, width, height);
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void QolWindow::poll(const InputState& input) {
    if (!window_ || !controller_->pressed(input.active(Action::Fullscreen))) return;
    const auto plan = controller_->toggle();
    apply(plan.fullscreen(), plan.width(), plan.height());
}

StoresPorts QolWindow::menuStores(const std::string& controlsFile, InputState& input) {
    StoresPorts ports;
    ports.settingsPath = path_;
    std::vector<Resolution> reported;
    for (const auto& mode : modes_) reported.push_back({mode.first, mode.second});
    ports.modes = usableModes(reported);
    ports.desktop = {desktop_.first, desktop_.second};
    ports.applyWindow = [this](const Settings& settings) {
        const auto plan = controller_->adopt(settings);
        apply(plan.fullscreen(), plan.width(), plan.height());
    };
    ports.makeControls = [controlsFile]() -> std::unique_ptr<ControlsPort> {
        return std::make_unique<QolControlsBindings>(controlsFile);
    };
    ports.controlsChanged = [controlsFile, &input]() {
        std::ifstream file(controlsFile);
        if (!file) throw std::runtime_error("cannot open controls: " + controlsFile);
        input.rebind(readBindings(file, sdlKeyResolver));
    };
    return ports;
}

}
