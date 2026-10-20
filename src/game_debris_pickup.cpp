#include "game_level.hpp"
#include "runtime/debris_pickup.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

constexpr const char* kMagnetJoint = "chest";

void placeObject(runtime::DebrisObject& object, const runtime::SuperDebrisPlacement& placed) {
    for (unsigned row = 0; row < 3; ++row) {
        float* r = &object.matrix[row * 4];
        const float length = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
        if (length > 0.0f)
            for (unsigned c = 0; c < 3; ++c) r[c] *= placed.scale / length;
    }
    for (unsigned a = 0; a < 3; ++a) object.matrix[12 + a] = placed.position[a];
    object.colour = placed.colour;
    object.alpha = placed.alpha;
}

std::uint32_t tintWord(const runtime::SuperBrickTint& tint) {
    const std::array<std::uint8_t, 3>& bytes = tint.bytes();
    return std::uint32_t(bytes[0]) | std::uint32_t(bytes[1]) << 8 | std::uint32_t(bytes[2]) << 16 | 0xff000000u;
}

}

// STAND-IN VEH-S112: the magnet is on while the player is seated in a vehicle (the state 0x19b of 0x24255c and the extras bit 8 are not modelled),
// the player's entity word +0xc bit 4 counts as clear, the collect sound plays without its volume 0.7, the collect clamp in the camera *0x377544 and
// the camera shift *0x58ef40 of the flight are left out, the scale of 0x23f6d0 scales the object's matrix rows, the tint's alpha byte is 0xff, the
// stud multipliers *0x3772b4 are not applied and the busy test 0x236104 and record +0x45 of 0x18083c / 0x2a2cc0 are not checked.
std::vector<runtime::SoundCall> LevelSession::updateDebrisPickup(float dt, const std::array<float, 3>& pickupPoint) {
    std::vector<runtime::SoundCall> sounds;
    runtime::DebrisField* field = effects.debris();
    if (!field || !player || !follow) return sounds;
    runtime::DebrisPickupInput input;
    input.frameDelta = dt;
    input.pickupPoint = pickupPoint;
    input.magnet = boarding && boarding->seatedVehicle();
    if (const std::optional<chase::render::Mat4> chest = player->jointWorld(kMagnetJoint)) input.magnetTarget = runtime::DebrisVec3{(*chest)[12], (*chest)[13], (*chest)[14]};
    else input.magnetTarget = runtime::DebrisVec3{player->motion.x, player->vertical.y, player->motion.z};
    input.flightTarget = runtime::unprojectTopScreen(runtime::kSuperDebrisSinkX, runtime::kSuperDebrisSinkY, runtime::kSuperDebrisSinkDepth, follow->eye(),
                                                     follow->yaw(), follow->pitch(), follow->verticalFieldOfView());
    input.tint = tintWord(superBricks.tint());
    std::uint32_t count = superBricks.total();
    const runtime::DebrisPickupStep step = runtime::updateDebrisPickup(field->records().data(), unsigned(field->records().size()), input, count);
    superBricks.setTotal(count);
    for (const runtime::SuperDebrisPlacement& placed : step.placements)
        if (runtime::DebrisObject* object = field->objectOf(field->records()[placed.record])) placeObject(*object, placed);
    for (const unsigned record : step.freedRecords) field->freeRecord(record);
    for (const runtime::DebrisPickupSound& sound : step.sounds) sounds.push_back({sound.id, sound.position});
    if (step.studUnits) studs.setCounter(std::min(studs.counter() + step.studUnits, runtime::kStudCounterLimit));
    for (unsigned i = 0; hud && i < step.shown; ++i) hud->superBrickPickedUp();
    for (const runtime::SuperDebrisBanked& banked : step.banked) {
        if (hud) hud->superBrickBanked(banked.total);
        soundLog.emplace_back(updates, banked.sound);
        if (shared.audio) shared.audio->playUi(banked.sound);
        std::cout << "super debris brick banked: value " << int(banked.value) << ", count " << banked.total << " at update " << updates << '\n';
    }
    return sounds;
}

}
