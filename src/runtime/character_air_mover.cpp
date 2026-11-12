#include "runtime/character_air_mover.hpp"

#include "runtime/player_vertical.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
// STAND-IN SCRIPT-S346: 0x21df6c with ctrl+0x9c = 0 and without the type scaling, as for the player.
constexpr float kCharacterGravity = kGravity;

void fall(CharacterAir& air, AirVector& position, bool gravity, float dt, CharacterMoveResult& result) {
    result.fallEvent = true;
    // STAND-IN SCRIPT-S346: the long ground query 0x203b34 of a failed probe is not ported, ctrl+0x244 holds no ground.
    air.groundBelow.reset();
    if (gravity) {
        result.fallStateRequest = true;
        air.fallSpeed = std::min(kTerminalFallSpeed, air.fallSpeed + kCharacterGravity * kGravityRate * dt);
    }
    position[1] -= air.fallSpeed;
    air.fallDistance += air.fallSpeed;
}
}

CharacterMoveResult moveCharacter(CharacterAir& air, AirVector& position, float forwardX, float forwardZ, const CharacterMove& move,
                                  const CollisionWorld& world, float dt) {
    CharacterMoveResult result;
    const AirVector start = position;
    AirVector delta = move.vector;
    if (move.gravity) delta[1] -= air.fallSpeed;
    for (std::size_t a = 0; a < 3; ++a) delta[a] += move.root[a];
    float dx = delta[0], dz = delta[2];
    // STAND-IN SCRIPT-S346: the character's box is the player's (entity +0x60 / +0x6c of the character are not read).
    if ((dx != 0 || dz != 0) && !world.empty())
        slideAlongWalls(world, position[0], position[1], position[2], forwardX, forwardZ, dx, dz, PlayerBox(), kPlayerExcludedSurfaces, delta[1]);
    position[0] += dx;
    position[2] += dz;
    air.fallSpeed = -delta[1];
    if (delta[1] > 0) {
        // STAND-IN SCRIPT-S346: no ceiling test (0x203c10), and the ground query after the rise (0x203b34 / 0x289b80) is not ported.
        position[1] += delta[1];
        air.onGround = false;
        air.groundBelow.reset();
        if (move.gravity) air.fallSpeed += kCharacterGravity;
    } else if (!(delta[1] == 0 && air.onGround && dx == 0 && delta[2] == 0)) {
        const bool wasInAir = !air.onGround;
        result.probed = true;
        air.onGround = false;
        GroundProbe probe;
        probe.x = position[0];
        probe.y = position[1];
        probe.z = position[2];
        probe.forwardX = forwardX;
        probe.forwardZ = forwardZ;
        probe.rootDelta = std::hypot(dx, delta[2]);
        probe.dt = dt;
        probe.minimumDistance = std::fabs(air.fallSpeed);
        const std::optional<GroundContact> ground = world.empty() ? std::nullopt : groundContact(world, probe);
        if (ground) {
            position[1] = ground->height;
            air.onGround = true;
            air.groundBelow = ground->height;
            air.fallSpeed = 0;
            air.fallDistance = 0;
            result.landed = wasInAir;
        } else {
            fall(air, position, move.gravity, dt, result);
        }
    }
    for (std::size_t a = 0; a < 3; ++a) air.lastMove[a] = position[a] - start[a];
    return result;
}

std::optional<ThrownFall> thrownFlight(CharacterAir& air, AirVector& position, float forwardX, float forwardZ, const CollisionWorld& world, float dt) {
    if (!air.thrown) return std::nullopt;
    ThrownFall result;
    if (air.onGround) {
        air.thrown = false;
        result.step = ThrownFallStep::Landed;
        return result;
    }
    air.velocity[1] -= kThrownFallStep;
    CharacterMove move;
    move.vector = air.velocity;
    result.move = moveCharacter(air, position, forwardX, forwardZ, move, world, dt);
    return result;
}

bool startThrownFall(CharacterAir& air, float nodeY) {
    if (!air.fallAllowed || air.thrown || air.onGround) return false;
    if (air.groundBelow && !(std::fabs(nodeY - *air.groundBelow) > kThrownFallStartDrop)) return false;
    air.velocity = air.lastMove;
    air.thrown = true;
    return true;
}

bool takedownEnterAir(CharacterAir& air, bool gunHitInAir) {
    air.thrown = false;
    bool started = false;
    if (gunHitInAir && !air.onGround) {
        air.velocity = air.lastMove;
        air.thrown = true;
        started = true;
    }
    air.fallAllowed = false;
    return started;
}

void sampleFallAllowed(CharacterAir& air, const assets::Animation& clip, float normalisedTime) {
    for (std::size_t s = 0; s < clip.subAnimations.size() && s < clip.subAspects.size(); ++s) {
        if (clip.subAspects[s] != kFallAllowedAspect) continue;
        for (const assets::AnimTrack& track : clip.subAnimations[s]) {
            if (track.hash != kFallAllowedTrack) continue;
            float value = 0.0f;
            for (const assets::CurveKey& key : track.keys)
                if (key.time <= normalisedTime) value = key.value;
            air.fallAllowed = value == 1.0f;
        }
    }
}

bool fallArrestHitOnLanding(bool marked, std::uint16_t state) {
    if (!marked) return false;
    return !((state >= 227 && state <= 231) || state == 307 || state == 325 || state == 409);
}

TakedownMover takedownMover(std::uint16_t state) {
    if (state == 227) return TakedownMover::Putdown;
    if ((state >= 0x122 && state <= 0x12f) || (state >= 0x133 && state <= 0x136) || (state >= 0x139 && state <= 0x13e)) return TakedownMover::Takedown;
    return TakedownMover::None;
}

}
