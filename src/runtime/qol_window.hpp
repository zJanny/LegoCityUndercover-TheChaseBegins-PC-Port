#pragma once
#include "input.hpp"
#include "qol/menu_stores.hpp"

#include <SDL.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qol {
class WindowController;
}


namespace chase::runtime {

class QolWindow {
public:
    QolWindow(const std::string& path, std::optional<std::pair<int, int>> explicitSize);
    ~QolWindow();

    int width() const { return width_; }
    int height() const { return height_; }
    Uint32 flags() const { return flags_; }
    void attach(SDL_Window* window);
    void poll(const InputState& input);
    qol::StoresPorts menuStores(const std::string& controlsFile, InputState& input);

private:
    void apply(bool fullscreen, int width, int height);

    std::unique_ptr<qol::WindowController> controller_;
    std::string path_;
    std::vector<std::pair<int, int>> modes_;
    std::pair<int, int> desktop_{0, 0};
    SDL_Window* window_ = nullptr;
    int width_ = 0, height_ = 0;
    Uint32 flags_ = 0;
};

}
