#include "runtime/traffic_character_gather.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace chase::runtime {
namespace {

constexpr std::int32_t kAxisAlignedBits = 0x3f7fbe77;
constexpr std::uint8_t kEntryVisited = 2;

std::int32_t bitsOf(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

AiVec3 transformPoint(const AiVec3& p, const AiMatrix& m) {
    AiVec3 out{};
    for (int axis = 0; axis < 3; ++axis) out[axis] = (p[1] * m[1][axis] + p[2] * m[2][axis]) + (m[3][axis] + p[0] * m[0][axis]);
    return out;
}

// 0x2588f4(out, a, b): out = a x b as 4x4 matrices whose last column is (0, 0, 0, 1).
AiMatrix multiply(const AiMatrix& a, const AiMatrix& b) {
    AiMatrix out{};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 3; ++column) {
            float sum = a[row][0] * b[0][column];
            sum += a[row][1] * b[1][column];
            sum += a[row][2] * b[2][column];
            if (row == 3) sum += b[3][column];
            out[row][column] = sum;
        }
    }
    return out;
}

std::uint32_t kindBit(std::uint8_t kind) {
    return kind < 32 ? (1u << kind) : 0u;
}

}

TrafficWorldBox trafficShapeWorldBox(const AiMatrix& matrix, const AiVec3& centre, const AiVec3& half) {
    TrafficWorldBox box;
    box.centre = transformPoint(centre, matrix);
    if (bitsOf(matrix[0][0]) > kAxisAlignedBits && bitsOf(matrix[1][1]) > kAxisAlignedBits) {
        box.half = half;
        return box;
    }
    for (int axis = 0; axis < 3; ++axis) {
        float sum = std::fabs(matrix[0][axis] * half[0]);
        sum = sum + std::fabs(matrix[1][axis] * half[1]);
        sum = sum + std::fabs(matrix[2][axis] * half[2]);
        box.half[axis] = sum;
    }
    return box;
}

TrafficWorldBox trafficEntryWorldBox(const TrafficGridEntry& entry) {
    AiMatrix matrix = entry.nodeMatrix;
    if (entry.extraMatrix) matrix = multiply(*entry.extraMatrix, entry.nodeMatrix);
    return trafficShapeWorldBox(matrix, entry.shapeCentre, entry.shapeHalf);
}

std::size_t trafficGridCellIndex(const TrafficGridLayout& layout, const std::array<std::uint8_t, 3>& cell) {
    return (std::size_t(cell[2]) * layout.counts[2] + cell[1]) * layout.counts[0] + cell[0];
}

std::array<std::uint8_t, 3> trafficGridCell(const TrafficGridLayout& layout, const AiVec3& point) {
    std::array<std::uint8_t, 3> cell{};
    for (int axis = 0; axis < 3; ++axis) {
        const float scaled = std::floor((point[axis] - layout.origin[axis]) * layout.inverseCell[axis]);
        const std::int32_t last = std::int32_t(layout.counts[axis]) - 1;
        std::int32_t value = 0;
        if (scaled >= float(last)) value = last;
        else if (scaled > 0.0f) value = static_cast<std::int32_t>(scaled);
        cell[axis] = static_cast<std::uint8_t>(value & 0xff);
    }
    return cell;
}

std::vector<TrafficContactEntry> gatherTrafficGrid(const TrafficGrid& grid, const AiVec3& centre, const AiVec3& half,
                                                   const TrafficContactDescriptor& descriptor, std::uint32_t excludedOwner, unsigned maxEntries) {
    std::vector<TrafficContactEntry> listed;
    const TrafficGridLayout& layout = grid.layout;
    if (unsigned(layout.counts[0]) * layout.counts[1] * layout.counts[2] == 0) return listed;
    if (descriptor.mode == 2) throw std::logic_error("traffic gather: the descriptor mode 2 (sphere test 0x2397a0) is not ported");

    const AiVec3 low = {centre[0] - half[0], centre[1] - half[1], centre[2] - half[2]};
    const AiVec3 high = {centre[0] + half[0], centre[1] + half[1], centre[2] + half[2]};
    const std::array<std::uint8_t, 3> first = trafficGridCell(layout, low);
    const std::array<std::uint8_t, 3> last = trafficGridCell(layout, high);

    struct Placed {
        std::array<std::uint8_t, 3> low;
        std::array<std::uint8_t, 3> high;
        AiMatrix matrix;
    };
    std::vector<Placed> placed;
    placed.reserve(grid.entries.size());
    for (const TrafficGridEntry& entry : grid.entries) {
        AiMatrix matrix = entry.nodeMatrix;
        if (entry.extraMatrix) matrix = multiply(*entry.extraMatrix, entry.nodeMatrix);
        const TrafficWorldBox box = trafficShapeWorldBox(matrix, entry.shapeCentre, entry.shapeHalf);
        const AiVec3 boxLow = {box.centre[0] - box.half[0], box.centre[1] - box.half[1], box.centre[2] - box.half[2]};
        const AiVec3 boxHigh = {box.centre[0] + box.half[0], box.centre[1] + box.half[1], box.centre[2] + box.half[2]};
        placed.push_back({trafficGridCell(layout, boxLow), trafficGridCell(layout, boxHigh), matrix});
    }

    std::vector<std::uint8_t> flags(grid.entries.size(), 0);
    std::vector<std::size_t> order;
    for (unsigned y = first[1]; y <= last[1]; ++y) {
        for (unsigned z = first[2]; z <= last[2]; ++z) {
            for (unsigned x = first[0]; x <= last[0]; ++x) {
                std::vector<std::uint32_t> inCell;
                if (grid.cellLists.empty()) {
                    for (std::size_t i = 0; i < grid.entries.size(); ++i) {
                        const Placed& at = placed[i];
                        if (x < at.low[0] || x > at.high[0] || y < at.low[1] || y > at.high[1] || z < at.low[2] || z > at.high[2]) continue;
                        inCell.push_back(std::uint32_t(i));
                    }
                } else {
                    const std::size_t cell = trafficGridCellIndex(layout, {std::uint8_t(x), std::uint8_t(y), std::uint8_t(z)});
                    if (cell < grid.cellLists.size()) inCell = grid.cellLists[cell];
                }
                for (const std::uint32_t i : inCell) {
                    const Placed& at = placed[i];
                    const TrafficGridEntry& entry = grid.entries[i];
                    if ((flags[i] & kEntryVisited) != 0) continue;
                    if ((descriptor.include & entry.mask) == 0) continue;
                    if ((entry.mask & descriptor.exclude) != 0) continue;
                    const std::uint32_t bit = kindBit(entry.kind);
                    if ((descriptor.kindsInclude & bit) == 0) continue;
                    if ((bit & descriptor.kindsExclude) != 0) continue;
                    if (entry.owner == excludedOwner) continue;
                    if (descriptor.ownerStateInclude != 0 && (descriptor.ownerStateInclude & entry.ownerFlags) == 0) continue;
                    if ((entry.ownerFlags & descriptor.ownerStateExclude) != 0) continue;
                    if (entry.nodeSkipped) continue;
                    if (descriptor.ownerFlagsInclude != 0 && (descriptor.ownerFlagsInclude & entry.ownerWord) == 0) continue;
                    if ((entry.ownerWord & descriptor.ownerFlagsExclude) != 0) continue;
                    if (descriptor.mode == 1) {
                        const TrafficWorldBox box = trafficShapeWorldBox(at.matrix, entry.shapeCentre, entry.shapeHalf);
                        bool overlaps = true;
                        for (int axis = 0; axis < 3 && overlaps; ++axis)
                            overlaps = !(std::fabs(box.centre[axis] - centre[axis]) > half[axis] + box.half[axis]);
                        if (!overlaps) continue;
                    }
                    if (order.size() >= maxEntries) break;
                    order.push_back(i);
                    flags[i] |= kEntryVisited;
                }
            }
        }
    }

    for (const std::size_t index : order) {
        const TrafficGridEntry& entry = grid.entries[index];
        TrafficContactEntry out;
        out.word = entry.mask;
        out.owner = entry.owner;
        out.ownerNode = entry.ownerNode;
        out.ownerNodeParent = entry.ownerNodeParent;
        listed.push_back(out);
    }
    return listed;
}

TrafficContactGather makeTrafficContactGather(const TrafficGrid& grid, std::function<std::uint32_t(std::size_t car)> ownerOfCar) {
    return [&grid, ownerOfCar = std::move(ownerOfCar)](std::size_t car, const AiVec3& centre, const AiVec3& halfExtents,
                                                       const TrafficContactDescriptor& descriptor, unsigned maxEntries) {
        return gatherTrafficGrid(grid, centre, halfExtents, descriptor, ownerOfCar(car), maxEntries);
    };
}

}
