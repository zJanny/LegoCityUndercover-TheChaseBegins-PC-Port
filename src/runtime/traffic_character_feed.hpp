#pragma once
// The grid of the traffic contact gather as the original builds and keeps it (docs/re/traffic-character-gather.md): the layout 0x271280 makes
// from the level's bounds, the cell lists that the add 0x1f5cd0 and the cell update 0x209cb8 maintain, and the feed of the characters of
// chase-game (the player and the pedestrians: mask 4, owner flag 0x200) that the contact step 0x29cc38 of a traffic car gathers. No SDL.
#include "runtime/traffic_character_gather.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kTrafficGridMinimumCells = 2048;  // 0x2716a8: the cell sizes are halved until the count product reaches this
constexpr float kTrafficGridBoundsLimit = 32767.0f;       // 0x271678 / 0x27167c: the start of the minimum and the maximum

// 0x271280(grid, level): the layout from two sets of boxes. `shapeBoxes` are the world boxes of the level's shapes (the scene's list +0x1308 and
// the items of the rooms), `roomBounds` the node bounds of the rooms (node +0xa0 centre, +0xac half). The grid's extent is the intersection of
// the bounds of both sets; the cell size starts as the extent and the largest cell size is halved (a tie goes to the later axis) until the
// cell count product reaches 2048; the counts are floor(extent x inverse cell + 0.999) as bytes. Throws std::invalid_argument for an empty set
// or an empty intersection (the original does not check).
TrafficGridLayout buildTrafficGridLayout(const std::vector<TrafficWorldBox>& shapeBoxes, const std::vector<TrafficWorldBox>& roomBounds);

// 0x1f5cd0(grid, entry, 0) for an entry that is not owner flag 0x400: the entry is appended to the entry table and 0x209cb8 puts it in its
// cells. Returns its index.
std::size_t trafficGridAdd(TrafficGrid& grid, const TrafficGridEntry& entry);

// 0x209cb8(grid, index): the entry's new cell range from its current world box. When it differs from the stored one, the entry leaves the cells
// that are not in the new range (the last entry of the list takes its place) and is appended to the cells that were not in the old range, in
// the order y, z, x. The table is the layout's count product, enlarged for the indices of trafficGridCellIndex that go past it.
void trafficGridUpdateCells(TrafficGrid& grid, std::size_t index);

// The cell bytes a new entry holds (0x30e0d8 and 0x2ba3f4 store 255, 255, 255 at +8..+0xa and 0, 0, 0 at +0xb..+0xd): an empty range, so the
// first cell update leaves no cell and joins every cell of the entry's box.
constexpr TrafficGridCellRange kTrafficGridNewEntryCells{{255, 255, 255}, {0, 0, 0}};

constexpr std::uint32_t kTrafficCharacterMask = 4;          // entry +0 of a character
constexpr std::uint16_t kTrafficCharacterOwnerFlags = 0x200;  // owner hword +0x10: the bit the contact descriptor includes
constexpr std::uint16_t kTrafficOwnerDisabled = 0x1;        // owner hword +0x10 bit 0: the descriptor excludes bits 0 and 1 (collision.md)
constexpr std::uint32_t kTrafficFeedPlayerId = 1;           // owner identities of the feed (compared only)
constexpr std::uint32_t kTrafficFeedPedestrianBase = 2;     // + the pedestrian slot
constexpr std::uint32_t kTrafficFeedCarBase = 0x10000;      // + the car, the excluded owner of the gather

// A character the feed puts in the grid. `shapeCentre` and `shapeHalf` are the entity's +0x60 and +0x6c (the entry's shape).
struct TrafficFedCharacter {
    std::uint32_t id = 0;     // identity, not 0, stable while the character stays in the grid
    AiMatrix node{};          // 0x250810 of the owner's node
    AiVec3 shapeCentre{};
    AiVec3 shapeHalf{};
    bool disabled = false;    // owner hword +0x10 bit 0 (STAND-IN TRAFFIC-S46: set for the characters the port holds disabled or hidden)
};

// The characters of a level in a grid. The original registers an entity once (0x2ba3f0 is idempotent) and removes none (docs/re/collision.md,
// "EnableObject / EnableCollision during play"): a new character is added, one that is listed gets the cell update and its owner flags, one
// that is not listed stays as it is. A disabled one keeps its entry and its cells; the descriptor's owner flag test leaves it out of a gather.
class TrafficCharacterFeed {
public:
    explicit TrafficCharacterFeed(const TrafficGridLayout& layout);
    void update(const std::vector<TrafficFedCharacter>& characters);
    const TrafficGrid& grid() const { return grid_; }
    TrafficGrid& grid() { return grid_; }

private:
    TrafficGrid grid_;
    std::map<std::uint32_t, std::size_t> slotOf_;
};

}
