#include "runtime/item_grid.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace chase::runtime {

ItemGrid::ItemGrid(const ItemGridTimings& timings, const ItemGridGeometry& geometry, int capacity)
    : timings_(timings), geometry_(geometry), rows_(std::size_t(std::max(capacity, 0))), leftArrow_(geometry.leftArrowOpacity),
      rightArrow_(geometry.rightArrowOpacity) {}

float ItemGrid::boardLength(const std::string& name) const {
    if (name == kItemTileShow) return timings_.show;
    if (name == kItemTileHide) return timings_.hide;
    if (name == kItemTilePressed) return timings_.pressed;
    if (name == kItemTileOutlined) return timings_.outlined;
    return timings_.defaultPose;
}

void ItemGrid::start(ItemGridTile& tile, const std::string& name, bool atEnd) {
    tile.boards.erase(std::remove_if(tile.boards.begin(), tile.boards.end(), [&](const ItemTileBoard& board) { return board.name == name; }),
                      tile.boards.end());
    const float length = boardLength(name);
    tile.boards.push_back({name, atEnd ? length : 0.0f, length, true});
}

void ItemGrid::stopAll(ItemGridTile& tile) {
    for (ItemTileBoard& board : tile.boards) board.running = false;
}

int ItemGrid::tileOf(int row) const {
    for (int i = 0; i < kItemGridTiles; ++i)
        if (row >= 0 && tiles_[std::size_t(i)].row == row) return i;
    return -1;
}

int ItemGrid::page() const { return (first_ & 0xff) / kItemGridTiles; }

int ItemGrid::pages() const { return ((count_ - 1) & 0xff) / kItemGridTiles + 1; }

void ItemGrid::refreshTexts() {
    itemName_ = selected_ == -1 ? std::string(" ") : rows_.at(std::size_t(selected_)).text;
    const int pageCount = pages() & 0xff;
    if (pageCount > 1) {
        char text[16];
        std::snprintf(text, sizeof text, "%d/%d", page() + 1, pageCount);
        pageNumber_ = text;
    } else {
        pageNumber_.clear();
    }
}

void ItemGrid::showRow(ItemGridTile& tile, int row) {
    tile.row = row;
    tile.picture = row >= 0 ? rows_[std::size_t(row)].picture : std::string();
}

void ItemGrid::open(ItemGridLayout& layout, int count, const std::vector<std::string>* texts, int selected) {
    count_ = std::clamp(count, 0, int(rows_.size()));
    layout.startStoryboard(kItemGridBlockbusterOn, timings_.blockbusterOn);
    state_ = ItemGridState::Opening;
    first_ = 0;
    selected_ = selected;
    while (selected_ >= first_ + kItemGridTiles) first_ += kItemGridTiles;
    for (int i = 0; i < kItemGridTiles; ++i) {
        ItemGridTile& tile = tiles_[std::size_t(i)];
        showRow(tile, -1);
        start(tile, kItemTileShow, false);
        start(tile, kItemTileDefault, false);
        ItemGridTile& old = oldTiles_[std::size_t(i)];
        showRow(old, -1);
        start(old, kItemTileDefault, false);
    }
    for (int r = 0; r < count_; ++r) {
        ItemGridRow& row = rows_[std::size_t(r)];
        row.text = texts && std::size_t(r) < texts->size() ? (*texts)[std::size_t(r)] : std::string();
        if (r >= first_ && r < first_ + kItemGridTiles) showRow(tiles_[std::size_t(r - first_)], r);
        row.enabled = true;
        row.locked = false;
        row.isNew = false;
    }
    refreshTexts();
}

void ItemGrid::setText(int row, const std::string& text) {
    rows_.at(std::size_t(row)).text = text;
    if (selected_ == row) refreshTexts();
}

void ItemGrid::setPicture(int row, const std::string& picture) {
    const int tile = tileOf(row);
    if (tile != -1) tiles_[std::size_t(tile)].picture.clear();
    rows_.at(std::size_t(row)).picture = picture;
    if (tile != -1) tiles_[std::size_t(tile)].picture = picture;
}

void ItemGrid::close(ItemGridLayout& layout, bool roundMenuStays, bool release) {
    if (state_ == ItemGridState::Closing || state_ == ItemGridState::Closed) return;
    layout.startStoryboard(kItemGridTransitionOn, timings_.transitionOn);
    if (!roundMenuStays) roundMenuClose_ = true;
    for (int i = 0; i < std::min(count_, kItemGridTiles); ++i) start(tiles_[std::size_t(i)], kItemTileHide, false);
    state_ = ItemGridState::Closing;
    release_ = release;
}

bool ItemGrid::hit(const ItemGridInput& input, const ItemGridCircle& circle) {
    return input.touch == TouchPhase::Pressed && std::hypot(input.x - circle.x, input.y - circle.y) <= circle.radius;
}

void ItemGrid::changePage(ItemGridLayout& layout, int direction) {
    sounds_.push_back(kItemGridSoundPage);
    first_ += direction * kItemGridTiles;
    refreshTexts();
    for (int i = 0; i < kItemGridTiles; ++i) {
        ItemGridTile& tile = tiles_[std::size_t(i)];
        ItemGridTile& old = oldTiles_[std::size_t(i)];
        stopAll(tile);
        stopAll(old);
        showRow(old, tile.row);
        showRow(tile, first_ + i < count_ ? first_ + i : -1);
        for (ItemGridTile* shown : {&tile, &old}) {
            if (shown->row != -1 && shown->row == selected_) {
                start(*shown, kItemTileOutlined, true);
                start(*shown, kItemTilePressed, true);
            } else {
                start(*shown, kItemTileDefault, true);
            }
        }
    }
    if (direction > 0) layout.startStoryboard(kItemGridLeftMove, timings_.leftMove);
    else layout.startStoryboard(kItemGridRightMove, timings_.rightMove);
}

void ItemGrid::setLooks() {
    for (int i = 0; i < kItemGridTiles; ++i) {
        ItemGridTile& tile = tiles_[std::size_t(i)];
        if (tile.row != -1) {
            const ItemGridRow& row = rows_[std::size_t(tile.row)];
            tile.look = {row.enabled, row.locked ? 1.0f : 0.0f, row.isNew ? 1.0f : 0.0f, row.gift ? 1.0f : 0.0f, 1.0f};
        } else {
            tile.look = {tile.look.enabled, 0.0f, 0.0f, 0.0f, 0.5f};
        }
        ItemGridTile& old = oldTiles_[std::size_t(i)];
        if (old.row != -1) {
            const ItemGridRow& row = rows_[std::size_t(old.row)];
            old.look = {row.enabled, row.locked ? 1.0f : 0.0f, row.isNew ? 1.0f : 0.0f, row.gift ? 1.0f : 0.0f, 1.0f};
        } else {
            old.look.locked = 0.0f;
            old.look.isNew = 0.0f;
            old.look.background = 0.5f;
        }
    }
}

int ItemGrid::update(float dt, ItemGridLayout& layout, const ItemGridInput& input) {
    for (std::array<ItemGridTile, kItemGridTiles>* set : {&tiles_, &oldTiles_})
        for (ItemGridTile& tile : *set)
            for (ItemTileBoard& board : tile.boards)
                if (board.running) board.time += dt;
    int selectedTile = tileOf(selected_);
    if (state_ == ItemGridState::Opening && layout.storyboardFinished(kItemGridBlockbusterOn)) {
        state_ = ItemGridState::Open;
        if (selectedTile != -1) {
            start(tiles_[std::size_t(selectedTile)], kItemTileOutlined, false);
            start(tiles_[std::size_t(selectedTile)], kItemTilePressed, false);
        }
    }
    if (state_ == ItemGridState::Closing && layout.storyboardFinished(kItemGridTransitionOn)) state_ = ItemGridState::Closed;
    if (state_ == ItemGridState::Closed && release_) {
        for (ItemGridTile& tile : tiles_) tile.picture.clear();
        for (int r = 0; r < count_; ++r) rows_[std::size_t(r)].picture.clear();
        release_ = false;
    }

    const int currentPage = page();
    const int pageCount = pages();
    bool a = input.selectTapped, b = input.backTapped;
    if (state_ == ItemGridState::Open) {
        const int before = selected_;
        int move = 0;
        if (input.a) {
            a = true;
        } else if (input.b) {
            b = true;
        } else if (input.right && count_ > first_ + kItemGridTiles) {
            move = 1;
        } else if (input.left && first_ - kItemGridTiles >= 0) {
            move = -1;
        } else {
            for (int i = 0; i < kItemGridTiles; ++i) {
                if (tiles_[std::size_t(i)].row == -1 || !hit(input, geometry_.tiles[std::size_t(i)])) continue;
                if (i == selectedTile) {
                    if (tapSelectedAccepts_) a = true;
                } else {
                    selected_ = first_ + i;
                    selectedTile = tileOf(selected_);
                    ItemGridRow& row = rows_[std::size_t(tiles_[std::size_t(i)].row)];
                    if (row.isNew) {
                        row.isNew = false;
                        newCleared_ = true;
                    }
                }
                break;
            }
            if (currentPage != 0 && hit(input, geometry_.leftArrow)) move = -1;
            if (currentPage < pageCount - 1 && hit(input, geometry_.rightArrow)) move = 1;
        }
        if (selected_ < 0 || !rows_.at(std::size_t(selected_)).enabled) a = false;
        if (move != 0) {
            changePage(layout, move);
        } else if (!a && selected_ != before) {
            sounds_.push_back(kItemGridSoundMove);
            for (int i = 0; i < kItemGridTiles; ++i)
                if (i != selectedTile) start(tiles_[std::size_t(i)], kItemTileDefault, false);
            if (selectedTile != -1) {
                start(tiles_[std::size_t(selectedTile)], kItemTileOutlined, false);
                start(tiles_[std::size_t(selectedTile)], kItemTilePressed, false);
            }
            refreshTexts();
        }
    }

    setLooks();
    const bool shown = state_ != ItemGridState::Closed && state_ != ItemGridState::Closing;
    const float step = dt * kItemGridArrowFadeRate;
    leftArrow_ = std::clamp(leftArrow_ + (shown && currentPage != 0 ? step : -step), 0.0f, 1.0f);
    rightArrow_ = std::clamp(rightArrow_ + (shown && pageCount - 1 > currentPage ? step : -step), 0.0f, 1.0f);
    return a ? 1 : b ? -1 : 0;
}

bool ItemGrid::takeRoundMenuClose() {
    const bool requested = roundMenuClose_;
    roundMenuClose_ = false;
    return requested;
}

std::vector<int> ItemGrid::takeSounds() {
    std::vector<int> out;
    out.swap(sounds_);
    return out;
}

}
