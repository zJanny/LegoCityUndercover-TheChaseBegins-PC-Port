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
// that put its leaf nodes in place (docs/re/doors.md). A leaf's palette entry is its node matrix at model+0x20 followed by the pose; the
// model draw applies it before the placement (0x10e028).
struct DoorObject {
    std::string instance;
    chase::runtime::DoorParams params;
    chase::runtime::DoorState state;
    std::size_t sceneInstance = 0;
    chase::render::Mat4 placement{};
    chase::render::Mat4 rest[2] = {chase::render::identity(), chase::render::identity()};
    float pivot[2][3] = {};
    std::array<float, 3> position{};
    std::uint16_t soundOwner = 0;
};

std::vector<DoorObject> loadDoors(chase::assets::FuseArchive& archive, const chase::assets::Level& level,
                                  const chase::assets::ClassLookup& classOf, const std::vector<chase::assets::ObjectCollision>& objects,
                                  const std::map<std::string, std::pair<std::size_t, chase::render::Mat4>>& instances);

void poseDoor(chase::render::Scene& scene, const DoorObject& door);

}
