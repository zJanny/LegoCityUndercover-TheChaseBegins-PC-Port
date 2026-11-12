// Tests of src/runtime/traffic_character_feed.*: the grid layout of 0x271280 (the grid build), the cell lists that 0x1f5cd0 / 0x209cb8 keep
// and the feed of characters to the contact gather 0x237f30. The data is made up; every expected value is evaluated by hand from the
// disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, 0x271280..0x271750, 0x209cb8..0x20a194,
// 0x1f5cd0, 0x20ea60, 0x237f30; read by 20261112-trafficfeed-B on 2026-11-12, docs/re/traffic-character-gather.md). No emulator was used.
#include "runtime/traffic_character_feed.hpp"
#include "runtime/traffic_driver_steering.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
bool throws(Function function) {
    try {
        function();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

AiMatrix at(float x, float y, float z) {
    AiMatrix matrix{};
    matrix[0] = {1.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, 1.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, 1.0f};
    matrix[3] = {x, y, z};
    return matrix;
}

TrafficFedCharacter character(std::uint32_t id, float x, float y, float z) {
    TrafficFedCharacter fed;
    fed.id = id;
    fed.node = at(x, y, z);
    fed.shapeCentre = {0.0f, 0.0f, 0.0f};
    fed.shapeHalf = {1.0f, 1.0f, 1.0f};
    return fed;
}

TrafficGridLayout tenMetreLayout() {
    TrafficGridLayout layout;
    layout.origin = {0.0f, 0.0f, 0.0f};
    layout.inverseCell = {0.1f, 0.1f, 0.1f};
    layout.counts = {4, 2, 4};
    return layout;
}

std::vector<std::uint32_t> owners(const std::vector<TrafficContactEntry>& entries) {
    std::vector<std::uint32_t> out;
    for (const TrafficContactEntry& entry : entries) out.push_back(entry.owner);
    return out;
}

std::vector<std::uint32_t> gatherAll(const TrafficCharacterFeed& feed) {
    return owners(gatherTrafficGrid(feed.grid(), {20.0f, 10.0f, 20.0f}, {30.0f, 30.0f, 30.0f}, TrafficContactDescriptor{}, 0, 128));
}

TrafficWorldBox box(float cx, float cy, float cz, float hx, float hy, float hz) {
    TrafficWorldBox out;
    out.centre = {cx, cy, cz};
    out.half = {hx, hy, hz};
    return out;
}

// 0x271280 by hand: both sets cover x 0..100, y 0..10, z 0..200, so the extent is (100, 10, 200) and the cell size starts as the extent
// (counts 1, 1, 1, product 1). Each pass halves the largest cell size (a tie goes to the later axis: x = z halves z, y = z halves z) and
// recounts: z 100 (product 2), z 50 (4), x 50 (8), z 25 (16), x 25 (32), z 12.5 (64), x 12.5 (128), z 6.25 (256), x 6.25 (512),
// y 5 (1024; x 6.25 < y 10 takes the second branch, y > z), z 3.125 (2048, stop). The counts are 16, 2, 64.
void testLayoutBuild() {
    const TrafficGridLayout layout = buildTrafficGridLayout({box(50, 5, 100, 50, 5, 100)}, {box(50, 5, 100, 50, 5, 100)});
    require(layout.counts == std::array<std::uint8_t, 3>({16, 2, 64}), "the halving ends at the counts 16, 2, 64");
    require(layout.origin == AiVec3({0.0f, 0.0f, 0.0f}), "the origin is the low corner");
    require(layout.inverseCell[0] == 1.0f / 6.25f && layout.inverseCell[1] == 1.0f / 5.0f && layout.inverseCell[2] == 1.0f / 3.125f,
            "the inverse cell sizes are 1 / (6.25, 5, 3.125)");
    require(trafficGridCell(layout, {99.0f, 9.0f, 199.0f}) == std::array<std::uint8_t, 3>({15, 1, 63}), "the last cell");
}

void testLayoutIntersection() {
    const TrafficGridLayout wide = buildTrafficGridLayout({box(50, 5, 100, 80, 20, 150), box(60, 5, 100, 10, 1, 1)}, {box(50, 5, 100, 50, 5, 100)});
    const TrafficGridLayout same = buildTrafficGridLayout({box(50, 5, 100, 50, 5, 100)}, {box(50, 5, 100, 50, 5, 100)});
    require(wide.counts == same.counts && wide.origin == same.origin && wide.inverseCell == same.inverseCell,
            "the extent is the intersection of the bounds of the two sets: the room bounds inside the larger shape bounds");
    const TrafficGridLayout shifted = buildTrafficGridLayout({box(50, 5, 100, 50, 5, 100)}, {box(80, 5, 100, 50, 5, 100)});
    require(shifted.origin == AiVec3({30.0f, 0.0f, 0.0f}), "the low corner is the larger of the two low corners");
    require(shifted.counts == std::array<std::uint8_t, 3>({16, 2, 64}),
            "the extent (70, 10, 200) halves z, z, x, z, x, z, x, z, y, x, z (cell sizes 4.375, 5, 3.125) to the counts 16, 2, 64");
}

void testLayoutRejects() {
    require(throws([] { buildTrafficGridLayout({}, {box(0, 0, 0, 1, 1, 1)}); }), "an empty shape set is rejected");
    require(throws([] { buildTrafficGridLayout({box(0, 0, 0, 1, 1, 1)}, {}); }), "an empty room set is rejected");
    require(throws([] { buildTrafficGridLayout({box(0, 0, 0, 1, 1, 1)}, {box(100, 0, 0, 1, 1, 1)}); }), "disjoint bounds (negative extent) are rejected");
    require(throws([] { buildTrafficGridLayout({box(0, 0, 0, 1, 0, 1)}, {box(0, 0, 0, 1, 0, 1)}); }), "a flat bounds set (zero extent) is rejected");
}

// The cell index of 0x209cb8 and 0x237f30 is (z x count z + y) x count x + x: with the counts (4, 2, 4), cell (1, 1, 2) is (2 x 4 + 1) x 4 + 1.
void testCellIndex() {
    const TrafficGridLayout layout = tenMetreLayout();
    require(trafficGridCellIndex(layout, {1, 1, 2}) == 37, "cell (1, 1, 2) is index 37");
    require(trafficGridCellIndex(layout, {0, 0, 2}) == 32, "cell (0, 0, 2) is index 32");
}

// 0x209cb8: a new entry is appended to every cell of its box (y, z, x order) and a moved one is appended to the new cells and replaced by the
// last entry of the cells it leaves.
void testCellListsFollowTheCellUpdate() {
    TrafficCharacterFeed feed(tenMetreLayout());
    feed.update({character(1, 5, 5, 5), character(2, 6, 5, 5), character(3, 4, 5, 5)});
    require(feed.grid().cellLists[0] == std::vector<std::uint32_t>({0, 1, 2}), "three entries in cell 0 in registration order");
    require(gatherAll(feed) == std::vector<std::uint32_t>({1, 2, 3}), "the gather lists the cell 0 in list order");

    feed.update({character(1, 35, 5, 5), character(2, 6, 5, 5), character(3, 4, 5, 5)});
    require(feed.grid().cellLists[0] == std::vector<std::uint32_t>({2, 1}), "entry 0 leaves cell 0 and the last entry takes its place");
    require(feed.grid().cellLists[3] == std::vector<std::uint32_t>({0}), "and joins cell (3, 0, 0)");
    require(gatherAll(feed) == std::vector<std::uint32_t>({3, 2, 1}), "the walk lists cell 0 first (3, 2) then cell 3 (1)");

    feed.update({character(1, 35, 5, 5), character(3, 4, 5, 5)});
    require(feed.grid().cellLists[0] == std::vector<std::uint32_t>({2, 1}), "a character that is not listed keeps its cells (no entry is ever removed)");
    require(gatherAll(feed) == std::vector<std::uint32_t>({3, 2, 1}), "and is still gathered");

    TrafficFedCharacter hidden = character(2, 6, 5, 5);
    hidden.disabled = true;
    feed.update({character(1, 35, 5, 5), hidden, character(3, 4, 5, 5)});
    require(feed.grid().cellLists[0] == std::vector<std::uint32_t>({2, 1}), "a disabled character stays in its cells");
    require(gatherAll(feed) == std::vector<std::uint32_t>({3, 1}), "but owner flag bit 0 keeps it out of the gather");

    feed.update({character(1, 35, 5, 5), character(2, 6, 5, 5), character(3, 4, 5, 5)});
    require(gatherAll(feed) == std::vector<std::uint32_t>({3, 2, 1}), "enabled again it is gathered at its place");
    require(feed.grid().entries.size() == 3, "without a new table slot");
}

// 0x30e0d8 / 0x2ba3f4 give a new entry the cell bytes 255, 255, 255 and 0, 0, 0 (docs/re/traffic-character-gather.md, 20261112-enemy-C).
void testNewEntryCellBytes() {
    TrafficGrid grid;
    grid.layout = tenMetreLayout();
    grid.cellLists.resize(32);
    TrafficGridEntry entry;
    entry.mask = kTrafficCharacterMask;
    entry.owner = 9;
    entry.nodeMatrix = at(5, 5, 5);
    entry.shapeHalf = {1.0f, 1.0f, 1.0f};
    const std::size_t index = trafficGridAdd(grid, entry);
    require(grid.entries[index].cells && grid.entries[index].cells->low == std::array<std::uint8_t, 3>({0, 0, 0}), "the first update stores the box's cells");
    require(grid.cellLists[0] == std::vector<std::uint32_t>({0}), "and joins the cell without leaving any");
    require(kTrafficGridNewEntryCells.low == std::array<std::uint8_t, 3>({255, 255, 255}) &&
                kTrafficGridNewEntryCells.high == std::array<std::uint8_t, 3>({0, 0, 0}),
            "a new entry holds 255 255 255 / 0 0 0");
}

void testBoxAcrossCells() {
    TrafficCharacterFeed feed(tenMetreLayout());
    TrafficFedCharacter wide = character(7, 10, 5, 5);
    wide.shapeHalf = {2.0f, 1.0f, 1.0f};
    feed.update({wide});
    require(feed.grid().cellLists[0] == std::vector<std::uint32_t>({0}) && feed.grid().cellLists[1] == std::vector<std::uint32_t>({0}),
            "a box over x 8..12 is in the cells 0 and 1");
    require(gatherAll(feed) == std::vector<std::uint32_t>({7}), "and is listed once (the visited mark)");
    feed.update({character(7, 5, 5, 5)});
    require(feed.grid().cellLists[0] == std::vector<std::uint32_t>({0}) && feed.grid().cellLists[1].empty(), "moved to x 5 it leaves cell 1");
}

// The contact step's gather of a car: the box around the car and the descriptor of 0x29cc38 (mask 4, owner flag 0x200).
void testCarNearCharacter() {
    TrafficCharacterFeed feed(tenMetreLayout());
    feed.update({character(kTrafficFeedPlayerId, 30, 5, 30), character(kTrafficFeedPedestrianBase + 4, 12, 5, 12)});
    const TrafficContactGather gather = makeTrafficContactGather(feed.grid(), [](std::size_t car) { return kTrafficFeedCarBase + std::uint32_t(car); });
    const TrafficContactDescriptor descriptor;
    const std::vector<TrafficContactEntry> near = gather(0, {13.0f, 5.0f, 13.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 128);
    require(near.size() == 1 && near[0].owner == kTrafficFeedPedestrianBase + 4 && near[0].word == kTrafficCharacterMask && near[0].ownerNode != 0,
            "a car beside the pedestrian gets its entry");
    require(gather(0, {20.0f, 5.0f, 20.0f}, {1.0f, 1.0f, 1.0f}, descriptor, 128).empty(), "a car between the two characters gets none");
    require(gather(0, {13.0f, 5.0f, 13.0f}, {2.0f, 2.0f, 2.0f}, descriptor, 0).empty(), "a cap of 0 lists none");
    TrafficContactDescriptor excluding = descriptor;
    excluding.ownerStateInclude = 0x100;
    require(gather(0, {13.0f, 5.0f, 13.0f}, {2.0f, 2.0f, 2.0f}, excluding, 128).empty(), "a descriptor that wants owner flag 0x100 lists no character");
}

void testFeedRejects() {
    TrafficCharacterFeed feed(tenMetreLayout());
    require(throws([&] { feed.update({character(0, 1, 1, 1)}); }), "an id of 0 is rejected");
    require(throws([&] { feed.update({character(5, 1, 1, 1), character(5, 2, 2, 2)}); }), "an id listed twice is rejected");
}

}

int main() {
    try {
        testLayoutBuild();
        testLayoutIntersection();
        testLayoutRejects();
        testCellIndex();
        testCellListsFollowTheCellUpdate();
        testNewEntryCellBytes();
        testBoxAcrossCells();
        testCarNearCharacter();
        testFeedRejects();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic character feed: ok\n";
    return 0;
}
