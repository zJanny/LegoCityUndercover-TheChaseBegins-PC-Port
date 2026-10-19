#pragma once
// The dropped studs in chase-game (runtime/stud_drop.hpp): the field on the game's random generator and the wall planes of the bricks, its update
// with the level studs' pickup point and HUD sink, and its sprites.
#include "render/studs.hpp"
#include "runtime/sound_events.hpp"
#include "runtime/stud_drop.hpp"

#include <optional>
#include <vector>

namespace game_stud_drops {

class LevelStudDrops {
public:
    void load(chase::runtime::GameRandom& random);
    // The wall planes of the bursts, shared with the bricks (LevelEffects).
    chase::runtime::DebrisPlaneRing& planes() { return planes_; }
    chase::runtime::StudDropField* field() { return field_ ? &*field_ : nullptr; }
    std::optional<chase::runtime::StudDropMode> drop(const std::optional<chase::runtime::StudDropRequest>& request, const chase::runtime::DebrisLineTest& wall,
                                                     const chase::runtime::DebrisLineTest& ground);
    // STAND-IN STUD-S6: the volume and frequency scale of the sounds are not applied.
    std::vector<chase::runtime::SoundCall> update(float frameDelta, const std::array<float, 3>& pickupPoint,
                                                  const std::optional<std::array<float, 3>>& flightTarget, chase::runtime::StudField& counter);
    // STAND-IN STUD-S6: a dropped heart is not drawn (no Pickups_heart sprite).
    void appendSprites(std::vector<chase::render::StudSprite>& sprites) const;

private:
    std::optional<chase::runtime::StudDropField> field_;
    chase::runtime::DebrisPlaneRing planes_;
};

}
