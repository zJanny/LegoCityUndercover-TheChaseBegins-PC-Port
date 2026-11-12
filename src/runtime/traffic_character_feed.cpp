#include "runtime/traffic_character_feed.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr std::uint32_t kCountMarginBits = 0x3f7ff972;  // 0x271688
constexpr unsigned kHalvingLimit = 64;

struct Extent {
    AiVec3 low;
    AiVec3 high;
};

float floatOfBits(std::uint32_t bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

Extent unionOf(const std::vector<TrafficWorldBox>& boxes) {
    Extent extent;
    extent.low = {kTrafficGridBoundsLimit, kTrafficGridBoundsLimit, kTrafficGridBoundsLimit};
    extent.high = {-kTrafficGridBoundsLimit, -kTrafficGridBoundsLimit, -kTrafficGridBoundsLimit};
    for (const TrafficWorldBox& box : boxes) {
        for (int axis = 0; axis < 3; ++axis) {
            const float low = box.centre[axis] - box.half[axis];
            const float high = box.centre[axis] + box.half[axis];
            if (extent.low[axis] > low) extent.low[axis] = low;
            if (extent.high[axis] <= high) extent.high[axis] = high;
        }
    }
    return extent;
}

bool insideRange(const TrafficGridCellRange& range, unsigned x, unsigned y, unsigned z) {
    return x >= range.low[0] && x <= range.high[0] && y >= range.low[1] && y <= range.high[1] && z >= range.low[2] && z <= range.high[2];
}

std::vector<std::uint32_t>& listOf(TrafficGrid& grid, unsigned x, unsigned y, unsigned z) {
    const std::size_t cell = trafficGridCellIndex(grid.layout, {std::uint8_t(x), std::uint8_t(y), std::uint8_t(z)});
    if (cell >= grid.cellLists.size()) grid.cellLists.resize(cell + 1);
    return grid.cellLists[cell];
}

void removeFromList(std::vector<std::uint32_t>& list, std::uint32_t index) {
    const auto found = std::find(list.begin(), list.end(), index);
    if (found == list.end()) return;
    *found = list.back();
    list.pop_back();
}

}

TrafficGridLayout buildTrafficGridLayout(const std::vector<TrafficWorldBox>& shapeBoxes, const std::vector<TrafficWorldBox>& roomBounds) {
    if (shapeBoxes.empty() || roomBounds.empty()) throw std::invalid_argument("traffic grid: a box set is empty");
    const Extent shapes = unionOf(shapeBoxes);
    const Extent rooms = unionOf(roomBounds);
    Extent bounds;
    for (int axis = 0; axis < 3; ++axis) {
        bounds.low[axis] = shapes.low[axis] <= rooms.low[axis] ? rooms.low[axis] : shapes.low[axis];
        bounds.high[axis] = shapes.high[axis] >= rooms.high[axis] ? rooms.high[axis] : shapes.high[axis];
    }
    AiVec3 extent{};
    for (int axis = 0; axis < 3; ++axis) {
        extent[axis] = bounds.high[axis] - bounds.low[axis];
        if (!(extent[axis] > 0.0f)) throw std::invalid_argument("traffic grid: the bounds have no extent on an axis");
    }

    TrafficGridLayout layout;
    layout.origin = bounds.low;
    AiVec3 cell = extent;
    const float margin = floatOfBits(kCountMarginBits);
    for (unsigned step = 0;; ++step) {
        std::uint32_t product = 1;
        for (int axis = 0; axis < 3; ++axis) {
            layout.inverseCell[axis] = 1.0f / cell[axis];
            const float scaled = extent[axis] * layout.inverseCell[axis];
            const float counted = margin + scaled;
            layout.counts[axis] = static_cast<std::uint8_t>(static_cast<std::int32_t>(std::floor(counted)) & 0xff);
            product *= layout.counts[axis];
        }
        if (product >= kTrafficGridMinimumCells) break;
        if (step >= kHalvingLimit) throw std::invalid_argument("traffic grid: the cell count does not reach the minimum");
        if (cell[0] > cell[1]) {
            if (cell[0] > cell[2]) cell[0] *= 0.5f;
            else cell[2] *= 0.5f;
        } else {
            if (cell[1] > cell[2]) cell[1] *= 0.5f;
            else cell[2] *= 0.5f;
        }
    }
    return layout;
}

std::size_t trafficGridAdd(TrafficGrid& grid, const TrafficGridEntry& entry) {
    grid.entries.push_back(entry);
    const std::size_t index = grid.entries.size() - 1;
    trafficGridUpdateCells(grid, index);
    return index;
}

void trafficGridUpdateCells(TrafficGrid& grid, std::size_t index) {
    const TrafficGridLayout& layout = grid.layout;
    if (unsigned(layout.counts[0]) * layout.counts[1] * layout.counts[2] == 0) return;
    TrafficGridEntry& entry = grid.entries.at(index);
    const TrafficWorldBox box = trafficEntryWorldBox(entry);
    const AiVec3 boxLow = {box.centre[0] - box.half[0], box.centre[1] - box.half[1], box.centre[2] - box.half[2]};
    const AiVec3 boxHigh = {box.centre[0] + box.half[0], box.centre[1] + box.half[1], box.centre[2] + box.half[2]};
    TrafficGridCellRange next;
    next.low = trafficGridCell(layout, boxLow);
    next.high = trafficGridCell(layout, boxHigh);
    if (entry.cells && *entry.cells == next) return;

    if (entry.cells) {
        const TrafficGridCellRange previous = *entry.cells;
        for (unsigned y = previous.low[1]; y <= previous.high[1]; ++y)
            for (unsigned z = previous.low[2]; z <= previous.high[2]; ++z)
                for (unsigned x = previous.low[0]; x <= previous.high[0]; ++x)
                    if (!insideRange(next, x, y, z)) removeFromList(listOf(grid, x, y, z), std::uint32_t(index));
    }
    for (unsigned y = next.low[1]; y <= next.high[1]; ++y)
        for (unsigned z = next.low[2]; z <= next.high[2]; ++z)
            for (unsigned x = next.low[0]; x <= next.high[0]; ++x)
                if (!entry.cells || !insideRange(*entry.cells, x, y, z)) listOf(grid, x, y, z).push_back(std::uint32_t(index));
    entry.cells = next;
}

void trafficGridLeaveCells(TrafficGrid& grid, std::size_t index) {
    TrafficGridEntry& entry = grid.entries.at(index);
    if (!entry.cells) return;
    const TrafficGridCellRange previous = *entry.cells;
    for (unsigned y = previous.low[1]; y <= previous.high[1]; ++y)
        for (unsigned z = previous.low[2]; z <= previous.high[2]; ++z)
            for (unsigned x = previous.low[0]; x <= previous.high[0]; ++x) removeFromList(listOf(grid, x, y, z), std::uint32_t(index));
    entry.cells.reset();
}

TrafficCharacterFeed::TrafficCharacterFeed(const TrafficGridLayout& layout) {
    grid_.layout = layout;
    grid_.cellLists.resize(std::size_t(layout.counts[0]) * layout.counts[1] * layout.counts[2]);
}

void TrafficCharacterFeed::update(const std::vector<TrafficFedCharacter>& characters) {
    std::map<std::uint32_t, bool> present;
    for (const TrafficFedCharacter& character : characters) {
        if (character.id == 0) throw std::invalid_argument("traffic feed: a character without an id");
        if (!present.emplace(character.id, true).second) throw std::invalid_argument("traffic feed: the id " + std::to_string(character.id) + " is listed twice");
        const auto known = slotOf_.find(character.id);
        const bool isNew = known == slotOf_.end();
        std::size_t index = 0;
        if (isNew) {
            index = grid_.entries.size();
            grid_.entries.emplace_back();
            slotOf_[character.id] = index;
        } else {
            index = known->second;
        }
        TrafficGridEntry& entry = grid_.entries[index];
        entry.mask = kTrafficCharacterMask;
        entry.kind = 0;
        entry.owner = character.id;
        entry.ownerFlags = kTrafficCharacterOwnerFlags;
        entry.ownerNode = character.id + 0x100000u;
        entry.nodeMatrix = character.node;
        entry.shapeCentre = character.shapeCentre;
        entry.shapeHalf = character.shapeHalf;
        trafficGridUpdateCells(grid_, index);
    }
    for (const auto& [id, index] : slotOf_) {
        if (present.count(id) != 0) continue;
        trafficGridLeaveCells(grid_, index);
        grid_.entries[index].mask = 0;
    }
}

}
