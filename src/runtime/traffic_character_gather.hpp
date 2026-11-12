#pragma once
// The grid gather 0x237f30 of the traffic contact step 0x29cc38 (docs/re/traffic-character-gather.md): the entries of the world grid 0x578dfc
// that pass the descriptor's filters and whose world box overlaps the query box, in the order of the cell walk (y outermost, then z, then x),
// at most `maxEntries`. No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/traffic_driver_calls.hpp"
#include "runtime/traffic_driver_steering.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// The cell layout of the grid: origin +0x20, inverse cell size +0x14, cell counts +0x2c, +0x2d and +0x2e. A count product of 0 is the
// grid's enable word +0x30 clear: the gather lists nothing.
struct TrafficGridLayout {
    AiVec3 origin{};
    AiVec3 inverseCell{};
    std::array<std::uint8_t, 3> counts{};
};

// The cell range stored at entry +8..+0xd: the low and the high cell of the entry's world box (x, y, z).
struct TrafficGridCellRange {
    std::array<std::uint8_t, 3> low{};
    std::array<std::uint8_t, 3> high{};
    bool operator==(const TrafficGridCellRange& other) const { return low == other.low && high == other.high; }
};

// One grid entry (0x20 bytes) with the owner words the gather reads.
struct TrafficGridEntry {
    std::uint32_t mask = 0;                 // entry +0: 4 for a character (no flag 0x100)
    std::uint8_t kind = 0;                  // entry +4: 0 box, 1 mesh
    std::uint32_t owner = 0;                // entry +0x10, identity
    std::uint32_t ownerWord = 0;            // owner +0xc
    std::uint16_t ownerFlags = 0;           // owner hword +0x10
    bool nodeSkipped = false;               // the owner's node word +0 bit 0x20
    std::uint32_t ownerNode = 0;            // owner +0x3c, identity
    std::uint32_t ownerNodeParent = 0;      // node +4
    AiMatrix nodeMatrix{};                  // 0x250810 of the owner's node
    std::optional<AiMatrix> extraMatrix;    // entry +0x14, null for entities
    AiVec3 shapeCentre{};                   // entry +0x18: centre at +0
    AiVec3 shapeHalf{};                     //   half extents at +0xc
    // Entry +8..+0xd. Empty: the entry is in no cell list yet (what a new entry holds is not read, TRAFFIC-S44).
    std::optional<TrafficGridCellRange> cells;
};

struct TrafficGrid {
    TrafficGridLayout layout;
    std::vector<TrafficGridEntry> entries;
    // The cell lists of the cell table at +0x6034 (traffic_character_feed.hpp keeps them as 0x1f5cd0 and 0x209cb8 do): the entry indices of
    // each cell in list order, indexed by trafficGridCellIndex. STAND-IN TRAFFIC-S44: when empty (a grid made by hand), the lists are the
    // entries in registration order in every cell of their current world box.
    std::vector<std::vector<std::uint32_t>> cellLists;
};

// The index in the cell table (gather 0x237f30 and cell update 0x209cb8): (z x count z + y) x count x + x, as the code computes it.
std::size_t trafficGridCellIndex(const TrafficGridLayout& layout, const std::array<std::uint8_t, 3>& cell);

// The box 0x22ec3c gives a shape under a matrix: the centre transformed, the half extents kept for an axis-aligned matrix (m00 and m11 above
// the literal 0x3f7fbe77), else the sum of the absolute rotated half extents.
struct TrafficWorldBox {
    AiVec3 centre{};
    AiVec3 half{};
};
TrafficWorldBox trafficShapeWorldBox(const AiMatrix& matrix, const AiVec3& centre, const AiVec3& half);

// The world box of an entry as 0x209cb8 takes it: the owner's node matrix times the extra matrix when there is one, then 0x22ec3c.
TrafficWorldBox trafficEntryWorldBox(const TrafficGridEntry& entry);

// 0x20ea60: the cell of a point, floor((p - origin) * inverseCell) clamped to [0, count - 1] per axis.
std::array<std::uint8_t, 3> trafficGridCell(const TrafficGridLayout& layout, const AiVec3& point);

// 0x237f30(grid, {centre, half}, list, maxEntries, descriptor). `excludedOwner` is descriptor word +0x20 (0 excludes none). Throws
// std::logic_error for the descriptor mode 2 (a sphere test through 0x2397a0, not ported).
std::vector<TrafficContactEntry> gatherTrafficGrid(const TrafficGrid& grid, const AiVec3& centre, const AiVec3& half,
                                                   const TrafficContactDescriptor& descriptor, std::uint32_t excludedOwner, unsigned maxEntries);

// The hook `gatherContacts` of TrafficDriveStep: the grid gather with the car itself as the excluded owner. `grid` must outlive the hook.
using TrafficContactGather = std::function<std::vector<TrafficContactEntry>(std::size_t car, const AiVec3& centre, const AiVec3& halfExtents,
                                                                            const TrafficContactDescriptor& descriptor, unsigned maxEntries)>;
TrafficContactGather makeTrafficContactGather(const TrafficGrid& grid, std::function<std::uint32_t(std::size_t car)> ownerOfCar);

}
