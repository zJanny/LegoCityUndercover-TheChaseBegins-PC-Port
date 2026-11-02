#include "qol/ui_menu_sdl.hpp"

#include "input.hpp"
#include "qol/controls_file.hpp"
#include "render/ui_draw.hpp"
#include "render/ui_font.hpp"
#include "render/window.hpp"

#include <algorithm>
#include <cmath>

namespace qol {

namespace {

using ui::Key;

std::optional<Key> keyFor(SDL_Keycode sym) {
    switch (sym) {
    case SDLK_UP: return Key::Up;
    case SDLK_DOWN: return Key::Down;
    case SDLK_LEFT: return Key::Left;
    case SDLK_RIGHT: return Key::Right;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return Key::Enter;
    case SDLK_ESCAPE: return Key::Escape;
    case SDLK_q: return Key::Q;
    case SDLK_e: return Key::E;
    case SDLK_PAGEUP: return Key::PageUp;
    case SDLK_PAGEDOWN: return Key::PageDown;
    case SDLK_DELETE: return Key::Delete;
    default: return std::nullopt;
    }
}

std::optional<Key> padKeyFor(int button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return Key::PadUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return Key::PadDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return Key::PadLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return Key::PadRight;
    case SDL_CONTROLLER_BUTTON_A: return Key::PadA;
    case SDL_CONTROLLER_BUTTON_B: return Key::PadB;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return Key::PadLB;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return Key::PadRB;
    default: return std::nullopt;
    }
}

std::string underscored(std::string name) {
    std::replace(name.begin(), name.end(), ' ', '_');
    return name;
}

int scaled(float value, float scale) { return static_cast<int>(std::lround(value * scale)); }

bool padConnected() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
        if (SDL_IsGameController(i)) return true;
    return false;
}

double nowSeconds() { return static_cast<double>(SDL_GetTicks()) / 1000.0; }

float bundledWidth(std::string_view text, float pixels) { return chase::render::uiTextWidth(text, pixels); }
float bundledLineHeight(float pixels) { return chase::render::uiFontMetrics(pixels).lineHeight; }

}

bool menuConsumes(const SDL_Event& event) {
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEMOTION:
    case SDL_MOUSEWHEEL: return true;
    default: return false;
    }
}

std::optional<ui::Event> menuEventFor(const SDL_Event& event, bool prompt, float scaleX, float scaleY) {
    switch (event.type) {
    case SDL_KEYDOWN: {
        if (event.key.repeat) return std::nullopt;
        const SDL_Keycode sym = event.key.keysym.sym;
        if (prompt && sym != SDLK_ESCAPE && sym != SDLK_DELETE) return ui::promptInput(ui::InputSource::Key, underscored(SDL_GetKeyName(sym)));
        const std::optional<Key> key = keyFor(sym);
        return key ? std::optional<ui::Event>(ui::keyEvent(*key)) : std::nullopt;
    }
    case SDL_CONTROLLERBUTTONDOWN: {
        const char* name = chase::padButtonName(event.cbutton.button);
        if (prompt) return name ? std::optional<ui::Event>(ui::promptInput(ui::InputSource::Pad, name)) : std::nullopt;
        const std::optional<Key> key = padKeyFor(event.cbutton.button);
        return key ? std::optional<ui::Event>(ui::keyEvent(*key)) : std::nullopt;
    }
    case SDL_MOUSEBUTTONDOWN: {
        const int x = scaled(float(event.button.x), scaleX), y = scaled(float(event.button.y), scaleY);
        if (event.button.button == SDL_BUTTON_LEFT) return ui::mouseDown(x, y);
        if (event.button.button == SDL_BUTTON_RIGHT) return ui::rightClick(x, y);
        const std::string name = mouseButtonName(event.button.button);
        if (prompt && !name.empty()) return ui::promptInput(ui::InputSource::Mouse, name);
        return std::nullopt;
    }
    case SDL_MOUSEBUTTONUP:
        if (event.button.button != SDL_BUTTON_LEFT) return std::nullopt;
        return ui::mouseUp(scaled(float(event.button.x), scaleX), scaled(float(event.button.y), scaleY));
    case SDL_MOUSEMOTION: return ui::mouseMove(scaled(float(event.motion.x), scaleX), scaled(float(event.motion.y), scaleY));
    case SDL_MOUSEWHEEL: {
        const int steps = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
        return steps == 0 ? std::nullopt : std::optional<ui::Event>(ui::wheel(-steps));
    }
    default: return std::nullopt;
    }
}

UiMenuWindow::UiMenuWindow(StoresPorts ports, std::vector<std::string> languageNames)
    : stores_(std::move(ports)), languageNames_(std::move(languageNames)), menu_(bundledWidth, bundledLineHeight) {}

UiMenuWindow::~UiMenuWindow() = default;

void UiMenuWindow::open() {
    backdrop_.release();
    stores_.open();
    const ui::MenuSetup setup = ui::menuSetupFor(stores_, languageNames_);
    menu_.setWindow(drawableWidth_, drawableHeight_);
    menu_.setPadConnected(padConnected());
    menu_.open(setup);
}

void UiMenuWindow::commit(const MenuEffects& effects) {
    stores_.commit(effects);
    if (!effects.quitGame) return;
    SDL_Event quit{};
    quit.type = SDL_QUIT;
    SDL_PushEvent(&quit);
}

bool UiMenuWindow::consume(const SDL_Event& event, int windowWidth, int windowHeight) {
    if (!isOpen()) return false;
    if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED) menu_.setPadConnected(padConnected());
    const float scaleX = windowWidth > 0 && drawableWidth_ > 0 ? float(drawableWidth_) / float(windowWidth) : 1.0f;
    const float scaleY = windowHeight > 0 && drawableHeight_ > 0 ? float(drawableHeight_) / float(windowHeight) : 1.0f;
    if (const std::optional<ui::Event> converted = menuEventFor(event, menu_.promptOpen(), scaleX, scaleY))
        commit(menu_.handle(*converted, nowSeconds()));
    if (!isOpen()) backdrop_.release();
    return menuConsumes(event);
}

bool UiMenuWindow::present(int width, int height) {
    if (!isOpen()) return false;
    if (width <= 0 || height <= 0) return true;
    drawableWidth_ = width;
    drawableHeight_ = height;
    const BackdropStep backdrop = backdrop_.step();
    if (backdrop == BackdropStep::Defer) return false;
    menu_.setWindow(width, height);
    commit(menu_.tick(nowSeconds()));
    chase::render::setScissorTest(false);
    chase::render::setDepthTest(false);
    chase::render::setViewport({0, 0, width, height});
    chase::render::clearWindow({0, 0, 0, 1}, false);
    if (backdrop == BackdropStep::Picture) backdrop_.draw(width, height);
    chase::render::UiDrawList list;
    menu_.paint(list);
    if (!draw_) draw_ = std::make_unique<chase::render::UiDraw>();
    draw_->draw(chase::render::buildUiBatch(list, width, height), width, height);
    return true;
}

}
