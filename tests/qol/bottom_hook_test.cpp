// Spec: docs/design/qol-screen-layout.md §2026-10-28T09:48:15 (Improved hides the bottom screen, Original shows it; Custom hides
// exactly when the minimap is on, and the minimap is master-only, so Custom shows it) and §2026-10-29T11:42:22 (QOLBOT-1 first; the
// user's answers of 2026-10-29: "Hide anyway" and "5:3 centred").
// Pinned rectangles computed by hand by 20261029-qolmenu-F on 2026-10-29: the 400x240 top picture scaled to fit the window and
// centred (GL rectangles count y from the window bottom, rounded to nearest); the bottom rectangle is empty. The Original
// rectangles of 1920x1080 are the unchanged game_hud::screensFor values (scale 2.25).
#include "game_hud.hpp"
#include "game_touch.hpp"
#include "qol/active_settings.hpp"
#include "qol/bottom_hook.hpp"

#include <SDL.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

bool same(const game_hud::Rect& a, const game_hud::Rect& b) { return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height; }

qol::Settings withMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    return settings;
}

SDL_Event mouse(Uint32 type, int x, int y) {
    SDL_Event event{};
    event.type = type;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = x;
    event.button.y = y;
    return event;
}

void predicate() {
    require(!qol::bottomScreenHidden(withMaster(qol::Master::Original)), "Original shows the bottom screen");
    require(qol::bottomScreenHidden(withMaster(qol::Master::Improved)), "Improved hides the bottom screen");
    qol::Settings custom;
    custom.setSwitch(qol::Feature::WidenedView, true);
    custom.setMaster(qol::Master::Custom);
    require(custom.master() == qol::Master::Custom, "Custom is set");
    require(!qol::bottomScreenHidden(custom), "Custom with the widened view still shows it");
    require(qol::minimapOn(withMaster(qol::Master::Improved)) && !qol::minimapOn(custom), "the minimap follows the master only");
}

void originalAndCustomKeepRectangles() {
    for (qol::Master master : {qol::Master::Original, qol::Master::Custom}) {
        qol::Settings settings;
        settings.setSwitch(qol::Feature::WidenedView, false);
        settings.setMaster(master);
        qol::setActiveSettings(settings);
        for (int width = 400; width <= 2600; width += 53)
            for (int height = 240; height <= 1500; height += 41) {
                const game_hud::Screens screens = game_hud::screensFor(width, height);
                require(screens.bottom.width > 0 && screens.bottom.height > 0, "bottom screen kept");
            }
        const game_hud::Screens screens = game_hud::screensFor(1920, 1080);
        require(same(screens.top, {510, 540, 900, 540}), "top rectangle of 1920x1080 unchanged");
        require(same(screens.bottom, {600, 0, 720, 540}), "bottom rectangle of 1920x1080 unchanged");
    }
}

void improvedHides() {
    qol::setActiveSettings(withMaster(qol::Master::Improved));
    const game_hud::Screens wide = game_hud::screensFor(1920, 1080);
    require(same(wide.top, {60, 0, 1800, 1080}), "1920x1080: top picture fits the window");
    require(wide.bottom.width == 0 && wide.bottom.height == 0, "1920x1080: bottom rectangle empty");
    const game_hud::Screens tall = game_hud::screensFor(1000, 1000);
    require(same(tall.top, {0, 200, 1000, 600}), "1000x1000: top picture centred between bars");
    const game_hud::Screens minimum = game_hud::screensFor(400, 240);
    require(same(minimum.top, {0, 0, 400, 240}), "400x240: top picture is the window");
    require(same(qol::hiddenTopArea(1920, 1080), {0, 0, 1920, 1080}), "view area is the whole window");
    require(!qol::hiddenScreensFor(withMaster(qol::Master::Improved), 0, 100, 400, 240), "empty window hides nothing");
}

void touch() {
    SDL_Window* window = SDL_CreateWindow("qol-bottom-hook", 0, 0, 1920, 1080, SDL_WINDOW_HIDDEN);
    if (!window) {
        std::cerr << "FAIL: no SDL window: " << SDL_GetError() << '\n';
        std::exit(1);
    }
    qol::setActiveSettings(withMaster(qol::Master::Original));
    {
        game_touch::Tracker tracker;
        require(tracker.consume(mouse(SDL_MOUSEBUTTONDOWN, 960, 800), window), "Original: click on the bottom screen is a touch");
    }
    qol::setActiveSettings(withMaster(qol::Master::Improved));
    {
        game_touch::Tracker tracker;
        require(!tracker.consume(mouse(SDL_MOUSEBUTTONDOWN, 960, 800), window), "Improved: click where the bottom screen was is no touch");
        require(!tracker.consume(mouse(SDL_MOUSEBUTTONDOWN, 960, 300), window), "Improved: click on the picture is no touch");
    }
    SDL_DestroyWindow(window);
}

}

int main() {
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "FAIL: SDL video: " << SDL_GetError() << '\n';
        return 1;
    }
    predicate();
    originalAndCustomKeepRectangles();
    improvedHides();
    touch();
    SDL_Quit();
    if (failures) return 1;
    std::cout << "qol-bottom-hook OK\n";
    return 0;
}
