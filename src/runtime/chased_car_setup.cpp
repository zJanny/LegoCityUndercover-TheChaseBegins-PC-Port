#include "runtime/chased_car_setup.hpp"

#include "runtime/detective.hpp"

namespace chase::runtime {

ChasedPathTable chasedPathTable(const assets::LevelPath& path) {
    ChasedPathTable table;
    table.lengths = detectivePathLengths(path, kChasedPathLengthSteps);
    if (!table.lengths.empty()) table.total = table.lengths.back();
    return table;
}

void chasedNodeTest(bool meshNode, ChasedNodeBytes& node) {
    if (!meshNode) return;
    node.alpha = kNodeFadeOpaqueByte;
    node.flags = static_cast<std::uint8_t>(node.flags & ~kNodeDistanceFadeBits);
}

}
