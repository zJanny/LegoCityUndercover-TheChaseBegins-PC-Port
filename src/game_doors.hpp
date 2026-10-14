#pragma once
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "assets/object_collision.hpp"
#include "render/scene.hpp"
#include "runtime/door.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace game_doors {

std::string lowerCase(std::string text);

// A door object (classes props/prop_door and leprops/leprop_door): its state machine, the scene instance of its model and the matrices
// that put its node in place (docs/re/doors.md). The node matrix at model+0x20 is applied before the pose and the placement; that order
// is not verified against the original's picture.
struct DoorObject {
    std::string instance;
    chase::runtime::DoorParams params;
    chase::runtime::DoorState state;
    std::size_t sceneInstance[2] = {0, 0};
    bool separateLeaves = false;
    chase::render::Mat4 placement{};
    chase::render::Mat4 rest[2] = {chase::render::identity(), chase::render::identity()};
    float pivot[2][3] = {};
    std::array<float, 3> position{};
    std::uint16_t soundOwner = 0;
};

std::vector<DoorObject> loadDoors(chase::render::Scene& scene, chase::assets::FuseArchive& archive, const chase::assets::Level& level,
                                  const chase::assets::ClassLookup& classOf, const std::vector<chase::assets::ObjectCollision>& objects,
                                  const std::map<std::string, std::pair<std::size_t, chase::render::Mat4>>& instances);

void poseDoor(chase::render::Scene& scene, const DoorObject& door);

}
