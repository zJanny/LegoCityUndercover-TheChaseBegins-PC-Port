#include "qol/menu_window_sdl.hpp"

#include "input.hpp"
#include "qol/front_panel.hpp"
#include "qol/menu_window.hpp"
#include "render/hud_overlay.hpp"
#include "render/window.hpp"
#include "frontend/layout.hpp"
#include "frontend/pages.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace qol {

namespace fe = chase::frontend;

namespace {

constexpr const char* gameText = "localisation/gametext_uk.loc";
constexpr const char* systemText = "localisation/systemtext_uk.loc";
constexpr const char* slotPageLayout = "sprites/blend/savegameflow/page_selectslot.bxaml";
constexpr const char* slotBar = "ListItem3";
constexpr const char* slideTemplate = "ListItem2";
constexpr const char* slotPageElements[] = {"canvas2", "canvas3", "canvas4", "canvas5", "NoticePanelBg", "canvas", "ListItem1",
    "ListItem2", "Text_Header", "Text_SubInfo", "Text_Confirm"};
constexpr const char* unusedBarFields[] = {"Text_SlotNum", "Text_Time", "Text_Percent", "Text_Location"};

int wheelSteps(const SDL_MouseWheelEvent& wheel) {
    return wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -wheel.y : wheel.y;
}

}

PageRect frontBottomScreen(int windowWidth, int windowHeight) {
    if (const auto panels = frontPanels(windowWidth, windowHeight, fe::kTopWidth, fe::kScreenHeight)) return panels->panel;
    const float scale = std::min(float(windowWidth) / fe::kTopWidth, float(windowHeight) / (2 * fe::kScreenHeight));
    const float originX = (float(windowWidth) - scale * fe::kTopWidth) / 2;
    const float originY = (float(windowHeight) - scale * 2 * fe::kScreenHeight) / 2;
    return {originX + scale * (fe::kTopWidth - fe::kBottomWidth) / 2, originY + scale * fe::kScreenHeight, scale * fe::kBottomWidth,
        scale * fe::kScreenHeight, scale};
}

std::optional<std::array<float, 2>> frontBottomPoint(int windowWidth, int windowHeight, float x, float y) {
    const PageRect screen = frontBottomScreen(windowWidth, windowHeight);
    if (screen.scale <= 0) return std::nullopt;
    const float px = (x - screen.x) / screen.scale, py = (y - screen.y) / screen.scale;
    if (px < 0 || py < 0 || px >= fe::kBottomWidth || py >= fe::kScreenHeight) return std::nullopt;
    return std::array<float, 2>{px, py};
}

const char* const frontButtonLabel = "PC Settings";
const ButtonPlace frontButtonPlace{121, 194, 0.5f};

fe::LayoutScene frontButtonScene() {
    fe::LayoutScene scene;
    for (const char* element : slotPageElements) scene.hidden.insert(element);
    for (const char* field : unusedBarFields) scene.hidden.insert(std::string(slotBar) + "/" + field);
    scene.positions[slotBar] = {0, 0};
    scene.scaleText = true;
    scene.referenceStoryboards[slotBar] = "ButtonHighlight";
    scene.referenceTimes[slotBar] = 0;
    fe::TextSlot label;
    label.text = frontButtonLabel;
    label.align = fe::Align::Left;
    scene.text[std::string(slotBar) + "/Text_NoData"] = label;
    return scene;
}

float frontButtonSlide(fe::Resources& resources, float seconds) {
    const chase::assets::UiLayout& layout = resources.layout(slotPageLayout);
    const auto sliding = fe::layoutChildBox(layout, {{"TransitionOn", seconds}}, slideTemplate);
    const auto resting = fe::layoutChildBox(layout, {}, slideTemplate);
    if (!sliding || !resting) throw std::runtime_error(std::string("settings menu: the slot page layout has no ") + slideTemplate);
    return (*sliding)[0] - (*resting)[0];
}

Box frontButtonBox(fe::Resources& resources) {
    fe::Canvas drawn(fe::kBottomWidth, fe::kScreenHeight);
    drawn.clearTransparent();
    fe::drawLayout(drawn, resources, slotPageLayout, frontButtonScene(), frontButtonPlace.x, frontButtonPlace.y, frontButtonPlace.scale);
    int left = drawn.width(), top = drawn.height(), right = -1, bottom = -1;
    for (int y = 0; y < drawn.height(); ++y)
        for (int x = 0; x < drawn.width(); ++x) {
            if (drawn.pixel(x, y)[3] == 0) continue;
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x);
            bottom = std::max(bottom, y);
        }
    if (right < 0) throw std::runtime_error(std::string("settings menu: the slot page bar ") + slotBar + " draws nothing");
    return {float(left), float(top), float(right - left + 1), float(bottom - top + 1)};
}

bool opensMenu(const fe::Flow& flow) {
    return flow.screen() == fe::Screen::SlotSelect && !flow.confirming();
}

struct FrontMenu::State {
    State(fe::Resources& resources, StoresPorts ports)
        : setup(menuPageSetup(resources, gameText, systemText)), stores(std::move(ports)), model(stores.settings()),
          window(model, setup, controlsListForm(setup)) {}

    MenuPageSetup setup;
    MenuStores stores;
    MenuModel model;
    MenuWindow window;
    std::optional<chase::render::HudOverlay> overlay;
    int overlayWidth = 0, overlayHeight = 0;
    std::uint64_t uploaded = 0;
};

FrontMenu::FrontMenu(fe::Resources& resources, StoresPorts ports) : resources_(resources), ports_(std::move(ports)) {}

FrontMenu::~FrontMenu() = default;

FrontMenu::State& FrontMenu::state() {
    if (!state_) state_ = std::make_unique<State>(resources_, std::move(ports_));
    return *state_;
}

const MenuModel* FrontMenu::model() const {
    return state_ ? &state_->model : nullptr;
}

bool FrontMenu::isOpen() const {
    return state_ && state_->window.isOpen();
}

bool FrontMenu::route(const SDL_Event& event, int width, int height) {
    State& s = *state_;
    Result result = Result::Ignored;
    switch (event.type) {
    case SDL_KEYDOWN:
        if (!event.key.repeat) result = s.window.keyDown(SDL_GetKeyName(event.key.keysym.sym));
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        if (const char* name = chase::padButtonName(event.cbutton.button)) result = s.window.padDown(name);
        break;
    case SDL_MOUSEBUTTONDOWN:
        result = s.window.mouseDown(event.button.button, width, height, float(event.button.x), float(event.button.y));
        break;
    case SDL_MOUSEMOTION:
        result = s.window.mouseMoved(width, height, float(event.motion.x), float(event.motion.y));
        break;
    case SDL_MOUSEWHEEL:
        result = s.window.wheel(wheelSteps(event.wheel));
        break;
    default:
        return false;
    }
    s.stores.commit(result);
    return true;
}

bool FrontMenu::consume(const SDL_Event& event, int width, int height, const fe::Flow& flow) {
    if (isOpen()) return route(event, width, height);
    if (!opensMenu(flow) || event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return false;
    const auto point = frontBottomPoint(width, height, float(event.button.x), float(event.button.y));
    if (!point) return false;
    if (!button_) button_ = frontButtonBox(resources_);
    const float slide = frontButtonSlide(resources_, flow.screenSeconds());
    if (!contains(HitArea{{button_->x + slide, button_->y, button_->width, button_->height}, false, std::nullopt, std::nullopt}, (*point)[0],
            (*point)[1]))
        return false;
    open();
    return true;
}

void FrontMenu::open() {
    State& s = state();
    s.stores.open(s.model);
    s.window.setGameOptions(s.stores.game());
    s.window.open();
}

bool FrontMenu::consumeOpen(const SDL_Event& event, int width, int height) {
    return isOpen() && route(event, width, height);
}

// STAND-IN QOLSET-S4: while the menu is open the front end draws and updates nothing, but sounds already playing in the mixer finish.
bool FrontMenu::present(SDL_Window* window) {
    if (!isOpen()) return false;
    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(window, &width, &height);
    return present(width, height);
}

bool FrontMenu::present(int width, int height) {
    if (!isOpen()) return false;
    State& s = *state_;
    if (width <= 0 || height <= 0) return true;
    if (!s.overlay || s.overlayWidth != width || s.overlayHeight != height) {
        s.overlay.reset();
        s.overlay.emplace(width, 1, height);
        s.overlayWidth = width;
        s.overlayHeight = height;
        s.uploaded = 0;
    }
    const RedrawnPage page = s.window.page(width, height, resources_);
    const bool upload = page.id != s.uploaded;
    s.uploaded = page.id;
    chase::render::setViewport({0, 0, width, height});
    chase::render::clearWindow({0, 0, 0, 1}, false);
    s.overlay->drawTop({width, height, page.pixels->rgba().data()}, {0, 0, width, height}, width, height, upload);
    return true;
}

void FrontMenu::drawButton(fe::Canvas& bottom, const fe::Flow& flow) {
    if (!opensMenu(flow)) return;
    const float slide = frontButtonSlide(resources_, flow.screenSeconds());
    fe::drawLayout(bottom, resources_, slotPageLayout, frontButtonScene(), frontButtonPlace.x + slide, frontButtonPlace.y, frontButtonPlace.scale);
}

}
