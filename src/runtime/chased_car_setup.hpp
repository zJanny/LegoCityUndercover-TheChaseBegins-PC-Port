#pragma once
// The rest of VehicleSetChased 0x1afd38 (docs/re/traffic-driver-states.md): the length table of the path that 0x22a6c0(path + 8, 10) builds
// (path +0x14 / +0x18) and the node test at 0x1afe04..0x1afe30. No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>
#include <vector>

namespace chase::runtime {

constexpr unsigned kChasedPathLengthSteps = 10;  // mov r1, #10 at 0x1afd84

// 0x22a6c0: entry 0 is 0.0 and entry i adds slot 1 of the path type (0x293540 for type 0) with 10 steps from point i - 1 to i; a closed path has
// count + 1 entries. The total (path +0x18) is the last entry, 0.0 for a path without points.
struct ChasedPathTable {
    std::vector<float> lengths;
    float total = 0.0f;
};
ChasedPathTable chasedPathTable(const assets::LevelPath& path);

// The bytes of the node that the test sets: the alpha byte +0xdd (255 from the node's creation 0x10d2d4) and the flags +0xde, whose bit 4 is
// the distance fade (EnableDistanceFade) and bit 5 its mid tier.
constexpr std::uint8_t kNodeFadeOpaqueByte = 255;
constexpr std::uint8_t kNodeDistanceFadeBits = 0x30;
struct ChasedNodeBytes {
    std::uint8_t alpha = kNodeFadeOpaqueByte;
    std::uint8_t flags = 0;
};

// 0x1afe04..0x1afe30: when the node word 0 & 31 equals *0x37674c (the mesh node type, `meshNode`), +0xdd = 255 and +0xde loses bits 4 and 5.
void chasedNodeTest(bool meshNode, ChasedNodeBytes& node);

}
