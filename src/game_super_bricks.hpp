#pragma once
// The placed super build bricks in chase-game (runtime/super_brick.hpp): the field built from the level's pickup_sbbrick placements, its update
// with the player's node, and the spin and fade of the placed models.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/super_brick.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace game_super_bricks {

struct Result {
    std::vector<std::uint16_t> sounds;  // indices handed to 0x235684 (the UI bank), played without a position
    unsigned saveRequests = 0;          // 0x21cf38(0) of each destroy
};

class LevelSuperBricks {
public:
    static constexpr std::size_t kNoModel = std::size_t(-1);

    void load(const std::vector<chase::assets::SuperBrickObject>& bricks, const std::vector<chase::assets::Placement>& placedObjects);
    // STAND-IN STUD-S9: the player's entity word +0xc bit 4 (the pickup block) counts as clear.
    Result update(float dt, const std::array<float, 3>& playerPosition, const std::function<bool(const std::string& instance)>& disabled);
    // STAND-IN STUD-S8: the idle spin takes the clock from `clockTicks` (the port's own sprite clock, STUD-S3), the rainbow tint of *0x3743f8 is
    // not applied and a collected brick fades in place instead of flying to the HUD.
    void apply(chase::render::Scene& scene, const std::vector<std::size_t>& placedSceneIndex, const std::function<bool(const std::string&)>& disabled,
               std::uint32_t clockTicks);
    std::uint32_t total() const { return field_.total(); }
    void setTotal(std::uint32_t total) { field_.setTotal(total); }
    void spend(std::uint32_t value) { field_.spend(value); }
    const chase::runtime::SuperBrickField& field() const { return field_; }
    std::string report() const;

private:
    chase::runtime::SuperBrickField field_;
    std::vector<std::size_t> placedIndex_;
};

}
