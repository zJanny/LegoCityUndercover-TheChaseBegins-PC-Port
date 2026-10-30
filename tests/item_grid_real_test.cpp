// The item grid read from the shipped layouts and drawn on the pause screen's Languages page (src/frontend/item_grid_view.cpp,
// src/frontend/pause_menu.cpp). Pinned values come from chase-layoutdump --tracks of sprites/ui_roundaboutmenu/main.bxaml and itemsmall.bxaml
// (20261030-qolpanels2-D, 2026-10-30): the portrait centre of itemSmall_i is itemSmallGroup (0, -10) + itemSmall_i + itemSmallPortrait (32, 32)
// + half its 64-pixel size, radius 32; Left_Arrow and Right_Arrow are 16 by 32 at Pageslots (59, 163) + (-57, -68) and + (239, -68), radius 16.
// The flag colour is compared with the decoded picture itself; nothing here is compared with the oracle. Needs the extracted RomFS; exits with
// 77 without it.
#include "frontend/pause_menu.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;
constexpr float kDt = 1.0f / 30;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void run(PauseMenu& menu, int updates, const PauseInput& input = {}) {
    for (int i = 0; i < updates; ++i) menu.update(kDt, input);
}

PauseInput press(bool PauseInput::*button) {
    PauseInput input;
    input.*button = true;
    return input;
}

bool near(const std::uint8_t* a, const std::uint8_t* b) {
    for (int c = 0; c < 3; ++c)
        if (std::abs(int(a[c]) - int(b[c])) > 2) return false;
    return true;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: item-grid-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "item-grid-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        const PauseMenuSetup setup = pauseMenuSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc");
        const chase::runtime::ItemGridTimings& t = setup.grid;
        require(t.blockbusterOn == 0.4f && t.transitionOn == 0.5f && t.leftMove == 0.4f && t.rightMove == 0.4f, "layout storyboard lengths");
        require(std::fabs(t.show - 0.24f) < 1e-6f && std::fabs(t.hide - 0.2f) < 1e-6f && t.pressed == 0.5f && t.outlined == 0 && t.defaultPose == 0,
                "itemSmall storyboard lengths");
        const float centres[chase::runtime::kItemGridTiles][2] = {{57, 53},   {57, 111}, {107, 81}, {107, 140}, {157, 52},
                                                                   {157, 110}, {208, 81}, {208, 139}, {258, 52}, {258, 110}};
        for (int i = 0; i < chase::runtime::kItemGridTiles; ++i) {
            const chase::runtime::ItemGridCircle& tile = setup.gridGeometry.tiles[std::size_t(i)];
            require(tile.x == centres[i][0] && tile.y == centres[i][1] && tile.radius == 32, "tile " + std::to_string(i) + " click area");
        }
        const chase::runtime::ItemGridGeometry& g = setup.gridGeometry;
        require(g.leftArrow.x == 10 && g.leftArrow.y == 111 && g.leftArrow.radius == 16 && g.rightArrow.x == 306 && g.rightArrow.y == 111 &&
                    g.rightArrow.radius == 16 && g.leftArrowOpacity == 1 && g.rightArrowOpacity == 1,
                "arrow click areas");

        PauseMenu menu(setup);
        menu.open();
        run(menu, 25);
        menu.update(kDt, press(&PauseInput::right));
        run(menu, 16);
        menu.update(kDt, press(&PauseInput::a));
        run(menu, 40);
        menu.update(kDt, press(&PauseInput::left));
        run(menu, 16);
        require(menu.page() == PausePage::Options && menu.round().selected() == 3, "the Language row of the Options page");
        menu.update(kDt, press(&PauseInput::a));
        run(menu, 40);
        require(menu.page() == PausePage::Languages && menu.grid().state() == chase::runtime::ItemGridState::Open, "the Languages page in the grid");
        require(menu.grid().tile(0).row == 0 && menu.grid().itemName() == " " && menu.grid().pageNumber().empty(), "no row selected, one page");

        Canvas bottom(kBottomWidth, kScreenHeight);
        menu.drawBottom(bottom, resources);
        const chase::assets::Image& uk = resources.image("sprites/ui_statusscreen/languages/ui_language_uk.bpng");
        const chase::assets::Image& german = resources.image("sprites/ui_statusscreen/languages/ui_language_german.bpng");
        const std::uint8_t* ukCentre = &uk.rgba[(std::size_t(uk.height / 2) * std::size_t(uk.width) + std::size_t(uk.width / 2)) * 4];
        const std::uint8_t* germanTop = &german.rgba[(std::size_t(german.height / 6) * std::size_t(german.width) + std::size_t(german.width / 2)) * 4];
        require(near(bottom.pixel(57, 53), ukCentre), "tile 0 (top left) shows the UK flag");
        const int germanTile = 3;
        require(near(bottom.pixel(int(centres[germanTile][0]), int(centres[germanTile][1] - 32 + 64 / 6)), germanTop),
                "tile 3 (the second column's lower tile) shows the German flag");
    } catch (const std::exception& error) {
        std::cerr << "item-grid-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "item-grid-real-test: ok\n";
    return 0;
}
