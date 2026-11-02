// The "PC Settings" bar of the slot page as a focusable entry, on the real front end (original mode: the master state is Original,
// the bar is the always shown exception of docs/rules/qol.md). Pinned to the brief of 20261102-qolmenu-A, 2026-11-02: Down from the
// last slot focuses the bar and the slot highlight goes, Up returns, A opens the menu and is not passed on to the front end, closing the
// menu leaves the focus on the bar, Y and X are not passed on while on the bar, a pointer pick of a slot drops the focus; on other
// pages nothing happens. Needs the extracted RomFS; exits with 77 without it.
#include "qol/menu_window_sdl.hpp"

#include "frontend/frontend.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {
namespace fe = chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

struct Drawn {
    fe::Canvas top{fe::kTopWidth, fe::kScreenHeight}, bottom{fe::kBottomWidth, fe::kScreenHeight};
};

fe::FlowConfig twoUsedSlots() {
    fe::FlowConfig config;
    config.slotCount = 2;
    config.slots = {{true, "01:00", "10%", 0}, {true, "02:00", "20%", 0}};
    return config;
}

fe::FrontInput press(std::uint32_t buttons) {
    return fe::FrontInput(buttons);
}

void settle(fe::FrontEnd& front) {
    for (int i = 0; i < 40; ++i) front.update(0.03f, {});
}

bool differs(const fe::Canvas& a, const fe::Canvas& b) {
    return a.rgba() != b.rgba();
}

void slotPage(fe::Resources& resources) {
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    front.flow().start(fe::Screen::SlotSelect);
    settle(front);
    qol::FrontMenu menu(resources);
    Drawn first, onBar, back;

    fe::FrontInput pad = press(fe::kButtonDown);
    menu.steer(front, 0.03f, pad);
    front.update(0.03f, pad);
    require(front.flow().selectedSlot() == 1 && !menu.entryFocused(), "Down from slot 1 moves to slot 2");
    settle(front);
    front.draw(first.top, first.bottom);
    menu.drawButton(first.bottom, front.flow());

    pad = press(fe::kButtonDown);
    menu.steer(front, 0.03f, pad);
    front.update(0.03f, pad);
    require(menu.entryFocused() && front.flow().selectedSlot() == 1, "Down from the last slot focuses the bar");
    for (int i = 0; i < 12; ++i) {
        pad = {};
        menu.steer(front, 0.03f, pad);
        front.update(0.03f, pad);
    }
    front.draw(onBar.top, onBar.bottom);
    menu.drawButton(onBar.bottom, front.flow());
    require(differs(first.bottom, onBar.bottom), "the picture changes: the slot highlight goes and the bar is highlighted");

    pad = press(fe::kButtonY | fe::kButtonX);
    menu.steer(front, 0.03f, pad);
    require(pad.pressed == 0, "Y and X are not passed on while the bar is focused");

    pad = press(fe::kButtonA);
    require(menu.steer(front, 0.03f, pad) && pad.pressed == 0 && menu.isOpen() && !front.flow().confirming(),
        "A opens the menu and does not confirm a slot");

    SDL_Event escape{};
    escape.type = SDL_KEYDOWN;
    escape.key.keysym.sym = SDLK_ESCAPE;
    require(menu.consumeOpen(escape, 960, 600) && !menu.isOpen() && menu.entryFocused(), "closing the menu leaves the focus on the bar");

    pad = press(fe::kButtonUp);
    menu.steer(front, 0.03f, pad);
    front.update(0.03f, pad);
    settle(front);
    require(!menu.entryFocused() && front.flow().selectedSlot() == 1, "Up returns to the last slot, which keeps its place");
    front.draw(back.top, back.bottom);
    menu.drawButton(back.bottom, front.flow());
    require(!differs(first.bottom, back.bottom), "back on the slot the page looks as before");

    pad = press(fe::kButtonDown);
    menu.steer(front, 0.03f, pad);
    pad = {};
    pad.selectRow = 0;
    menu.steer(front, 0.03f, pad);
    require(!menu.entryFocused(), "a pointer pick of a slot drops the focus");
}

void otherPage(fe::Resources& resources) {
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    front.flow().start(fe::Screen::Language);
    qol::FrontMenu menu(resources);
    fe::FrontInput pad = press(fe::kButtonDown);
    menu.steer(front, 0.03f, pad);
    require(!menu.entryFocused() && pad.pressed == fe::kButtonDown, "on other pages the input is untouched");
}

void confirming(fe::Resources& resources) {
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    front.flow().start(fe::Screen::SlotSelect);
    settle(front);
    qol::FrontMenu menu(resources);
    front.update(0.03f, press(fe::kButtonA));
    require(front.flow().confirming(), "A on a slot confirms it");
    fe::FrontInput pad = press(fe::kButtonDown);
    menu.steer(front, 0.03f, pad);
    require(!menu.entryFocused(), "a confirming slot gives the bar no focus");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-slot-page-entry-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-slot-page-entry-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        slotPage(resources);
        otherPage(resources);
        confirming(resources);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-slot-page-entry-real-data: all checks passed\n";
    return 0;
}
