#pragma once
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/floor_walk.hpp"
#include "runtime/room_locator.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <vector>

namespace chase::runtime {

// STAND-IN PREGEN-S2: the loaders walk the loaded levels in the order of the list at 0x589a18 and the current room is the
// global 0x376c78; neither is known at the time the managers initialise, so the caller supplies both (0 = no current room).
struct PregenRoomContext {
    const StreamRooms* rooms = nullptr;
    const std::map<int, RoomBox>* boxes = nullptr;
    int currentRoom = 0;
};

struct PregenDerivedFields {
    bool pathFound = false;
    std::size_t levelIndex = 0;
    std::size_t pathIndex = 0;
    std::array<float, 3> tangent{};
    int room = 0;
};

// The fields the loaders 0x2e02d4 / 0x2ee868 derive per point: the path record (first loaded level whose path section holds the
// hash, 0x1b3d04), the unit tangent the path sampler 0x1c17c0 stores at the stored parameter, and the room 0x239838.
// The sampler's node transform does not apply: path +0x18 is set (0x1c1b48) only by the natives EffectAttachParticlesToPath and
// EffectLinkPathToObject, which no script calls.
// A point whose path is not found keeps a zero tangent (the original leaves the stale file word) and still gets a room.
// Throws for a path of a type other than 0 (the other sampler table entries are not ported).
std::vector<PregenDerivedFields> deriveSpawnFields(const assets::PregenSpawns& spawns,
                                                   const std::vector<const assets::Level*>& loadedLevels,
                                                   const PregenRoomContext& context);

// 0x15f82c: whether the point lies inside the cell's triangle in the xz plane (three edge tests against the corners, each side negative).
bool floorCellContains(const FloorMesh& mesh, int cell, const FloorPoint& point);

// 0x1edaac(point, 0.0), the traffic floor cell the loader 0x2e02d4 stores at point +40, over the "collision_floor" cells (0x376c60 count,
// 0x376c68 cells): a bisection on the 3D distance from the point to the first corner of the cells at the two ends, then the cells outward
// from both ends, one up and one down per step, until one contains the point. Cell 0 is never tested (the downward scan stops at it
// without testing). -1 when no cell contains it. The original loops forever when the bisection leaves the high end on cell 0 (a single
// cell, or two cells with the first closer); this throws instead.
int findTrafficFloorCell(const FloorMesh& mesh, const FloorPoint& point);

// The floor cell of every point of a traffic file; all -1 when the level has no "collision_floor" item (mesh null or without a floor graph).
std::vector<int> deriveTrafficFloorCells(const assets::PregenSpawns& spawns, const FloorMesh* mesh);

}
