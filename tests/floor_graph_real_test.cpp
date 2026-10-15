// Real-data test of the floor graph of a collision item (assets::FloorGraph, loader 0x11fd20; docs/re/vehicles-physics.md). Pinned values: Azahar
// GDB reads of 2026-10-15 (agent VEH40, state drv-police-car-seated, lvl_01): 0x376c60 = 0x5ca1 cells, the cells at *0x376c68 (cell 0: index 0,
// neighbours 0x45, 0x7f, 1; cell 2: 3, 0xa3, 0x51; cell 3 flags 0x6000; cell 0x52fd under the trooper: neighbours 0x5322, 0x5795, 0x52fc,
// flags 0x2000) and the first extra triangle at *0x376c64 (normal 0x3e252a9b 0x3f7c919a 0x3cca7131, d 0x4343f57c). Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/floor_walk.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace {

using namespace chase;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

float bitsToFloat(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}

void cell(const assets::FloorGraph& graph, std::uint32_t index, std::uint16_t runtimeFlags, std::uint16_t a, std::uint16_t b, std::uint16_t c,
          const char* what) {
    const assets::FloorCell& entry = graph.cells.at(index);
    const std::uint16_t flags = std::uint16_t(entry.flags | (entry.itemTriangle ? 0x2000 : 0));
    expect(entry.index == index && flags == runtimeFlags && entry.neighbour[0] == a && entry.neighbour[1] == b && entry.neighbour[2] == c, what);
}

// Oracle trap of 0x23b5e8 and its return sites in 0x1c54f8 (agent VEH40 walk.txt, drv-police-car-seated, A held 5 updates): the eight body
// points of the collision pass from the centre of mass (Clip, return 0x1c5700 and 0x1c5e28) and the half-step query (Stay, return 0x1c5818).
struct Walk {
    int cell;
    runtime::FloorWalkEdge edge;
    runtime::FloorPoint from, to;
    int result;
};
const Walk kWalks[] = {
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {884.055237f, 16.9901905f, 1112.58813f}, 21245},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {882.679626f, 16.9936886f, 1113.95483f}, 21244},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {880.159912f, 17.109293f, 1108.66724f}, 21243},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {878.784302f, 17.1127911f, 1110.03406f}, 21243},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {880.306396f, 18.7553997f, 1108.948f}, 21243},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {879.068359f, 18.7585487f, 1110.1781f}, 21244},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {883.812134f, 18.6482086f, 1112.47681f}, 21245},
    {21244, runtime::FloorWalkEdge::Clip, {881.352539f, 17.0494823f, 1111.24341f}, {882.574097f, 18.6513557f, 1113.70691f}, 21244},
    {21244, runtime::FloorWalkEdge::Stay, {881.352539f, 17.0494823f, 1111.24341f}, {881.31958f, 17.0488644f, 1111.21021f}, 21244},
    {21244, runtime::FloorWalkEdge::Clip, {881.286621f, 17.0482464f, 1111.177f}, {883.989258f, 16.9858265f, 1112.52161f}, 21245},
    {21244, runtime::FloorWalkEdge::Clip, {881.286621f, 17.0482464f, 1111.177f}, {878.718445f, 17.1144733f, 1109.96765f}, 21243},
    {21244, runtime::FloorWalkEdge::Clip, {881.286621f, 17.0482464f, 1111.177f}, {883.747498f, 18.6441174f, 1112.41162f}, 21244},
    {21244, runtime::FloorWalkEdge::Clip, {881.286621f, 17.0482464f, 1111.177f}, {882.50946f, 18.6472607f, 1113.64172f}, 21244},
};

void walks(const assets::CollisionItem& floor) {
    const runtime::FloorMesh mesh(floor);
    for (const Walk& walk : kWalks) {
        runtime::FloorPoint to = walk.to;
        const int got = runtime::walkFloor(mesh, walk.cell, walk.from, to, walk.edge);
        if (got != walk.result) std::printf("walk from cell %d: got %d, oracle %d\n", walk.cell, got, walk.result);
        expect(got == walk.result, "0x23b5e8 ends in the oracle's cell");
        expect(to == walk.to, "a walk inside the graph leaves the end point alone");
    }
    expect(runtime::floorNeighbour(0xffff) == -1 && runtime::floorNeighbour(0xfffd) == -3 && runtime::floorNeighbour(0xfffc) == 0xfffc &&
               runtime::floorNeighbour(0x8000) == 0x8000,
           "neighbour markers -1..-3, other values unsigned");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::printf("floor-graph-test: data not available, skipped\n");
        return 77;
    }
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    const assets::FuseEntry* entry = archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl");
    if (!entry) {
        std::printf("FAIL: level model not in archive\n");
        return 1;
    }
    const assets::Model model = assets::parseModel(archive.read(*entry));
    const assets::CollisionItem* floor = nullptr;
    int graphs = 0;
    for (const assets::CollisionItem& item : model.collision)
        if (item.floor) {
            ++graphs;
            floor = &item;
        }
    expect(graphs == 1 && floor && floor->name == "Collision_Floor", "only Collision_Floor has a floor graph");
    if (!floor) return 1;
    const assets::FloorGraph& graph = *floor->floor;
    expect(graph.cells.size() == 0x5ca1, "cell count *0x376c60 = 0x5ca1");
    cell(graph, 0, 0x2000, 0x45, 0x7f, 0x1, "cell 0");
    cell(graph, 1, 0x2000, 0x47, 0x0, 0x3, "cell 1");
    cell(graph, 2, 0x2000, 0x3, 0xa3, 0x51, "cell 2");
    cell(graph, 3, 0x6000, 0x7b, 0x2, 0x1, "cell 3");
    cell(graph, 0x52fd, 0x2000, 0x5322, 0x5795, 0x52fc, "cell 0x52fd under the parked trooper");
    expect(!graph.extraTriangles.empty(), "extra triangles present");
    if (!graph.extraTriangles.empty()) {
        const assets::CollisionTriangle& t = graph.extraTriangles[0];
        expect(t.normal[0] == bitsToFloat(0x3e252a9bu) && t.normal[1] == bitsToFloat(0x3f7c919au) && t.normal[2] == bitsToFloat(0x3cca7131u) &&
                   t.d == bitsToFloat(0x4343f57cu),
               "extra triangle 0 plane");
    }
    walks(*floor);
    std::size_t extraCells = 0;
    for (const assets::FloorCell& c : graph.cells) extraCells += !c.itemTriangle;
    std::printf("floor graph: %zu cells (%zu on extra triangles), %zu extra triangles, %zu extra vertices, header %u %u %u\n", graph.cells.size(),
                extraCells, graph.extraTriangles.size(), graph.extraVertices.size() / 3, graph.header[0], graph.header[2], graph.header[4]);
    if (failures) return 1;
    std::printf("floor graph tests passed\n");
    return 0;
}
