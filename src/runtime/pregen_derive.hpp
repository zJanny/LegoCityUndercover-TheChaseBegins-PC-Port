#pragma once
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
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
// STAND-IN PREGEN-S3: the sampler's node transform (path +0x18, stored by 0x1c1b48 whose callers are 0x2a83b4 and 0x2bac44) is taken as unset.
// A point whose path is not found keeps a zero tangent (the original leaves the stale file word) and still gets a room.
// Throws for a path of a type other than 0 (the other sampler table entries are not ported).
std::vector<PregenDerivedFields> deriveSpawnFields(const assets::PregenSpawns& spawns,
                                                   const std::vector<const assets::Level*>& loadedLevels,
                                                   const PregenRoomContext& context);

}
