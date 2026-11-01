#include "qol/menu_window_sdl.hpp"

#include "frontend/layout.hpp"
#include "frontend/pages.hpp"
#include "runtime/languages_page.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>
#include <utility>

namespace qol {

namespace fe = chase::frontend;

namespace {

constexpr const char* gameText = "localisation/gametext_uk.loc";
constexpr const char* slotPageLayout = "sprites/blend/savegameflow/page_selectslot.bxaml";
constexpr const char* slotBar = "ListItem3";
constexpr const char* slideTemplate = "ListItem2";
constexpr const char* slotPageElements[] = {"canvas2", "canvas3", "canvas4", "canvas5", "NoticePanelBg", "canvas", "ListItem1",
    "ListItem2", "Text_Header", "Text_SubInfo", "Text_Confirm"};
constexpr const char* unusedBarFields[] = {"Text_SlotNum", "Text_Time", "Text_Percent", "Text_Location"};

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

namespace {

std::vector<std::string> languageNames(fe::Resources& resources) {
    std::vector<std::string> names;
    for (const std::uint32_t key : chase::runtime::kLanguageNameKeys) names.push_back(resources.textForKey(gameText, key));
    return names;
}

}

FrontMenu::FrontMenu(fe::Resources& resources, StoresPorts ports) : resources_(resources), ports_(std::move(ports)) {}

FrontMenu::~FrontMenu() = default;

UiMenuWindow& FrontMenu::window() {
    if (!window_) window_ = std::make_unique<UiMenuWindow>(std::move(ports_), languageNames(resources_));
    return *window_;
}

const ui::UiMenu* FrontMenu::menu() const {
    return window_ ? &window_->menu() : nullptr;
}

bool FrontMenu::isOpen() const {
    return window_ && window_->isOpen();
}

bool FrontMenu::consume(const SDL_Event& event, int width, int height, const fe::Flow& flow) {
    if (isOpen()) return window_->consume(event, width, height);
    if (!opensMenu(flow) || event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return false;
    const auto point = frontBottomPoint(width, height, float(event.button.x), float(event.button.y));
    if (!point) return false;
    if (!button_) button_ = frontButtonBox(resources_);
    const float slide = frontButtonSlide(resources_, flow.screenSeconds());
    if (!contains(Box{button_->x + slide, button_->y, button_->width, button_->height}, (*point)[0], (*point)[1])) return false;
    open();
    return true;
}

void FrontMenu::open() {
    window().open();
}

bool FrontMenu::consumeOpen(const SDL_Event& event, int width, int height) {
    return isOpen() && window_->consume(event, width, height);
}

// STAND-IN QOLSET-S4: while the menu is open the front end draws and updates nothing, but sounds already playing in the mixer finish.
bool FrontMenu::present(SDL_Window* window) {
    if (!isOpen()) return false;
    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(window, &width, &height);
    return present(width, height);
}

bool FrontMenu::present(int width, int height) {
    return isOpen() && window_->present(width, height);
}

void FrontMenu::drawButton(fe::Canvas& bottom, const fe::Flow& flow) {
    if (!opensMenu(flow)) return;
    const float slide = frontButtonSlide(resources_, flow.screenSeconds());
    fe::drawLayout(bottom, resources_, slotPageLayout, frontButtonScene(), frontButtonPlace.x + slide, frontButtonPlace.y, frontButtonPlace.scale);
}

}
