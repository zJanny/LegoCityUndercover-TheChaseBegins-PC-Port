#pragma once

#include "qol/display_tab.hpp"
#include "qol/settings.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qol {

struct WindowPlan {
    WindowMode mode;
    Resolution size;
    bool fullscreen() const { return mode == WindowMode::Fullscreen; }
    int width() const { return size.width; }
    int height() const { return size.height; }
};

ResolutionChoices usableModes(const std::vector<Resolution>& reported);

class WindowController;
std::unique_ptr<WindowController> makeWindowController(const std::string& path);

class WindowController {
public:
    WindowController(Settings settings, std::string path);

    WindowPlan start(const std::vector<Resolution>& reported, const Resolution& desktop,
        std::optional<Resolution> forcedWindow = std::nullopt);
    WindowPlan start(const std::vector<std::pair<int, int>>& reported, int desktopWidth, int desktopHeight,
        std::optional<std::pair<int, int>> forcedWindow);
    static std::pair<int, int> minimumSize();
    WindowPlan toggle();
    bool pressed(bool keyActive);
    const Settings& settings() const { return settings_; }

private:
    WindowPlan plan() const;

    Settings settings_;
    std::string path_;
    Resolution desktop_{0, 0};
    std::optional<Resolution> forced_;
    bool keyWasActive_ = false;
};

}
