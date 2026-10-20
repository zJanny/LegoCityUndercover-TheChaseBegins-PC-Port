#pragma once
// The super bricks of the brick debris in chase-game (runtime/debris.hpp, docs/re/debris.md): the objects SBDebris01..SBDebris20 of
// levels/charbits.blvl that a super brick record shows, their animation 0x2c4f9c and the block of 0x2cdca4 / 0x2c51e4.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/debris.hpp"
#include "runtime/super_brick.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace game_debris_super {

class SuperDebrisObjects {
public:
    // 0x2b3c48 (the tail of the debris load 0x2ecf30): the SBDebris objects with their SuperBuildValue into the field, one hidden scene instance each.
    void load(chase::render::Scene& scene, chase::assets::FuseArchive& archive, const chase::assets::Level& level, const chase::assets::ClassLookup& classOf,
              chase::runtime::DebrisField& field);
    // 0x2c4f9c after the field's update with `rainbow`, the tint *0x3743f8 of the placed super bricks (game_super_bricks.hpp) as of the last update
    // (0x2600dc is the last step of 0x180168). STAND-IN VEH-S109: the clock 0x5165c8 and the tint phase *0x3743f4 count from the level start.
    void update(float frameDelta, chase::runtime::DebrisField& field, const chase::runtime::SuperBrickTint& rainbow);
    void apply(chase::render::Scene& scene, const chase::runtime::DebrisField& field) const;
    std::size_t instances() const;

private:
    std::array<std::ptrdiff_t, chase::runtime::kSuperDebrisObjectCount> instances_ = filled();
    float clock_ = 0;
    static std::array<std::ptrdiff_t, chase::runtime::kSuperDebrisObjectCount> filled() {
        std::array<std::ptrdiff_t, chase::runtime::kSuperDebrisObjectCount> out{};
        out.fill(-1);
        return out;
    }
};

// 0x2cdca4 with 0x3743f2 set (`enabled`), or 0x2c51e4 before a vehicle contact: the free lists of the SBDebris objects are rebuilt and the block
// gets the node of its object; none without a block or with the flag clear.
std::optional<chase::runtime::DebrisSuperBlock> prepareSuperBlock(chase::runtime::DebrisField& field, const std::optional<chase::runtime::DebrisSuperBlock>& block,
                                                                  const std::array<float, 16>& node, bool enabled);

}
