// The pause screen drawn from the shipped layouts (src/frontend/pause_menu.cpp). Provenance of the recorded values: the round menu button
// centres are the oracle's reads of the round menu object 0x0902ff40 (+0xf4 54 119, +0xfc 159 119, +0x104 264 119; request r38-B-1, state
// gameplay-start, 1.5 s after Start); the pixels are the oracle screenshot s4_left_p10 of the same request (the menu open on Resume, both
// screens 400x480; top (5,120) and (200,230) = 7 15 30, bottom (160,120) = 255 222 22, (60,120) = 161 168 106, (260,120) = 42 101 150,
// (160,60) = 6 47 121); the texts are gametext_uk / systemtext_uk entries of the keys in pause_menu.hpp. Needs the extracted RomFS; exits
// with 77 without it.
#include "frontend/pause_menu.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void pixel(const Canvas& canvas, int x, int y, int r, int g, int b, const std::string& what) {
    const std::uint8_t* p = canvas.pixel(x, y);
    const int tolerance = 2;
    if (std::abs(p[0] - r) > tolerance || std::abs(p[1] - g) > tolerance || std::abs(p[2] - b) > tolerance)
        throw std::runtime_error(what + ": " + std::to_string(p[0]) + " " + std::to_string(p[1]) + " " + std::to_string(p[2]));
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: pause-menu-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "pause-menu-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        const PauseMenuSetup setup = pauseMenuSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc");
        require(setup.centres[0] == std::array<float, 2>{54, 119} && setup.centres[1] == std::array<float, 2>{159, 119} &&
                    setup.centres[2] == std::array<float, 2>{264, 119},
                "button centres equal the oracle's reads");
        require(setup.round.transitionOnRound == 0.5f && setup.round.leftMove == 0.5f && setup.topTransitionOn == 0.4f, "storyboard lengths");
        require(setup.texts.items[0] == "Resume" && setup.texts.items[1] == "Options" && setup.texts.items[2] == "Credits" &&
                    setup.texts.items[3] == "Exit",
                "entry texts");
        require(setup.texts.title == "Paused" && setup.texts.subtitle == "Pause Menu" && setup.texts.select == "Select" && setup.texts.back == "Back",
                "title, subtitle and shortcut texts");
        PauseMenu menu(setup);
        menu.open();
        for (int i = 0; i < 50; ++i) menu.update(1.0f / 30, {});
        Canvas top(kTopWidth, kScreenHeight), bottom(kBottomWidth, kScreenHeight);
        menu.drawTop(top, resources);
        menu.drawBottom(bottom, resources);
        pixel(top, 5, 120, 7, 15, 30, "top background");
        pixel(top, 200, 230, 7, 15, 30, "top panel edge");
        pixel(bottom, 160, 120, 255, 222, 22, "Resume picture");
        pixel(bottom, 60, 120, 161, 168, 106, "Exit picture");
        pixel(bottom, 260, 120, 42, 101, 150, "Options picture");
        pixel(bottom, 160, 60, 6, 47, 121, "round menu background");
    } catch (const std::exception& error) {
        std::cerr << "pause-menu-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pause-menu-real-test: ok\n";
    return 0;
}
