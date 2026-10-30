#pragma once
// The item grid of the round menu layout (object 0x37600c, size 0x8f4, created by 0x202d54; docs/re/ui-screens-pause.md): ten itemSmall tiles
// with an old copy of each for the page move, the page arrows, the page number and the name of the selected row. Open 0x21377c, row text
// 0x213624, row picture 0x213558, close 0x1e87d0, update 0x1dad8c, page change 0x292018, texts 0x20ed0c, busy 0x1fbedc, active 0x20fd0c,
// selected 0x20fcf4, tile of a row 0x2ae038. No SDL dependency.
// STAND-IN UI-S15: the hit centre of a tile and of an arrow (node +0xf0) is taken as the element's pivot (its centre: every element of the
// grid has origin 0.5, 0.5); A while no row is selected (row -1, whose bytes 0x1dad8c reads in front of the row array) does nothing; the grey
// tint of a disabled row (0x20a330 with 100, 100, 100) is not drawn.
#include "runtime/map_buttons.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kItemGridTiles = 10;
constexpr int kItemGridCapacity = 13;  // rows allocated by the pause screen's create (0x202d54(…, 13, 0) at 0x26423c)
constexpr int kItemGridSoundMove = 0x12, kItemGridSoundPage = 0x18;
constexpr float kItemGridArrowFadeRate = 10.0f;  // literal at 0x1db798, opacity per second

enum class ItemGridState : std::uint8_t { Closed = 0, Open = 1, Opening = 2, Closing = 3 };

// The 0x88-byte rows at +0x8e4: +0 enabled, +1 locked, +2 new, +3 gift, +4 text, +0x84 picture.
struct ItemGridRow {
    bool enabled = true, locked = false, isNew = false, gift = false;
    std::string text, picture;
};

// Storyboard lengths of the round menu layout (Blockbuster_On +0x14, TransitionOn +0x18, Blockbuster_LeftMove +0x1c, Blockbuster_RightMove
// +0x20) and of the itemSmall layout (Show, Hide, pressed, outlined, default of each tile).
struct ItemGridTimings {
    float blockbusterOn = 0, transitionOn = 0, leftMove = 0, rightMove = 0;
    float show = 0, hide = 0, pressed = 0, outlined = 0, defaultPose = 0;
};

struct ItemGridCircle {
    float x = 0, y = 0, radius = 0;
};

// Click areas of 0x20a410 (touch phase 1, distance at most the radius): the portrait of each tile with half its width, each arrow with its
// width. The arrows' opacity before the first update is their layout value.
struct ItemGridGeometry {
    std::array<ItemGridCircle, kItemGridTiles> tiles{};
    ItemGridCircle leftArrow, rightArrow;
    float leftArrowOpacity = 1, rightArrowOpacity = 1;
};

struct ItemGridInput {
    bool a = false, b = false, left = false, right = false;  // press edges of the pad slots 4, 5, 2, 3
    bool selectTapped = false, backTapped = false;            // +2 and +1, set by the shortcut callbacks 0x2bf688 and 0x2b4d64
    TouchPhase touch = TouchPhase::None;
    float x = 0, y = 0;
};

// The storyboards the grid starts on its own layout (the round menu's layout, through the round menu object's storyboard player).
class ItemGridLayout {
public:
    virtual ~ItemGridLayout() = default;
    virtual void startStoryboard(const std::string& name, float length) = 0;
    virtual bool storyboardFinished(const std::string& name) const = 0;
};

constexpr const char* kItemGridBlockbusterOn = "Blockbuster_On";
constexpr const char* kItemGridTransitionOn = "TransitionOn";
constexpr const char* kItemGridLeftMove = "Blockbuster_LeftMove";
constexpr const char* kItemGridRightMove = "Blockbuster_RightMove";
constexpr const char* kItemTileShow = "Show";
constexpr const char* kItemTileHide = "Hide";
constexpr const char* kItemTilePressed = "pressed";
constexpr const char* kItemTileOutlined = "outlined";
constexpr const char* kItemTileDefault = "default";

struct ItemTileBoard {
    std::string name;
    float time = 0, length = 0;
    bool running = true;
};

// The opacities 0x1dad8c sets every update (0x24d0a8): unset until the grid first sets them.
struct ItemTileLook {
    bool enabled = true;
    std::optional<float> locked, isNew, gift, background;
};

struct ItemGridTile {
    int row = -1;
    std::string picture;  // the portrait's texture (0x234f3c), empty when cleared (0x234b90)
    std::vector<ItemTileBoard> boards;
    ItemTileLook look;
};

class ItemGrid {
public:
    ItemGrid(const ItemGridTimings& timings, const ItemGridGeometry& geometry, int capacity = kItemGridCapacity);

    void open(ItemGridLayout& layout, int count, const std::vector<std::string>* texts, int selected);
    void setText(int row, const std::string& text);
    void setPicture(int row, const std::string& picture);
    // 0x1e87d0(roundMenuStays, release): without `roundMenuStays` the round menu is closed too (0x20eec4(0, 0), see takeRoundMenuClose);
    // `release` frees the pictures once the grid has closed.
    void close(ItemGridLayout& layout, bool roundMenuStays, bool release);
    // 1 = A, -1 = B, 0 = nothing.
    int update(float dt, ItemGridLayout& layout, const ItemGridInput& input);

    ItemGridState state() const { return state_; }
    bool busy() const { return state_ == ItemGridState::Opening || state_ == ItemGridState::Closing; }
    bool active() const { return state_ != ItemGridState::Closed; }
    int selected() const { return selected_; }
    int first() const { return first_; }
    int count() const { return count_; }
    int page() const;
    int pages() const;
    const ItemGridRow& row(int index) const { return rows_.at(std::size_t(index)); }
    const ItemGridTile& tile(int index) const { return tiles_.at(std::size_t(index)); }
    const ItemGridTile& oldTile(int index) const { return oldTiles_.at(std::size_t(index)); }
    const std::string& itemName() const { return itemName_; }
    const std::string& pageNumber() const { return pageNumber_; }
    float leftArrowOpacity() const { return leftArrow_; }
    float rightArrowOpacity() const { return rightArrow_; }
    // +0x8ef: a touch cleared the new mark of a row.
    bool newCleared() const { return newCleared_; }
    bool takeRoundMenuClose();
    std::vector<int> takeSounds();

private:
    int tileOf(int row) const;
    void refreshTexts();
    void changePage(ItemGridLayout& layout, int direction);
    void start(ItemGridTile& tile, const std::string& name, bool atEnd);
    void stopAll(ItemGridTile& tile);
    float boardLength(const std::string& name) const;
    void showRow(ItemGridTile& tile, int row);
    void setLooks();
    static bool hit(const ItemGridInput& input, const ItemGridCircle& circle);

    ItemGridTimings timings_;
    ItemGridGeometry geometry_;
    std::vector<ItemGridRow> rows_;
    std::array<ItemGridTile, kItemGridTiles> tiles_{}, oldTiles_{};
    ItemGridState state_ = ItemGridState::Closed;
    int count_ = 0, first_ = 0, selected_ = -1;
    bool tapSelectedAccepts_ = true;  // +0x8f0, set by the create
    bool release_ = false, newCleared_ = false, roundMenuClose_ = false;
    float leftArrow_ = 1, rightArrow_ = 1;
    std::string itemName_, pageNumber_;
    std::vector<int> sounds_;
};

}
