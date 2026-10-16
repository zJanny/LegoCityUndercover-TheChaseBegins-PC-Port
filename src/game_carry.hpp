#pragma once
#include "assets/assets.hpp"
#include "game_player.hpp"
#include "runtime/carry.hpp"
#include <array>
#include <map>
#include <optional>
#include <string>

namespace game_carry {

// Gives the carry set the timing of the player's pickup and put-down clips (runtime::carryTiming) for big and small objects.
void applyClipTimings(chase::runtime::CarrySet& carry, const game_player::PlayerRig& player);

// Hangs carried objects on the player's hand joint (hook 0x2ce4a4, docs/re/carrying.md).
class HeldObjects {
public:
    // World transform of the object's model while carried: its "carryhandle" node on the joint "weaponright" (big object) or "weaponleft" (small);
    // nothing when the player's skeleton lacks the joint.
    std::optional<chase::render::Mat4> world(chase::assets::FuseArchive& archive, const chase::assets::Placement& placed, bool small,
                                             const game_player::PlayerRig& player);

private:
    std::map<std::string, std::array<float, 16>> offsets_;
};

// Heading (s16 units, 65536 per turn) from the player to a position: atan2(dx, dz) as 0x2011b0 computes it.
std::int16_t headingTowards(const game_player::PlayerRig& player, const float target[3]);

}
