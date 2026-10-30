// The item grid of the round menu layout (src/runtime/item_grid.cpp; docs/re/ui-screens-pause.md). Rules read from the shipped code with
// llvm-objdump on the analysis ELF by 20261030-qolpanels2-D on 2026-10-30 (open 0x21377c, close 0x1e87d0, update 0x1dad8c, page change
// 0x292018, texts 0x20ed0c). Storyboard lengths and click areas from the layouts (chase-layoutdump --tracks of
// sprites/ui_roundaboutmenu/main.bxaml and itemsmall.bxaml, same agent and date): Blockbuster_On 0.4, TransitionOn 0.5, Blockbuster_LeftMove
// and _RightMove 0.4; Show 0.24, Hide 0.2, pressed 0.5, outlined 0, default 0; portrait centres of itemSmall_0..9 with radius 32 (half the
// 64-pixel portrait), arrow centres with radius 16 (the arrow width); item-grid-real-data checks them against the layout.
#include "runtime/item_grid.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;
constexpr float kDt = 1.0f / 30;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

class Boards : public ItemGridLayout {
public:
    void startStoryboard(const std::string& name, float length) override {
        boards.erase(std::remove_if(boards.begin(), boards.end(), [&](const ItemTileBoard& board) { return board.name == name; }), boards.end());
        boards.push_back({name, 0, length, true});
        started.push_back(name);
    }
    bool storyboardFinished(const std::string& name) const override {
        for (const ItemTileBoard& board : boards)
            if (board.name == name) return board.time >= board.length;
        return true;
    }
    void advance(float dt) {
        for (ItemTileBoard& board : boards) board.time += dt;
    }
    std::vector<ItemTileBoard> boards;
    std::vector<std::string> started;
};

ItemGridTimings timings() {
    ItemGridTimings t;
    t.blockbusterOn = 0.4f;
    t.transitionOn = 0.5f;
    t.leftMove = t.rightMove = 0.4f;
    t.show = 0.24f;
    t.hide = 0.2f;
    t.pressed = 0.5f;
    return t;
}

ItemGridGeometry geometry() {
    ItemGridGeometry g;
    const float centres[kItemGridTiles][2] = {{57, 53}, {57, 111}, {107, 81}, {107, 140}, {157, 52}, {157, 110}, {208, 81}, {208, 139}, {258, 52}, {258, 110}};
    for (int i = 0; i < kItemGridTiles; ++i) g.tiles[std::size_t(i)] = {centres[i][0], centres[i][1], 32};
    g.leftArrow = {10, 111, 16};
    g.rightArrow = {306, 111, 16};
    return g;
}

int step(ItemGrid& grid, Boards& boards, const ItemGridInput& input = {}) {
    boards.advance(kDt);
    return grid.update(kDt, boards, input);
}

void run(ItemGrid& grid, Boards& boards, int updates) {
    for (int i = 0; i < updates; ++i) step(grid, boards);
}

ItemGridInput tap(float x, float y) {
    ItemGridInput input;
    input.touch = TouchPhase::Pressed;
    input.x = x;
    input.y = y;
    return input;
}

ItemGridInput press(bool ItemGridInput::*button) {
    ItemGridInput input;
    input.*button = true;
    return input;
}

std::vector<std::string> boardNames(const ItemGridTile& tile) {
    std::vector<std::string> names;
    for (const ItemTileBoard& board : tile.boards) names.push_back(board.name);
    return names;
}

ItemGrid opened(Boards& boards, int count) {
    ItemGrid grid(timings(), geometry());
    grid.open(boards, count, nullptr, -1);
    for (int row = 0; row < count; ++row) {
        grid.setText(row, "row " + std::to_string(row));
        grid.setPicture(row, "Sprites/Pictures/P" + std::to_string(row) + ".png");
    }
    return grid;
}

void openAndSelect() {
    Boards boards;
    ItemGrid grid = opened(boards, 9);
    require(grid.state() == ItemGridState::Opening && grid.busy() && grid.active(), "open starts in state 2");
    require(boards.started == std::vector<std::string>{kItemGridBlockbusterOn}, "open plays Blockbuster_On");
    require(grid.selected() == -1 && grid.itemName() == " " && grid.pageNumber().empty(), "no row selected, name ' ', one page shows no number");
    require(grid.tile(0).row == 0 && grid.tile(8).row == 8 && grid.tile(9).row == -1, "rows 0..8 on tiles 0..8");
    require(grid.tile(3).picture == "Sprites/Pictures/P3.png" && grid.tile(9).picture.empty(), "the row pictures on their tiles");
    require((boardNames(grid.tile(2)) == std::vector<std::string>{kItemTileShow, kItemTileDefault}) &&
                (boardNames(grid.oldTile(2)) == std::vector<std::string>{kItemTileDefault}),
            "Show and default on each tile, default on each old tile");
    step(grid, boards);
    require(grid.tile(9).look.background == 0.5f && grid.tile(0).look.background == 1.0f && grid.tile(0).look.locked == 0.0f,
            "an empty tile's background is at half opacity");
    run(grid, boards, 12);
    require(grid.state() == ItemGridState::Open && !grid.busy(), "open once Blockbuster_On has ended");

    require(step(grid, boards, press(&ItemGridInput::a)) == 0, "A without a selected row does nothing (UI-S15)");
    require(step(grid, boards, press(&ItemGridInput::b)) == -1, "B is -1");
    require(step(grid, boards, tap(100, 200)) == 0 && grid.selected() == -1, "a touch outside the tiles selects nothing");
    require(step(grid, boards, tap(107 + 20, 81 + 20)) == 0 && grid.selected() == 2, "a touch within 32 of tile 2's portrait centre selects row 2");
    require(grid.takeSounds() == std::vector<int>{kItemGridSoundMove} && grid.itemName() == "row 2", "move sound, row name");
    require((boardNames(grid.tile(2)) == std::vector<std::string>{kItemTileShow, kItemTileDefault, kItemTileOutlined, kItemTilePressed}) &&
                boardNames(grid.tile(5)).back() == kItemTileDefault,
            "outlined and pressed on the selected tile, default on the others");
    ItemGridInput held = tap(107, 81);
    held.touch = TouchPhase::Held;
    require(step(grid, boards, held) == 0, "a held touch does not act");
    require(step(grid, boards, tap(107, 81)) == 1 && grid.takeSounds().empty(), "a touch on the selected tile is A");
    require(step(grid, boards, press(&ItemGridInput::a)) == 1, "A with a row selected");
    ItemGridInput select;
    select.selectTapped = true;
    require(step(grid, boards, select) == 1, "the Select shortcut's tap is A");

    grid.close(boards, true, false);
    require(grid.state() == ItemGridState::Closing && boards.started.back() == kItemGridTransitionOn && !grid.takeRoundMenuClose(),
            "close plays TransitionOn and leaves the round menu");
    require(boardNames(grid.tile(8)).back() == kItemTileHide && boardNames(grid.tile(9)).back() != kItemTileHide, "Hide on the tiles below the row count");
    require(step(grid, boards, press(&ItemGridInput::a)) == 0, "no input while closing");
    run(grid, boards, 14);
    require(grid.state() == ItemGridState::Closed && !grid.busy() && !grid.active(), "closed once TransitionOn has ended");
    require(grid.tile(3).picture == "Sprites/Pictures/P3.png", "the pictures are kept without the release flag");
}

void pages() {
    Boards boards;
    ItemGrid grid = opened(boards, 12);
    require(grid.pages() == 2 && grid.pageNumber() == "1/2", "twelve rows make two pages");
    run(grid, boards, 13);
    require(step(grid, boards, press(&ItemGridInput::left)) == 0 && grid.first() == 0 && grid.takeSounds().empty(), "Left on the first page does nothing");
    step(grid, boards, tap(57, 53));
    grid.takeSounds();
    step(grid, boards, press(&ItemGridInput::right));
    require(grid.first() == 10 && grid.pageNumber() == "2/2" && grid.takeSounds() == std::vector<int>{kItemGridSoundPage}, "Right turns the page");
    require(boards.started.back() == kItemGridLeftMove, "with Blockbuster_LeftMove");
    require(grid.tile(0).row == 10 && grid.tile(1).row == 11 && grid.tile(2).row == -1 && grid.tile(2).picture.empty(), "rows 10 and 11 on the new page");
    require(grid.oldTile(0).row == 0 && grid.oldTile(0).picture == "Sprites/Pictures/P0.png", "the old tiles keep the previous page");
    require((boardNames(grid.oldTile(0)) == std::vector<std::string>{kItemTileDefault, kItemTileOutlined, kItemTilePressed}) &&
                grid.oldTile(0).boards[1].time == 0 && grid.oldTile(0).boards[2].time == 0.5f && !grid.tile(0).boards.front().running,
            "the old tile of the selected row is outlined at the end, earlier boards stopped");
    require(grid.selected() == 0 && grid.itemName() == "row 0", "the selection stays on its row");
    require(step(grid, boards, press(&ItemGridInput::right)) == 0 && grid.first() == 10, "Right on the last page does nothing");
    step(grid, boards, tap(10, 111 + 15));
    require(grid.first() == 0 && boards.started.back() == kItemGridRightMove, "a touch within 16 of the left arrow turns back");
    float left = grid.leftArrowOpacity();
    step(grid, boards, tap(306, 111));
    require(grid.first() == 10, "the right arrow turns forward");
    require(grid.leftArrowOpacity() < left || left == 0.0f, "the left arrow fades while on the first page");
    run(grid, boards, 4);
    require(grid.leftArrowOpacity() == 1.0f && grid.rightArrowOpacity() == 0.0f, "on the last page the left arrow is shown, the right one hidden");
}

void openOnSelection() {
    Boards boards;
    ItemGrid grid(timings(), geometry());
    grid.open(boards, 13, nullptr, 12);
    require(grid.first() == 10 && grid.selected() == 12 && grid.tile(2).row == 12, "the page of the selected row opens");
    run(grid, boards, 13);
    require(boardNames(grid.tile(2)).back() == kItemTilePressed, "the selected tile is outlined when the grid is open");
}

void rowFlags() {
    Boards boards;
    ItemGrid grid = opened(boards, 3);
    run(grid, boards, 13);
    step(grid, boards, tap(57, 53));
    grid.close(boards, false, true);
    require(grid.takeRoundMenuClose(), "close without the first flag closes the round menu too");
    run(grid, boards, 16);
    require(grid.tile(0).picture.empty() && grid.row(0).picture.empty(), "the release flag clears the pictures once closed");
    grid.open(boards, 3, nullptr, -1);
    require(grid.row(0).text.empty() && grid.row(0).enabled, "open clears the texts without a text list");
}

}

int main() {
    try {
        openAndSelect();
        pages();
        openOnSelection();
        rowFlags();
    } catch (const std::exception& error) {
        std::cerr << "item-grid: " << error.what() << '\n';
        return 1;
    }
    std::cout << "item-grid: ok\n";
    return 0;
}
