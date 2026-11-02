#include "game_level.hpp"

#include <iostream>

namespace game_level {
using namespace chase;

void LevelSession::loadPushables(assets::FuseArchive& archive) {
    pushables.load(archive, level, classOf);
    pushGrip = runtime::PushGrip{};
    pushableScene.clear();
    for (const runtime::PushableObject& object : pushables.objects()) {
        std::optional<std::size_t> found;
        for (std::size_t i = 0; i < placedObjects.size() && !found; ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == object.instance) found = placedSceneIndex[i];
        pushableScene.push_back(found);
    }
    std::cout << "pushables: " << pushables.objects().size() << '\n';
}

game_player::PlayerAnimation LevelSession::movePlayerWithPush(const runtime::StickIntent& intent, bool jumpPressed, float dt) {
    if (pushables.objects().empty()) return movePlayer(intent, jumpPressed, dt);
    if (pushGrip.active) {
        runtime::PushGripInput input;
        input.position[0] = player->motion.x;
        input.position[1] = player->vertical.y;
        input.position[2] = player->motion.z;
        input.facing = player->motion.facing;
        input.stickActive = intent.active;
        input.stickHeading = intent.heading;
        input.actionHeld = pushActionHeld;
        input.dt = dt;
        const std::size_t pushable = pushGrip.pushable;
        const runtime::PushGripStep step = runtime::pushGripUpdate(pushGrip, pushables, input);
        if (step.exited) {
            std::cout << "pushable " << pushables.objects()[pushable].instance << ": player state 1 at update " << updates << '\n';
            return {0, 1.0f};
        }
        if (step.enteredState) std::cout << "pushable " << pushables.objects()[pushable].instance << ": player state " << *step.enteredState << " at update " << updates << '\n';
        player->motion.x = step.position[0];
        player->motion.z = step.position[2];
        player->motion.facing = player->motion.lastIntent = step.facing;
        return {step.animation, 1.0f};
    }
    const game_player::PlayerAnimation shown = movePlayer(intent, jumpPressed, dt);
    if (player->vertical.posture != runtime::Posture::Ground) return shown;
    const float position[3] = {player->motion.x, player->vertical.y, player->motion.z};
    for (std::size_t i = 0; i < pushables.objects().size(); ++i) {
        const runtime::PushableObject& object = pushables.objects()[i];
        if (objectState.isDisabled(object.instance)) continue;
        const std::optional<std::size_t> handle = runtime::pushGrabTest(object, position, player->motion.facing);
        if (!handle) continue;
        runtime::pushGripStart(pushGrip, i, *handle, position);
        std::cout << "pushable " << object.instance << ": grabbed at " << object.handles[*handle].instance << ", player state 84 at update " << updates << '\n';
        return {runtime::kAnimationPushIdle, 1.0f};
    }
    return shown;
}

void LevelSession::updatePushables(const std::function<void(const runtime::SoundCall&)>& emit) {
    const auto disabled = [this](const std::string& instance) { return objectState.isDisabled(instance); };
    const auto destination = [this](const std::string& instance, float position[3]) {
        const std::optional<assets::Placement> placed = assets::objectStart(level, instance);
        if (!placed) return false;
        std::copy_n(placed->position, 3, position);
        return true;
    };
    for (const runtime::PushableEvent& event : pushables.update(disabled, destination)) {
        const runtime::PushableObject& object = pushables.objects()[event.index];
        const std::uint16_t owner = levelScripts.soundNatives().ownerOf(object.instance);
        const std::array<float, 3> at = {object.node[12], object.node[13], object.node[14]};
        switch (event.kind) {
        case runtime::PushableEventKind::Moved:
            break;
        case runtime::PushableEventKind::SoundStart:
            if (object.soundPush) emit({object.soundPush, at, owner, true});
            break;
        case runtime::PushableEventKind::SoundStop:
            if (object.soundPush && shared.audio) shared.audio->stop(object.soundPush, owner);
            break;
        case runtime::PushableEventKind::Arrived:
            std::cout << "pushable " << object.instance << ": state 1 at update " << updates << '\n';
            pushableDrawnPending.insert(event.index);
            break;
        case runtime::PushableEventKind::Done:
            std::cout << "pushable " << object.instance << ": state 2 at update " << updates << '\n';
            if (!object.triggerObject.empty()) levelScripts.triggerObjectEnabling(object.triggerObject);
            break;
        }
    }
    for (std::size_t i = 0; i < pushables.objects().size(); ++i)
        if (pushableScene[i]) scene.setTransform(*pushableScene[i], pushables.objects()[i].node);
}

}
