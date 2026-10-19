#pragma once
// The first callers of the dropped studs (docs/re/studs.md, entry "First callers of the dropped studs"): the parameters 0x2149ac reads from an
// object (SpawnCone, SpawnLocal, SpawnDir, SpawnLoc), the non-lethal wobble hit 0x2bc370 -> 0x23a6b0 and the finished build-it 0x1aad90. The car
// break 0x16cccc and the destroy of a wobbly prop contain no stud call. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/stud_drop.hpp"

#include <array>
#include <optional>

namespace chase::runtime {

// The instance parameters of 0x2149ac: SpawnCone (float, default 1.0), SpawnLocal (int, 0), SpawnDir (int, 999 none) and SpawnLoc, the object
// reference whose node replaces the entity's (0x23a6b0 finds it; 0x1aad90 passes the build-it's final prop instead). STAND-IN STUD-S7: a SpawnLoc
// target is taken at its placement (a moved node and a target of kind 11, 0x22faf8, are not read).
struct StudSpawnSource {
    float spawnCone = 1.0f;
    std::int32_t spawnDir = 999;
    std::int32_t spawnLocal = 0;
    bool hasSpawnLoc = false;
    std::array<float, 16> spawnLocNode{};
};

StudSpawnSource studSpawnSource(const assets::Level& level, const assets::LevelObject& object, const assets::Level& cls);

// 0x2bc370: 0x23a6b0(entity, WobbleStudValue, 1, 1, 1) -> 0x2149ac(entity, SpawnLoc, count, forward 1, mode flag 1): mode 0 (count), the direction
// SpawnDir or the node's row 2. `node` is the entity's world matrix.
std::optional<StudDropRequest> wobbleHitStudRequest(const StudSpawnSource& source, unsigned wobbleStudValue, const std::array<float, 16>& node);

// 0x1aad90, change to state 11: 0x2149ac(build-it, final prop, StudValue, forward 1, mode flag 0): mode 1 (random), the position and forward axis
// of the final prop's world matrix.
std::optional<StudDropRequest> builditStudRequest(const StudSpawnSource& source, float studValue, const std::array<float, 16>& finalPropNode);

}
