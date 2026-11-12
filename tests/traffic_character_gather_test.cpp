// Tests of src/runtime/traffic_character_gather.*: the grid gather 0x237f30 of the traffic contact step 0x29cc38. The data is made up (a small
// grid of 10 m cells, boxes of 1 m); every expected value follows from the disassembly of 0x237f30, 0x20ea60, 0x22ec3c, 0x2335e4 and 0x2588f4
// (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261112-traffic-B on 2026-11-12,
// docs/re/traffic-character-gather.md) and is evaluated by hand. No emulator was used.
#include "runtime/traffic_character_gather.hpp"
#include "runtime/traffic_driver_connect.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

AiMatrix identityAt(float x, float y, float z) {
    AiMatrix matrix{};
    matrix[0] = {1.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, 1.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, 1.0f};
    matrix[3] = {x, y, z};
    return matrix;
}

TrafficGrid makeGrid() {
    TrafficGrid grid;
    grid.layout.origin = {0.0f, 0.0f, 0.0f};
    grid.layout.inverseCell = {0.1f, 0.1f, 0.1f};
    grid.layout.counts = {4, 2, 4};
    return grid;
}

TrafficGridEntry character(std::uint32_t owner, float x, float y, float z) {
    TrafficGridEntry entry;
    entry.mask = 4;
    entry.kind = 0;
    entry.owner = owner;
    entry.ownerFlags = 0x200;
    entry.ownerNode = owner + 100;
    entry.ownerNodeParent = 0;
    entry.nodeMatrix = identityAt(x, y, z);
    entry.shapeCentre = {0.0f, 0.0f, 0.0f};
    entry.shapeHalf = {1.0f, 1.0f, 1.0f};
    return entry;
}

std::vector<std::uint32_t> owners(const std::vector<TrafficContactEntry>& entries) {
    std::vector<std::uint32_t> out;
    for (const TrafficContactEntry& entry : entries) out.push_back(entry.owner);
    return out;
}

const AiVec3 kWholeCentre = {20.0f, 10.0f, 20.0f};
const AiVec3 kWholeHalf = {30.0f, 30.0f, 30.0f};

void testCellClamp() {
    const TrafficGridLayout layout = makeGrid().layout;
    require((trafficGridCell(layout, {15.0f, 5.0f, 25.0f}) == std::array<std::uint8_t, 3>{1, 0, 2}), "floor(p * 0.1) per axis");
    require((trafficGridCell(layout, {-5.0f, -0.1f, -30.0f}) == std::array<std::uint8_t, 3>{0, 0, 0}), "below the origin clamps to 0");
    require((trafficGridCell(layout, {95.0f, 15.0f, 40.0f}) == std::array<std::uint8_t, 3>{3, 1, 3}), "past the end clamps to count - 1");
}

void testWorldBox() {
    const TrafficWorldBox flat = trafficShapeWorldBox(identityAt(3.0f, 4.0f, 5.0f), {0.5f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f});
    require(flat.centre == AiVec3({3.5f, 4.0f, 5.0f}) && flat.half == AiVec3({1.0f, 2.0f, 3.0f}), "an axis-aligned matrix keeps the half extents");
    AiMatrix turned{};
    turned[0] = {0.0f, 0.0f, 1.0f};
    turned[1] = {0.0f, 1.0f, 0.0f};
    turned[2] = {-1.0f, 0.0f, 0.0f};
    turned[3] = {0.0f, 0.0f, 0.0f};
    const TrafficWorldBox box = trafficShapeWorldBox(turned, {1.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f});
    require(box.centre == AiVec3({0.0f, 0.0f, 1.0f}), "the centre goes through the matrix");
    require(box.half == AiVec3({3.0f, 2.0f, 1.0f}), "a quarter turn about y swaps the x and z half extents");
}

void testCellWalkOrder() {
    TrafficGrid grid = makeGrid();
    grid.entries.push_back(character(1, 35.0f, 5.0f, 5.0f));   // cell (3,0,0)
    grid.entries.push_back(character(2, 5.0f, 15.0f, 5.0f));   // cell (0,1,0)
    grid.entries.push_back(character(3, 5.0f, 5.0f, 25.0f));   // cell (0,0,2)
    grid.entries.push_back(character(4, 5.0f, 5.0f, 5.0f));    // cell (0,0,0)
    const TrafficContactDescriptor descriptor;
    const std::vector<std::uint32_t> found = owners(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 0, 128));
    require((found == std::vector<std::uint32_t>{4, 1, 3, 2}), "y outermost, then z, then x: (0,0,0) (3,0,0) (0,0,2) (0,1,0)");
}

void testEntrySpanningCellsListedOnce() {
    TrafficGrid grid = makeGrid();
    TrafficGridEntry big = character(7, 10.0f, 5.0f, 5.0f);
    big.shapeHalf = {3.0f, 1.0f, 1.0f};
    grid.entries.push_back(big);
    const TrafficContactDescriptor descriptor;
    const std::vector<TrafficContactEntry> found = gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 0, 128);
    require(found.size() == 1 && found[0].owner == 7, "an entry in two cells is listed once");
    require(found[0].word == 4 && found[0].ownerNode == 107 && found[0].ownerNodeParent == 0, "the entry word is the mask, with the owner's node");
}

void testFilters() {
    TrafficGrid grid = makeGrid();
    const auto add = [&](std::uint32_t owner) -> TrafficGridEntry& {
        grid.entries.push_back(character(owner, 5.0f + float(owner), 5.0f, 5.0f));
        return grid.entries.back();
    };
    add(1);
    add(2).mask = 6;                          // mask 6 shares bit 4 with the include mask 4
    add(3).mask = 1;                          // no shared bit
    add(4).kind = 1;
    add(5).ownerFlags = 0x201;                // owner hword +0x10 bit 0 in the exclude mask 3
    add(6).ownerFlags = 0x100;                // no bit of the include mask 0x200
    add(7).ownerWord = 0x10;                  // owner +0xc bit 4 in the exclude mask 16
    add(8).nodeSkipped = true;
    add(9);                                   // the excluded owner
    TrafficContactDescriptor descriptor;
    descriptor.exclude = 2;
    require((owners(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 9, 128)) == std::vector<std::uint32_t>{1, 4}),
            "mask, exclude mask, owner flags, owner word, node bit and the excluded owner filter; kind 1 passes the default kind mask");
    descriptor.kindsExclude = 2;
    require((owners(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 9, 128)) == std::vector<std::uint32_t>{1}), "kind 1 in the exclude kinds");
    descriptor.kindsExclude = 0;
    descriptor.kindsInclude = 2;
    require(owners(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 9, 128)) == std::vector<std::uint32_t>{4}, "kind 1 only");
    descriptor.kindsInclude = 0xffffffffu;
    descriptor.ownerFlagsInclude = 0x20;
    require(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 9, 128).empty(), "owner word include bit 0x20 is set on none");
}

void testBoxOverlapAndMode() {
    TrafficGrid grid = makeGrid();
    grid.entries.push_back(character(1, 2.0f, 5.0f, 5.0f));
    grid.entries.push_back(character(2, 9.0f, 5.0f, 5.0f));   // same cell, 9 m away
    TrafficContactDescriptor descriptor;
    require((owners(gatherTrafficGrid(grid, {2.0f, 5.0f, 5.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 0, 128)) == std::vector<std::uint32_t>{1}),
            "mode 1: the entry at 9 m is out of the box (|7| > 2 + 1)");
    require((owners(gatherTrafficGrid(grid, {5.0f, 5.0f, 5.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 0, 128)) == std::vector<std::uint32_t>{1}),
            "mode 1: |3| <= 2 + 1 keeps the entry at 2, |4| > 3 drops the entry at 9");
    descriptor.mode = 0;
    require((owners(gatherTrafficGrid(grid, {2.0f, 5.0f, 5.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 0, 128)) == std::vector<std::uint32_t>{1, 2}),
            "mode 0: no box test, every entry of the cells passes");
}

void testCap() {
    TrafficGrid grid = makeGrid();
    for (std::uint32_t owner = 1; owner <= 5; ++owner) grid.entries.push_back(character(owner, 5.0f, 5.0f, 5.0f));
    const TrafficContactDescriptor descriptor;
    require((owners(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 0, 3)) == std::vector<std::uint32_t>{1, 2, 3}),
            "the list holds at most maxEntries, the first ones of the walk");
    require(gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 0, 0).empty(), "a cap of 0 lists nothing");
}

void testExtraMatrixMultiplied() {
    TrafficGrid grid = makeGrid();
    TrafficGridEntry entry = character(1, 0.0f, 5.0f, 25.0f);
    entry.extraMatrix = identityAt(5.0f, 0.0f, -20.0f);
    grid.entries.push_back(entry);
    const TrafficContactDescriptor descriptor;
    require(owners(gatherTrafficGrid(grid, {5.0f, 5.0f, 5.0f}, {1.0f, 1.0f, 1.0f}, descriptor, 0, 128)) == std::vector<std::uint32_t>{1},
            "the extra matrix times the node matrix puts the shape at (5, 5, 5)");
    require(gatherTrafficGrid(grid, {0.0f, 5.0f, 25.0f}, {1.0f, 1.0f, 1.0f}, descriptor, 0, 128).empty(), "and not at the node matrix alone");
}

void testDisabledGridAndModeTwo() {
    TrafficGrid grid = makeGrid();
    grid.entries.push_back(character(1, 5.0f, 5.0f, 5.0f));
    TrafficContactDescriptor descriptor;
    TrafficGrid empty = grid;
    empty.layout.counts = {4, 0, 4};
    require(gatherTrafficGrid(empty, kWholeCentre, kWholeHalf, descriptor, 0, 128).empty(), "a grid without cells lists nothing");
    descriptor.mode = 2;
    bool threw = false;
    try {
        gatherTrafficGrid(grid, kWholeCentre, kWholeHalf, descriptor, 0, 128);
    } catch (const std::logic_error&) {
        threw = true;
    }
    require(threw, "mode 2 is not ported and says so");
}

void testGatherHookExcludesTheCar() {
    TrafficGrid grid = makeGrid();
    grid.entries.push_back(character(11, 5.0f, 5.0f, 5.0f));
    grid.entries.push_back(character(12, 6.0f, 5.0f, 5.0f));
    const TrafficContactGather gather = makeTrafficContactGather(grid, [](std::size_t car) { return std::uint32_t(11 + car); });
    const TrafficContactDescriptor descriptor;
    TrafficDriveStep step;
    step.gatherContacts = gather;
    require(owners(step.gatherContacts(0, {5.0f, 5.0f, 5.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 128)) == std::vector<std::uint32_t>{12}, "car 0 is owner 11");
    require(owners(step.gatherContacts(1, {5.0f, 5.0f, 5.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 128)) == std::vector<std::uint32_t>{11}, "car 1 is owner 12");
}

}

int main() {
    try {
        testCellClamp();
        testWorldBox();
        testCellWalkOrder();
        testEntrySpanningCellsListedOnce();
        testFilters();
        testBoxOverlapAndMode();
        testCap();
        testExtraMatrixMultiplied();
        testDisabledGridAndModeTwo();
        testGatherHookExcludesTheCar();
    } catch (const std::exception& error) {
        std::cerr << "traffic-character-gather: " << error.what() << "\n";
        return 1;
    }
    std::cout << "traffic-character-gather: ok\n";
    return 0;
}
