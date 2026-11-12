#include "runtime/traffic_car_push.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

template <typename Function>
const Function& need(const Function& function, const char* name) {
    if (!function) throw std::logic_error(std::string("traffic push: missing hook ") + name);
    return function;
}

float dot(const AiVec3& a, const AiVec3& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

AiVec3 transformPoint(const AiVec3& p, const AiMatrix& m) {
    AiVec3 out{};
    out[0] = (p[1] * m[1][0] + p[2] * m[2][0]) + (m[3][0] + p[0] * m[0][0]);
    out[1] = (p[1] * m[1][1] + p[2] * m[2][1]) + (m[3][1] + p[0] * m[0][1]);
    out[2] = (p[1] * m[1][2] + p[2] * m[2][2]) + (m[3][2] + p[0] * m[0][2]);
    return out;
}

}

TrafficPushHook makeTrafficCarPush(const TrafficPushCar& car, const TrafficPushWorld& world) {
    return [&car, &world](AiVec3& move, std::size_t entryIndex, std::vector<TrafficContactEntry>& entries, bool slide) {
        const TrafficContactEntry& entry = entries.at(entryIndex);
        const std::uint32_t owner = entry.owner;
        if (need(world.ignorePair, "ignorePair")(car.entity, owner)) return false;
        if ((car.flags & kEntityFlagTakesPush) == 0) return false;
        if ((need(world.flags, "flags")(owner) & kEntityFlagTakesPush) == 0) return false;

        AiVec3 vector = {0.0f, 0.0f, 0.0f};
        if (!need(world.pushVector, "pushVector")(car.entity, move, entry, &vector, slide)) return false;
        need(world.message45, "message45")(owner, car.entity, &vector);
        if (!slide) return true;

        if (need(world.bit3, "bit3")(owner) && need(world.isCharacter, "isCharacter")(car.entity)) {
            need(world.squash, "squash")(car.entity, owner);
            return false;
        }

        for (const TrafficContactEntry& other : entries) {
            const std::uint32_t o = other.owner;
            if (o == car.entity || o == owner) continue;
            if ((world.flags(o) & kEntityFlagBlocker) == 0) continue;
            if ((other.word & kTrafficContactSkipBit) != 0) continue;
            if (world.ignorePair(car.entity, o)) continue;
            if (world.pushVector(car.entity, move, other, nullptr, slide)) break;
        }

        if ((car.flags & kEntityFlagReturnsPush) != 0 || (world.flags(owner) & kEntityFlagBlocker) != 0) {
            world.message45(car.entity, owner, nullptr);
            return true;
        }

        for (const TrafficContactEntry& other : entries) {
            const std::uint32_t o = other.owner;
            if ((world.flags(o) & kEntityFlagTakesPush) == 0) continue;
            if (world.ignorePair(owner, o)) continue;
            AiVec3 second = {0.0f, 0.0f, 0.0f};
            if (!world.pushVector(owner, vector, other, &second, true)) continue;
            if ((world.flags(o) & kEntityFlagBlocker) == 0) world.moveNode(o, second);
            break;
        }
        world.moveNode(owner, vector);
        return false;
    };
}

CharacterPushResult receiveCarPush(const std::optional<CharacterPushSender>& sender, const AiVec3& characterPosition, bool characterIsPlayer) {
    CharacterPushResult result;
    if (!sender || sender->entity == 0) return result;
    if (sender->character) {
        result.senderEvent18 = sender->entity;
        return result;
    }
    if (sender->kind != kEntityKindVehicleA && sender->kind != kEntityKindVehicle && sender->kind != kEntityKindVehicleB) return result;

    AiVec3 front = transformPoint(sender->localPoint, sender->node);
    const float reach = sender->halfLength * kCarPushFrontScale;
    for (int axis = 0; axis < 3; ++axis) front[axis] += sender->node[2][axis] * reach;
    const AiVec3 offset = {characterPosition[0] - front[0], characterPosition[1] - front[1], characterPosition[2] - front[2]};
    const float frontDot = dot(offset, sender->node[2]);
    const float limit = std::cos(kCarPushFrontAngle);
    if (!(sender->forwardSpeed >= kCarPushMinForwardSpeed && frontDot >= limit)) return result;

    CharacterHitMessage hit;
    hit.move = std::nullopt;
    hit.sender.present = true;
    hit.sender.player = false;
    hit.sender.character = false;
    hit.sender.kind = sender->kind;
    hit.damage = characterIsPlayer ? 1 : 0;
    hit.force = sender->forwardSpeed;
    hit.kind = kCarPushHitKind;
    hit.weapon = kCarPushHitWeapon;
    result.hit = hit;
    return result;
}

bool copWhistleRequested(const CopWhistleGate& gate) {
    return gate.extraActive && gate.state == kCopWhistleRequestState && gate.downPressed && gate.word13c == 0 &&
           gate.characterClass == kCopWhistleCharacterClass;
}

CopWhistleEnter enterCopWhistle(TrafficWhistleStop& stop, std::uint32_t clock, GameRandom& random) {
    startTrafficWhistleStop(stop, clock, random);
    return CopWhistleEnter{};
}

std::optional<std::uint16_t> updateCopWhistle(bool clipFinished) {
    if (clipFinished) return kCopWhistleRequestState;
    return std::nullopt;
}

std::uint32_t gameClockAfter(std::uint32_t updates) { return updates * kGameClockUnitsPerUpdateStandIn; }

}
