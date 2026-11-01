#pragma once

#include "qol/menu_stores.hpp"
#include "qol/ui_backdrop.hpp"
#include "qol/ui_menu.hpp"

#include <SDL.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace chase::render {
class UiDraw;
}

namespace qol {

std::optional<ui::Event> menuEventFor(const SDL_Event& event, bool prompt, float scaleX, float scaleY);
bool menuConsumes(const SDL_Event& event);

class UiMenuWindow {
public:
    UiMenuWindow(StoresPorts ports, std::vector<std::string> languageNames);
    ~UiMenuWindow();
    UiMenuWindow(const UiMenuWindow&) = delete;
    UiMenuWindow& operator=(const UiMenuWindow&) = delete;

    bool isOpen() const { return menu_.isOpen(); }
    const ui::UiMenu& menu() const { return menu_; }
    void open();
    bool consume(const SDL_Event& event, int windowWidth, int windowHeight);
    bool present(int width, int height);

private:
    void commit(const MenuEffects& effects);

    MenuStores stores_;
    std::vector<std::string> languageNames_;
    ui::UiMenu menu_;
    std::unique_ptr<chase::render::UiDraw> draw_;
    Backdrop backdrop_;
    int drawableWidth_ = 0;
    int drawableHeight_ = 0;
};

}
