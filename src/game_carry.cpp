#include "game_carry.hpp"
#include "runtime/carry_animation.hpp"
#include "runtime/player_locomotion.hpp"
#include <cmath>

namespace game_carry {
using namespace chase;

void applyClipTimings(runtime::CarrySet& carry, const game_player::PlayerRig& player) {
    const auto& clips = player.animations;
    if (clips.count(runtime::kAnimationPickupBig) && clips.count(runtime::kAnimationPutdownBig))
        carry.setTiming(false, runtime::carryTiming(clips.at(runtime::kAnimationPickupBig), clips.at(runtime::kAnimationPutdownBig)));
    if (clips.count(runtime::kAnimationPickupSmall) && clips.count(runtime::kAnimationPutdownSmall))
        carry.setTiming(true, runtime::carryTiming(clips.at(runtime::kAnimationPickupSmall), clips.at(runtime::kAnimationPutdownSmall)));
}

std::optional<render::Mat4> HeldObjects::world(assets::FuseArchive& archive, const assets::Placement& placed, bool small, const game_player::PlayerRig& player) {
    const std::optional<render::Mat4> joint = player.jointWorld(small ? runtime::kCarryHandSmall : runtime::kCarryHandBig);
    if (!joint) return std::nullopt;
    const std::string path = assets::meshModelPath(placed.mesh);
    auto found = offsets_.find(path);
    if (found == offsets_.end()) {
        const assets::FuseEntry* entry = archive.find(path);
        found = offsets_.emplace(path, entry ? runtime::carryHandOffset(assets::parseModel(archive.read(*entry))) : render::identity()).first;
    }
    return render::multiply(found->second, *joint);
}

std::int16_t headingTowards(const game_player::PlayerRig& player, const float target[3]) {
    const float angle = std::atan2(target[0] - player.motion.x, target[2] - player.motion.z);
    return runtime::wrapHeading(std::int32_t(std::lround(angle * 10430.378f)));
}

}
