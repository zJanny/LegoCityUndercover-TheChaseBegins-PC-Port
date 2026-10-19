#include "game_level.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::loadDestructibleProps(chase::assets::FuseArchive& archive) {
    wobblyProps.load(archive, level, classOf);
    bouncers.load(level, classOf);
    std::cout << "wobbly props: " << wobblyProps.size() << ", bouncers: " << bouncers.size() << '\n';
}

void LevelSession::playPropSound(std::uint16_t id, const std::array<float, 3>& position) {
    if (id == 0) return;
    const runtime::SoundCall call{id, position, 0};
    soundLog.emplace_back(updates, call.id);
    if (shared.audio) shared.audio->play(call);
}

bool LevelSession::deliverPropHit(const std::string& lowerInstance, const runtime::WobbleHitMessage& message) {
    const std::optional<runtime::WobblyReaction> reaction = wobblyProps.deliver(lowerInstance, message);
    if (!reaction) return false;
    const runtime::WobbleHitResult& hit = reaction->hit;
    if (hit.dropped) return true;
    levelScripts.fireObjectEvent("objecthit", lowerInstance);
    playPropSound(reaction->sound, reaction->soundPosition);
    if (hit.wobble) {
        if (effects.debris()) effects.debris()->burst(*reaction->wobbleBricks, debrisLineStandIn(), debrisLineStandIn());
        std::cout << "prop " << lowerInstance << ": hit, health " << hit.health << " at update " << updatesForLog << '\n';
        return true;
    }
    levelScripts.destroyEntity(lowerInstance);
    std::cout << "prop " << lowerInstance << ": destroyed at update " << updatesForLog << '\n';
    return true;
}

void LevelSession::deliverAttackHits() {
    for (const auto& [instance, message] : attackMessages) {
        if (message.id != runtime::kAttackMessageHit) continue;
        runtime::WobbleHitMessage hit;
        hit.damage = message.damage;
        deliverPropHit(instance, hit);
    }
}

void LevelSession::bounceOnLanding(runtime::Posture before, float fallSpeedBefore) {
    if (!player || bouncers.size() == 0) return;
    runtime::VerticalState& vertical = player->vertical;
    const bool landed = (before == runtime::Posture::Falling || before == runtime::Posture::Jumping) &&
                        (vertical.posture == runtime::Posture::Ground || vertical.posture == runtime::Posture::Landing);
    if (!landed || !vertical.groundRecord) return;
    const std::string& owner = collision.ownerOf(vertical.groundRecord);
    const runtime::BouncerParams* bouncer = owner.empty() ? nullptr : bouncers.find(owner);
    if (!bouncer) {
        bounceMemory = runtime::BounceMemory{};
        return;
    }
    const runtime::BounceLaunch launch = runtime::bounceLaunch(*bouncer, bounceMemory, fallSpeedBefore, pad.slot(kSlotJumpButton).value > 0.0f);
    runtime::startBounceFlight(vertical, launch);
    playPropSound(bouncer->sound, player->position());
    std::cout << "bounce: " << owner << " launches at " << launch.speed << " at update " << updatesForLog << '\n';
}

}
