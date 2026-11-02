// The "PC Settings" button of the save slot page and the menu it opens, pinned to qol-settings.md §2026-10-29T18:06:35 (SlotSelect page
// only; a bottom-screen button clicked with the mouse only; the front end stops while the menu is open; the menu covers the whole window;
// the unused third list bar ListItem3 of the slot page at half size, centred under slot 2, covering nothing, sliding in with slot 2's
// timing; the click area follows the bar, so it is the box of the bar's drawn pixels). Original behaviour first: on the other front-end
// screens nothing of it runs or draws. Values (20261029-qolhook-B, 2026-10-29, chase-layoutdump --tracks of
// sprites/blend/savegameflow/page_selectslot.bxaml and panel_slotlistitem.bxaml): ListItem2's TransitionOn x track is -280 until 0.3 s
// and 0 from 0.6 s. The place {121, 194, scale 0.5} puts the drawn bar (x 104-217, y 188-212, the pixels with alpha above 0, also
// measured by the code reviewer on the slot page after 1.2 s) under slot 2 (drawn to y 187, centre x 160.5) and right of the Delete
// shortcut (drawn to x 84). The window 960x600 is the size of the test window in the build's config/qol.cfg. Written by
// 20261029-qolhook-B, 2026-10-29. Needs the extracted RomFS; exits with 77 without it.
#include "qol/menu_window_sdl.hpp"

#include "frontend/frontend.hpp"
#include "frontend/pages.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace fe = chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

constexpr int width = 960, height = 600;
constexpr float screenX = 280, screenY = 300, scale = 1.25f;

SDL_Event mouse(Uint32 type, int button, float x, float y) {
    SDL_Event event{};
    event.type = type;
    event.button.button = Uint8(button);
    event.button.x = int(std::lround(screenX + scale * x));
    event.button.y = int(std::lround(screenY + scale * y));
    return event;
}

SDL_Event key(Uint32 type, SDL_Keycode code) {
    SDL_Event event{};
    event.type = type;
    event.key.keysym.sym = code;
    return event;
}

bool sameCanvas(const fe::Canvas& a, const fe::Canvas& b) {
    return a.rgba() == b.rgba();
}

qol::StoresPorts displayPorts() {
    qol::StoresPorts ports;
    ports.settingsPath = (std::filesystem::temp_directory_path() / "chase-qol-front-menu-real-never-written.cfg").string();
    ports.modes = {{960, 600}, {1920, 1080}};
    ports.desktop = {1920, 1080};
    return ports;
}

int changedPixels(const fe::Canvas& a, const fe::Canvas& b, int x0, int y0, int x1, int y1) {
    int changed = 0;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const std::uint8_t *p = a.pixel(x, y), *q = b.pixel(x, y);
            changed += p[0] != q[0] || p[1] != q[1] || p[2] != q[2];
        }
    return changed;
}

void geometry() {
    const qol::PageRect bottom = qol::frontBottomScreen(width, height);
    require(bottom.x == screenX && bottom.y == screenY && bottom.width == 400 && bottom.height == 300 && bottom.scale == scale,
        "the bottom screen sits where the front presenter blits it");
    const auto inside = qol::frontBottomPoint(width, height, screenX + scale * 20, screenY + scale * 220);
    require(inside && (*inside)[0] == 20 && (*inside)[1] == 220, "window pixel to bottom-screen pixel");
    require(!qol::frontBottomPoint(width, height, 100, 400) && !qol::frontBottomPoint(width, height, 400, 100),
        "the bars and the top screen are outside the bottom screen");
}

void otherScreens(fe::Resources& resources) {
    fe::Flow flow(nullptr, {});
    flow.start(fe::Screen::Language);
    require(!qol::opensMenu(flow), "the language page has no button");
    qol::FrontMenu menu(resources);
    const qol::Box box = qol::frontButtonBox(resources);
    require(!menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, box.x + box.width / 2, box.y + box.height / 2), width, height, flow) &&
            !menu.isOpen() && !menu.menu(),
        "a click on the button place of another page goes to the front end and builds nothing");
    fe::Canvas bottom(fe::kBottomWidth, fe::kScreenHeight), before(fe::kBottomWidth, fe::kScreenHeight);
    bottom.clear(10, 20, 30);
    before.clear(10, 20, 30);
    menu.drawButton(bottom, flow);
    require(sameCanvas(bottom, before), "no button is drawn on another page");
}

void slotPage(fe::Resources& resources) {
    require(qol::frontButtonSlide(resources, 0) == -280 && qol::frontButtonSlide(resources, 0.3f) == -280 &&
            qol::frontButtonSlide(resources, 0.6f) == 0 && qol::frontButtonSlide(resources, 2) == 0,
        "the bar slides in with slot 2's TransitionOn track (x -280 until 0.3 s, 0 from 0.6 s)");
    fe::Flow flow(nullptr, {});
    flow.start(fe::Screen::SlotSelect);
    require(qol::opensMenu(flow), "the slot page has the button");
    qol::FrontMenu early(resources);
    const qol::Box resting = qol::frontButtonBox(resources);
    require(!early.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, resting.x + resting.width / 2, resting.y + resting.height / 2), width,
                height, flow),
        "before the bar has slid in, its resting place is no button");
    for (int i = 0; i < 30; ++i) flow.update(0.03f, {});

    fe::Canvas bottom(fe::kBottomWidth, fe::kScreenHeight), before(fe::kBottomWidth, fe::kScreenHeight);
    bottom.clear(0, 0, 0);
    before.clear(0, 0, 0);
    qol::FrontMenu menu(resources, displayPorts());
    menu.drawButton(bottom, flow);
    const qol::Box box = qol::frontButtonBox(resources);
    const qol::ButtonPlace place = qol::frontButtonPlace;
    require(box.x == 104 && box.y == 188 && box.width == 114 && box.height == 25,
        "the click area is the bar as drawn at its place: x 104-217, y 188-212");
    const int left = int(box.x), top = int(box.y), right = int(std::ceil(box.x + box.width)), bottomEdge = int(std::ceil(box.y + box.height));
    require(changedPixels(bottom, before, left, top, right, bottomEdge) > 0, "the third list bar is drawn at its place");
    fe::Canvas expected(fe::kBottomWidth, fe::kScreenHeight);
    expected.clear(0, 0, 0);
    fe::drawLayout(expected, resources, "sprites/blend/savegameflow/page_selectslot.bxaml", qol::frontButtonScene(),
        place.x + qol::frontButtonSlide(resources, flow.screenSeconds()), place.y, place.scale);
    require(sameCanvas(bottom, expected), "the button is the slot page layout's third list bar");
    const fe::LayoutScene scene = qol::frontButtonScene();
    require(scene.text.at("ListItem3/Text_NoData").text == "PC Settings", "labelled PC Settings");
    require(!scene.hidden.count("ListItem3") && scene.hidden.count("ListItem1") && scene.hidden.count("ListItem2"),
        "only the third bar is drawn, the slots stay the page's own");

    fe::FlowConfig used;
    used.slots = {fe::SlotSummary{true, "1:23", "4%", 0}, fe::SlotSummary{}};
    fe::FrontEnd front(resources, nullptr, used);
    front.flow().start(fe::Screen::SlotSelect);
    for (int i = 0; i < 50; ++i) front.update(0.03f, {});
    fe::Canvas upper(fe::kTopWidth, fe::kScreenHeight), page(fe::kBottomWidth, fe::kScreenHeight), plain(fe::kBottomWidth, fe::kScreenHeight);
    front.draw(upper, plain);
    front.draw(upper, page);
    menu.drawButton(page, front.flow());
    require(qol::opensMenu(front.flow()) && changedPixels(page, plain, 0, 0, fe::kBottomWidth, fe::kScreenHeight) > 0,
        "the button is drawn on a page with a used slot selected");
    fe::Canvas shortcuts(fe::kBottomWidth, fe::kScreenHeight);
    shortcuts.clearTransparent();
    fe::drawSelectShortcut(shortcuts, resources, "Select", "Delete");
    int shortcutPixels = 0, covered = 0;
    for (int y = 0; y < fe::kScreenHeight; ++y)
        for (int x = 0; x < fe::kBottomWidth; ++x) {
            if (shortcuts.pixel(x, y)[3] == 0) continue;
            ++shortcutPixels;
            covered += changedPixels(page, plain, x, y, x + 1, y + 1);
        }
    require(shortcutPixels > 0 && covered == 0, "every pixel of the Delete and Select shortcuts is drawn unchanged");
    require(changedPixels(page, plain, 0, 0, fe::kBottomWidth, 188) == 0, "the Select Slot bar and both slots are drawn unchanged");

    const float cx = box.x + box.width / 2, cy = box.y + box.height / 2;
    require(!menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, 100, 224), width, height, flow) && !menu.isOpen() && !menu.menu(),
        "a click on the Delete shortcut goes to the front end and builds no menu");
    require(!menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, box.x + box.width + 4, cy), width, height, flow) && !menu.isOpen(),
        "a click beside the bar goes to the front end");
    require(!menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, cx, cy), width, height, flow) && !menu.isOpen(),
        "the right button does not open the menu");
    require(!menu.consume(key(SDL_KEYDOWN, SDLK_RETURN), width, height, flow) && !menu.isOpen(), "no key opens the menu");
    require(!menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, 160, 186), width, height, flow) && !menu.isOpen(),
        "a click on slot 2's last drawn rows goes to the front end");
    const float edges[][2] = {{106, 200}, {215, 200}, {160, 190}, {160, 211}};
    for (const auto& edge : edges) {
        qol::FrontMenu probe(resources);
        require(probe.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, edge[0], edge[1]), width, height, flow) && probe.isOpen(),
            "a click near the drawn edge of the bar at " + std::to_string(int(edge[0])) + "," + std::to_string(int(edge[1])) +
                " opens the menu");
    }
    require(menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, cx, cy), width, height, flow) && menu.isOpen(),
        "a left click on the bar opens the menu and does not reach the front end");
    require(menu.menu() && menu.menu()->tabTitles() == std::vector<std::string>{"Display", "Quality of life"} &&
                menu.menu()->screen().selectedTab == 0 && menu.menu()->screen().focusRow == 0,
        "the menu opens on the first row of its first tab (here Display: no Game or Controls store is given)");
    require(!menu.consume(mouse(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, cx, cy), width, height, flow), "button releases pass on");

    require(menu.consume(key(SDL_KEYDOWN, SDLK_DOWN), width, height, flow) && menu.menu()->screen().focusRow == 3,
        "keys go to the menu (Down skips the greyed-out Resolution and View distance rows in full screen and Original)");
    require(!menu.consume(key(SDL_KEYUP, SDLK_DOWN), width, height, flow), "key releases pass on");
    SDL_Event repeat = key(SDL_KEYDOWN, SDLK_UP);
    repeat.key.repeat = 1;
    require(menu.consume(repeat, width, height, flow) && menu.menu()->screen().focusRow == 3, "key repeats are swallowed and do nothing");
    SDL_Event wheel{};
    wheel.type = SDL_MOUSEWHEEL;
    wheel.wheel.y = 1;
    require(menu.consume(wheel, width, height, flow) && menu.menu()->screen().focusRow == 3, "the wheel goes to the menu");
    SDL_Event pad{};
    pad.type = SDL_CONTROLLERBUTTONDOWN;
    pad.cbutton.button = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    require(menu.consume(pad, width, height, flow) && menu.menu()->screen().selectedTab == 1, "pad buttons go to the menu");
    SDL_Event quit{};
    quit.type = SDL_QUIT;
    require(!menu.consume(quit, width, height, flow), "quitting passes on");
    fe::Flow language(nullptr, {});
    language.start(fe::Screen::Language);
    require(menu.consume(key(SDL_KEYDOWN, SDLK_ESCAPE), width, height, language) && !menu.isOpen(),
        "Escape closes the menu and does not reach the front end");
    require(!menu.consume(key(SDL_KEYDOWN, SDLK_RIGHT), width, height, flow), "after closing, keys go to the front end again");
    require(menu.consume(mouse(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, cx, cy), width, height, flow) && menu.isOpen() &&
            menu.menu()->screen().selectedTab == 1,
        "reopening keeps the tab");

    fe::FrontInput a;
    a.pressed = fe::kButtonA;
    flow.update(0.03f, a);
    require(flow.confirming() && !qol::opensMenu(flow), "once a slot is chosen the button is gone");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-front-menu-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-front-menu-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        geometry();
        otherScreens(resources);
        slotPage(resources);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-front-menu-real-data: all checks passed\n";
    return 0;
}
