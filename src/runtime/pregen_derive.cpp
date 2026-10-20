#include "runtime/pregen_derive.hpp"

#include "runtime/detective.hpp"

#include <stdexcept>

namespace chase::runtime {

std::vector<PregenDerivedFields> deriveSpawnFields(const assets::PregenSpawns& spawns,
                                                   const std::vector<const assets::Level*>& loadedLevels,
                                                   const PregenRoomContext& context) {
    static const StreamRooms noRooms;
    static const std::map<int, RoomBox> noBoxes;
    const StreamRooms& rooms = context.rooms ? *context.rooms : noRooms;
    const std::map<int, RoomBox>& boxes = context.boxes ? *context.boxes : noBoxes;
    std::vector<PregenDerivedFields> fields;
    fields.reserve(spawns.points.size());
    for (const assets::PregenSpawnPoint& point : spawns.points) {
        PregenDerivedFields derived;
        for (std::size_t level = 0; level < loadedLevels.size() && !derived.pathFound; ++level) {
            const std::vector<assets::LevelPath>& paths = loadedLevels[level]->paths;
            for (std::size_t index = 0; index < paths.size(); ++index) {
                if (paths[index].nameHash != point.pathHash) continue;
                if (paths[index].type != 0) throw std::runtime_error("pregen point on a path of type other than 0");
                derived.pathFound = true;
                derived.levelIndex = level;
                derived.pathIndex = index;
                detectivePathTangent(paths[index], point.pathPosition, derived.tangent.data());
                break;
            }
        }
        const Vec3 position = {point.position[0], point.position[1], point.position[2]};
        derived.room = objectRoomAfterMove(rooms, boxes, 0, context.currentRoom, position);
        fields.push_back(derived);
    }
    return fields;
}

}
