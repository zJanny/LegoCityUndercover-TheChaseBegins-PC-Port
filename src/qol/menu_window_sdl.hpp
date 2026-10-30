#pragma once

#include "frontend/canvas.hpp"
#include "frontend/flow.hpp"
#include "frontend/layout.hpp"
#include "frontend/resources.hpp"
#include "qol/menu_hit.hpp"
#include "qol/menu_model.hpp"
#include "qol/menu_stores.hpp"

#include <SDL.h>
#include <array>
#include <memory>
#include <optional>

namespace qol {

PageRect frontBottomScreen(int windowWidth, int windowHeight);
std::optional<std::array<float, 2>> frontBottomPoint(int windowWidth, int windowHeight, float x, float y);
bool opensMenu(const chase::frontend::Flow& flow);

extern const char* const frontButtonLabel;

struct ButtonPlace {
    float x, y, scale;
};

extern const ButtonPlace frontButtonPlace;
chase::frontend::LayoutScene frontButtonScene();
Box frontButtonBox(chase::frontend::Resources& resources);
float frontButtonSlide(chase::frontend::Resources& resources, float seconds);

class FrontMenu {
public:
    explicit FrontMenu(chase::frontend::Resources& resources, StoresPorts ports = {});
    ~FrontMenu();
    FrontMenu(const FrontMenu&) = delete;
    FrontMenu& operator=(const FrontMenu&) = delete;

    bool isOpen() const;
    const MenuModel* model() const;
    bool consume(const SDL_Event& event, int windowWidth, int windowHeight, const chase::frontend::Flow& flow);
    bool present(SDL_Window* window);
    void drawButton(chase::frontend::Canvas& bottom, const chase::frontend::Flow& flow);

private:
    struct State;
    State& state();
    bool route(const SDL_Event& event, int windowWidth, int windowHeight);

    chase::frontend::Resources& resources_;
    StoresPorts ports_;
    std::optional<Box> button_;
    std::unique_ptr<State> state_;
};

inline bool frontMenuConsumes(FrontMenu& menu, const SDL_Event& event, SDL_Window* window, const chase::frontend::Flow& flow) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return menu.consume(event, width, height, flow);
}

inline bool frontMenuPresented(FrontMenu& menu, SDL_Window* window) {
    return menu.present(window);
}

inline void drawFrontMenuButton(FrontMenu& menu, chase::frontend::Canvas& bottom, const chase::frontend::Flow& flow) {
    menu.drawButton(bottom, flow);
}

}
