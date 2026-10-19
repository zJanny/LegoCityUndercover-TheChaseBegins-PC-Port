#include "game_level.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
unsigned clipUpdates(const game_player::PlayerRig& player, int role) {
    const assets::Animation& clip = player.animations.at(role);
    return unsigned(std::ceil(float(assets::animationLength(clip)) * 30.0f / assets::animationFramesPerSecond(clip) - 1e-3f));
}

bool hasZiplineClips(const game_player::PlayerRig& player) {
    return player.animations.count(runtime::kAnimationZiplineGrab) && player.animations.count(runtime::kAnimationZiplineSlide) &&
           player.animations.count(runtime::kAnimationZiplineDismount);
}
}

// The player step of one update: the zipline states while riding, else the locomotion and vertical step, with the use press (A, 0x2011b0 -> 0x1fef10 with pressed 1) on a zipline
// anchor before it and the grab test of the jump update after it (docs/re/player-traversal.md).
game_player::PlayerAnimation LevelSession::movePlayer(const runtime::StickIntent& intent, bool jumpPressed, float dt) {
    if (ziplines.empty() || !hasZiplineClips(*player)) return game_player::updatePlayer(*player, collision, intent, jumpPressed, dt);
    if (!ziplineDismount) ziplineDismount = assets::rootMotion(player->animations.at(runtime::kAnimationZiplineDismount));
    runtime::ZiplineInput input;
    input.dt = dt;
    input.grabUpdates = clipUpdates(*player, runtime::kAnimationZiplineGrab);
    input.dismountUpdates = clipUpdates(*player, runtime::kAnimationZiplineDismount);
    input.dismountMotion = ziplineDismount ? &*ziplineDismount : nullptr;
    input.dismountFramesPerSecond = assets::animationFramesPerSecond(player->animations.at(runtime::kAnimationZiplineDismount));
    input.enabled = [this](const runtime::ZiplineAnchor& anchor) { return !objectState.isDisabled(anchor.instance); };
    const auto readPlayer = [&]() {
        input.position[0] = player->motion.x;
        input.position[1] = player->vertical.y;
        input.position[2] = player->motion.z;
        input.facing = player->motion.facing;
    };
    const auto apply = [&](const runtime::ZiplineStep& step) -> game_player::PlayerAnimation {
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
        player->motion.facing = player->motion.lastIntent = step.facing;
        player->vertical.posture = runtime::Posture::Falling;
        player->vertical.jump = nullptr;
        player->vertical.fallSpeed = step.fallSpeed;
        player->vertical.fallDistance = step.fallDistance;
        for (const runtime::ZiplineSound& sound : step.sounds) {
            if (sound.action == runtime::ZiplineSoundAction::Play) player->sounds.push_back({sound.id, player->position(), 0});
            else if (shared.audio) shared.audio->stop(sound.id, 0);
        }
        if (step.falling) {
            std::cout << "zipline: fall at update " << updates << '\n';
            return {runtime::kAnimationFall, 1.0f};
        }
        return {step.animation, 1.0f};
    };
    readPlayer();
    if (ziplineRider.active()) {
        input.jumpPressed = jumpPressed;
        return apply(runtime::ziplineUpdate(ziplineRider, ziplines, input));
    }
    if (pole.usePressed && player->vertical.posture == runtime::Posture::Ground) {
        const int used = runtime::ziplineUseTest(ziplines, input.position, input.facing, input.enabled);
        if (used >= 0) {
            std::cout << "zipline: use " << ziplines[std::size_t(used)].instance << " at update " << updates << '\n';
            return apply(runtime::ziplineEnter(ziplineRider, ziplines, used, input));
        }
    }
    const runtime::Posture before = player->vertical.posture;
    const game_player::PlayerAnimation shown = game_player::updatePlayer(*player, collision, intent, jumpPressed, dt);
    const bool started = before != runtime::Posture::Jumping && player->vertical.posture == runtime::Posture::Jumping;
    if (!runtime::ziplineJumpTick(ziplineRider, started, before == runtime::Posture::Jumping)) return shown;
    readPlayer();
    const float heading = runtime::headingRadians(input.facing);
    const float point[3] = {input.position[0] + runtime::kZiplineGrabForwardOffset * std::sin(heading), input.position[1],
                            input.position[2] + runtime::kZiplineGrabForwardOffset * std::cos(heading)};
    const int grabbed = runtime::ziplineGrabTest(ziplines, point, input.enabled);
    if (grabbed < 0) return shown;
    std::cout << "zipline: grab " << ziplines[std::size_t(grabbed)].instance << " at update " << updates << '\n';
    return apply(runtime::ziplineEnter(ziplineRider, ziplines, grabbed, input));
}

void LevelSession::loadZiplineRopes(assets::FuseArchive& archive) {
    ziplineRopes = runtime::loadZiplineRopes(level, classOf, ziplines);
    ropeRenderer.emplace(archive);
}

void LevelSession::drawZiplineRopes(const render::Mat4& view, const render::Mat4& viewProjection) {
    if (!ropeRenderer || ziplines.empty()) return;
    const runtime::ParticleCamera camera = game_particles::cameraOf(view);
    const float position[3] = {camera.position[0], camera.position[1], camera.position[2]};
    const float forward[3] = {camera.forward[0], camera.forward[1], camera.forward[2]};
    const auto enabled = [this](const runtime::ZiplineAnchor& anchor) { return !objectState.isDisabled(anchor.instance); };
    ropeRenderer->draw(viewProjection, runtime::ziplineRopeDraws(ziplines, ziplineRopes, position, forward, enabled));
}

}
