#include "game_level.hpp"

#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {
runtime::PlatformVector forwardOf(std::int16_t facing) {
    const float heading = runtime::headingRadians(facing);
    return {std::sin(heading), 0.0f, std::cos(heading)};
}

std::optional<runtime::PlatformFrame> frameOf(const runtime::PlatformSet& platforms, const std::string& name) {
    const auto frame = platforms.nodeFrame(name);
    if (!frame) return std::nullopt;
    return runtime::PlatformFrame{frame->first, frame->second};
}

bool oriented(const runtime::PlatformSet& platforms, const std::string& name) {
    const runtime::PlatformRecord* record = platforms.find(name);
    return record && record->motion.orient != 0;
}
}

// STAND-IN SCRIPT-S310: only the player has a ground entity, and only platforms (kind 64) become one.
void LevelSession::rideGroundEntity() {
    if (!player) return;
    runtime::VerticalState& vertical = player->vertical;
    const runtime::PlatformSet& platforms = levelScripts.platforms();
    const std::optional<runtime::PlatformFrame> frame = frameOf(platforms, playerGround.owner);
    if (vertical.posture == runtime::Posture::Jumping || !frame) return;
    if (playerGround.oriented) {
        const runtime::GroundEntityTurn ride = runtime::groundEntityRideOriented(playerGround, *frame, player->position(), forwardOf(player->motion.facing));
        std::copy(ride.move.begin(), ride.move.end(), vertical.groundEntityMove);
        player->motion.facing = std::int16_t(player->motion.facing + ride.turn);
        player->motion.lastIntent = std::int16_t(player->motion.lastIntent + ride.turn);
        return;
    }
    const runtime::PlatformVector move = runtime::groundEntityRide(playerGround, frame->translation);
    std::copy(move.begin(), move.end(), vertical.groundEntityMove);
}

void LevelSession::linkGroundEntity() {
    if (!player) return;
    runtime::VerticalState& vertical = player->vertical;
    std::fill(vertical.groundEntityMove, vertical.groundEntityMove + 3, 0.0f);
    const std::string before = playerGround.owner;
    const runtime::PlatformSet& platforms = levelScripts.platforms();
    if (vertical.posture == runtime::Posture::Jumping || vertical.posture == runtime::Posture::Falling || vertical.posture == runtime::Posture::Sliding) {
        playerGround = {};
    } else if (vertical.groundKept) {
        if (const std::optional<runtime::PlatformVector> now = platforms.nodeTranslation(playerGround.owner)) playerGround.remembered = *now;
    } else if (vertical.groundProbed) {
        const std::string owner = vertical.groundFound ? collision.ownerOf(vertical.groundRecord) : std::string();
        const std::optional<runtime::PlatformFrame> frame = frameOf(platforms, owner);
        if (!frame) playerGround = {};
        else if (oriented(platforms, owner)) runtime::groundEntityContactOriented(playerGround, owner, *frame, player->position(), forwardOf(player->motion.facing));
        else runtime::groundEntityContact(playerGround, owner, frame->translation);
    }
    if (playerGround.owner == before) return;
    if (playerGround.owner.empty()) std::cout << "player: left ground entity " << before << " at update " << updates << '\n';
    else std::cout << "player: ground entity " << playerGround.owner << " at update " << updates << '\n';
}

}
