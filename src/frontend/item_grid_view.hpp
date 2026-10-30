#pragma once
// The picture of the item grid (runtime/item_grid.hpp) in the round menu layout sprites/UI_RoundaboutMenu/Main: the tiles itemSmall_0..9 (tile i
// is the reference itemSmall_i: the pause screen creates the grid with 0x202d54(…, 13, 0), whose element names are "itemSmall_%u" and
// "itemSmall_%u_Old" of the tile index), the arrows Left_Arrow and Right_Arrow, PageNumber and TEXT_ItemName. No SDL dependency.
#include "frontend/layout.hpp"
#include "frontend/round_menu.hpp"
#include "runtime/item_grid.hpp"

namespace chase::frontend {

constexpr const char* kItemSmallLayout = "sprites/ui_roundaboutmenu/itemsmall.bxaml";

runtime::ItemGridTimings itemGridTimings(const assets::UiLayout& main, const assets::UiLayout& itemSmall);
runtime::ItemGridGeometry itemGridGeometry(Resources& resources);
std::string itemGridTileElement(int tile, bool old);
// Adds the grid's tiles, pictures, opacities and texts to the round menu's scene (the grid's own storyboards are in the round menu's list).
void addItemGridScene(LayoutScene& scene, const runtime::ItemGrid& grid);

class RoundMenuGridLayout : public runtime::ItemGridLayout {
public:
    explicit RoundMenuGridLayout(RoundMenu& round) : round_(round) {}
    void startStoryboard(const std::string& name, float length) override { round_.startStoryboard(name, length); }
    bool storyboardFinished(const std::string& name) const override { return round_.storyboardFinished(name); }

private:
    RoundMenu& round_;
};

}
