// The map buttons on the shipped layout sprites/blend/mapbuttons/mapbuttons.bxaml (src/frontend/map_buttons_view.cpp, src/runtime/map_buttons.cpp)
// against the oracle: RPC request 20261028-uiport-A-1 (servant, 2026-10-28, recorded from a save state: mission 1-3, the helipad built, the
// player on the fast travel pad; three loads agree) read for every button record of the object 0x519e24 the node position +0xf0, +0xf4 and the
// size +0x54, +0x58 of its reference, before A (normal play, the scan flag +0x476 set, level 0x51640e = 2), 4 s after A (the fast travel map
// open, page +0x49b = 1) and after the touch on `area_exit` (page 0, the boxes of normal play again).
#include "frontend/map_buttons_view.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;
using chase::runtime::MapButton;
using chase::runtime::MapButtonBox;
using chase::runtime::kMapButtonCount;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

using Boxes = std::array<std::array<float, 2>, kMapButtonCount>;

// scan, exit, characters, audio, red bricks, freeplay, hint, area_0..5, area_exit
constexpr Boxes kNormal = {{{0, 176}, {-70, 176}, {0, 0}, {32, 148}, {326, 27}, {326, 149}, {326, 88},
                            {32, 176}, {83, 205}, {134, 176}, {185, 205}, {236, 176}, {287, 205}, {159, 65}}};
constexpr Boxes kMapOpen = {{{-70, 176}, {-70, 176}, {-70, 0}, {32, 148}, {326, 27}, {326, 149}, {326, 88},
                             {-32, 112}, {19, 141}, {70, 112}, {121, 141}, {172, 112}, {223, 141}, {95, 1}}};

void same(const std::array<MapButtonBox, kMapButtonCount>& boxes, const Boxes& expected, const std::string& when) {
    for (std::size_t i = 0; i < kMapButtonCount; ++i) {
        const float size = i >= std::size_t(MapButton::Area0) ? 128.0f : 64.0f;
        const MapButtonBox& box = boxes[i];
        require(std::abs(box.x - expected[i][0]) < 0.01f && std::abs(box.y - expected[i][1]) < 0.01f && box.width == size && box.height == size,
                when + ": " + chase::runtime::mapButtonInfo()[i].element + " at " + std::to_string(box.x) + " " + std::to_string(box.y));
    }
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: map-buttons-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "map-buttons-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        chase::runtime::MapButtonsSetup setup = mapButtonsSetup(resources);
        require(setup.storyboardLengths.at("charsOpen") > 0.69f && setup.storyboardLengths.at("charsOpen") < 0.71f, "charsOpen 0.7 s");
        setup.level = 2;
        setup.scan = true;
        chase::runtime::MapButtons buttons(setup);
        chase::runtime::MapButtonsInput idle;
        idle.dt = 1.0f / 30;
        idle.canChangeCharacter = idle.characterSwitchReady = true;
        for (int i = 0; i < 30; ++i) buttons.update(idle);
        same(mapButtonBoxes(resources, buttons), kNormal, "normal play");
        buttons.openFastTravel(0);
        for (int i = 0; i < 30; ++i) buttons.update(idle);
        same(mapButtonBoxes(resources, buttons), kMapOpen, "map open");
        buttons.setBoxes(mapButtonBoxes(resources, buttons));
        chase::runtime::MapButtonsInput touch = idle;
        touch.phase = chase::runtime::TouchPhase::Pressed;
        touch.x = 159;
        touch.y = 65;
        buttons.update(touch);
        require(buttons.page() == 0, "the touch at (159, 65) closes the map");
        for (int i = 0; i < 30; ++i) buttons.update(idle);
        same(mapButtonBoxes(resources, buttons), kNormal, "after the exit");
        const LayoutScene scene = mapButtonsScene(buttons);
        require(scene.attachedLayouts.at("area_exit") == chase::runtime::kMapButtonLayout, "button.bxaml instances");
        require(scene.images.at("buttonScan/icon") == "sprites/blend/mapbuttons/images/buttonicon_scan.bpng", "icon path");
        Canvas bottom(kBottomWidth, kScreenHeight);
        drawLayout(bottom, resources, chase::runtime::kMapButtonsLayout, scene, 0, 0);
    } catch (const std::exception& error) {
        std::cerr << "map-buttons-real-data: " << error.what() << '\n';
        return 1;
    }
    std::cout << "map-buttons-real-data: ok\n";
    return 0;
}
