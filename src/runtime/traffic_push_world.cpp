#include "runtime/traffic_push_world.hpp"

#include "runtime/vehicle_prop_smash.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

template <typename Function>
const Function& need(const Function& function, const char* name) {
    if (!function) throw std::logic_error(std::string("traffic push world: missing hook ") + name);
    return function;
}

AiVec3 transformPoint(const AiVec3& p, const AiMatrix& m) {
    AiVec3 out{};
    for (int axis = 0; axis < 3; ++axis) {
        const float rest = p[1] * m[1][axis] + p[2] * m[2][axis];
        const float first = m[3][axis] + p[0] * m[0][axis];
        out[axis] = rest + first;
    }
    return out;
}

AiVec3 intoFrame(const AiVec3& p, const AiMatrix& m) {
    const float dx = p[0] - m[3][0];
    const float dy = p[1] - m[3][1];
    const float dz = p[2] - m[3][2];
    AiVec3 out{};
    for (int row = 0; row < 3; ++row) {
        float sum = dx * m[row][0];
        sum += dy * m[row][1];
        sum += dz * m[row][2];
        out[row] = sum;
    }
    return out;
}

assets::Matrix toAssetsMatrix(const AiMatrix& m) {
    assets::Matrix out{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 3; ++col) out[row * 4 + col] = m[row][col];
    out[15] = 1.0f;
    return out;
}

bool belowBits(float value, std::uint32_t limitBits) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits <= static_cast<std::int32_t>(limitBits);
}

void normalise2(float& x, float& z) {
    float sum = x * x;
    sum += z * z;
    const float length = std::sqrt(sum);
    if (belowBits(length, 0x34000000u)) {
        x = 0.0f;
        z = 1.0f;
        return;
    }
    const float inverse = 1.0f / length;
    x *= inverse;
    z *= inverse;
}

void normalise3(AiVec3& v) {
    float sum = v[0] * v[0];
    sum += v[1] * v[1];
    sum += v[2] * v[2];
    const float length = std::sqrt(sum);
    if (belowBits(length, 0x34000000u)) {
        v = {0.0f, 0.0f, 1.0f};
        return;
    }
    const float inverse = 1.0f / length;
    v = {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

bool insideOpen(float value, float half) { return value > -half && value < half; }

}

bool trafficPushVector(const TrafficPushShapes& shapes, std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry, AiVec3* vector,
                       bool slide) {
    const std::uint32_t owner = entry.owner;
    const AiMatrix moverMatrix = need(shapes.nodeMatrix, "nodeMatrix")(mover);
    AiMatrix matrix = need(shapes.entryMatrix, "entryMatrix")(entry);
    if (vector) *vector = {0.0f, 0.0f, 0.0f};
    const std::uint16_t moverFlags = need(shapes.flags, "flags")(mover);
    const std::uint16_t ownerFlags = shapes.flags(owner);
    if ((moverFlags & kEntityFlagBlocker) != 0 && (ownerFlags & kEntityFlagBlocker) != 0) slide = false;

    if (need(shapes.meshEntry, "meshEntry")(entry)) {
        if (!need(shapes.meshPush, "meshPush")(mover, move, entry)) return false;
        need(shapes.lastHit, "lastHit")(owner);
        return true;
    }

    const std::uint8_t ownerKind = need(shapes.kind, "kind")(owner);
    if (std::find(shapes.excludedKinds.begin(), shapes.excludedKinds.end(), ownerKind) != shapes.excludedKinds.end()) return false;

    const TrafficPushBox moverBox = need(shapes.box, "box")(mover);
    const AiVec3 moverCentre = transformPoint(moverBox.centre, moverMatrix);
    const AiVec3 movedCentre = {moverCentre[0] + move[0], moverCentre[1] + move[1], moverCentre[2] + move[2]};

    const bool oriented = (moverFlags & kEntityFlagReturnsPush) != 0 || (ownerFlags & kEntityFlagOrientedPush) != 0;
    TrafficPushBox box = shapes.box(owner);
    if (!oriented && std::fabs(matrix[1][1]) < kPushUprightLimit) {
        const SmashBox world = worldSmashBox(toAssetsMatrix(matrix), SmashBox{{box.centre[0], box.centre[1], box.centre[2]}, {box.half[0], box.half[1], box.half[2]}});
        box.centre = {world.centre[0] - matrix[3][0], world.centre[1] - matrix[3][1], world.centre[2] - matrix[3][2]};
        box.half = {world.half[0], world.half[1], world.half[2]};
        matrix[0] = {1.0f, 0.0f, 0.0f};
        matrix[1] = {0.0f, 1.0f, 0.0f};
        matrix[2] = {0.0f, 0.0f, 1.0f};
    }
    if (oriented) {
        if (!need(shapes.orientedPush, "orientedPush")(mover, move, entry, matrix, box, moverCentre, movedCentre, slide)) return false;
        need(shapes.lastHit, "lastHit")(owner);
        return true;
    }

    AiVec3 local = intoFrame(movedCentre, matrix);
    for (int axis = 0; axis < 3; ++axis) local[axis] -= box.centre[axis];
    for (int axis = 0; axis < 3; ++axis)
        if (!insideOpen(local[axis], box.half[axis] + moverBox.half[axis])) return false;
    if (!slide) return true;
    if (!vector) throw std::logic_error("traffic push world: 0x1f49d8 writes the push of a box entry with slide through a null vector");

    float lengthSquared = move[0] * move[0];
    lengthSquared += move[1] * move[1];
    lengthSquared += move[2] * move[2];
    const float length = std::sqrt(lengthSquared);
    float dx = moverMatrix[3][0] - matrix[3][0];
    float dz = moverMatrix[3][2] - matrix[3][2];
    normalise2(dx, dz);
    dx *= length;
    dz *= length;
    const float shareX = dx * kPushShare;
    const float shareZ = dz * kPushShare;
    AiVec3 back = {shareX, 0.0f, shareZ};
    AiVec3 pushed = {-shareX, -0.0f, -shareZ};
    const AiVec3 before = pushed;
    need(shapes.slideMove, "slideMove")(owner, pushed, kPushSlidePushedFlags, 0.0f);
    for (int axis = 0; axis < 3; ++axis) back[axis] += pushed[axis] - before[axis];
    shapes.slideMove(mover, back, kPushSlideMoverFlags, shapes.floorNormalMin);
    need(shapes.moveNode, "moveNode")(mover, {back[0], 0.0f, back[2]});
    *vector = pushed;
    return true;
}

std::function<bool(std::uint32_t, const AiVec3&, const TrafficContactEntry&, AiVec3*, bool)> makeTrafficPushVector(const TrafficPushShapes& shapes) {
    return [&shapes](std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry, AiVec3* vector, bool slide) {
        return trafficPushVector(shapes, mover, move, entry, vector, slide);
    };
}

CharacterHitMessage trafficSquashHit(const CharacterHitSender& sender, std::uint8_t weapon) {
    CharacterHitMessage hit;
    hit.move = std::nullopt;
    hit.sender = sender;
    hit.damage = kSquashDamage;
    hit.force = 0.0f;
    hit.kind = 0;
    hit.weapon = weapon;
    return hit;
}

CollisionEventOutcome superKnockbackEvent(std::uint32_t type, std::optional<std::uint32_t> other) {
    CollisionEventOutcome outcome;
    outcome.destroysSelf = true;
    if ((type != kCharacterEventCollider && type != kCharacterEventCollidee) || !other || *other == 0) return outcome;
    CharacterHitMessage hit;
    hit.move = std::nullopt;
    hit.sender = CharacterHitSender{};
    hit.damage = kSuperKnockbackDamage;
    hit.force = 0.0f;
    hit.kind = 0;
    hit.weapon = 0;
    outcome.messageTarget = *other;
    outcome.hit = hit;
    return outcome;
}

CollisionEventOutcome throwFinisherEvent(const CharacterHitSender& selfSender, std::uint32_t self, const AiVec3& selfPosition,
                                         const std::optional<ThrowFinisherOther>& other) {
    CollisionEventOutcome outcome;
    if (!other || other->entity == 0) return outcome;
    CharacterHitMessage hit;
    hit.move = std::nullopt;
    hit.sender = selfSender;
    hit.damage = kThrowFinisherDamage;
    hit.force = 0.0f;
    hit.kind = kThrowFinisherHitKind;
    hit.weapon = 0;

    if (other->character && other->takesCombat) {
        if (other->target140 == self || other->ability52 || !other->reachable) return outcome;
        AiVec3 toSelf = {selfPosition[0] - other->position[0], selfPosition[1] - other->position[1], selfPosition[2] - other->position[2]};
        normalise3(toSelf);
        const float angle = std::atan2(toSelf[0], toSelf[2]);
        outcome.combatHeading = static_cast<std::uint16_t>(static_cast<std::int32_t>(angle * kHeadingUnits));
        outcome.combatTarget = other->entity;
        outcome.hit = hit;
        return outcome;
    }
    const bool takesByController = other->controlled && !other->ctrlBitDE0 && (other->ctrlCC == 1 || other->ctrlCC == 7 || other->ctrlCC == 11);
    const bool takesByKind = other->kind == kThrowFinisherKindA || other->kind == kThrowFinisherKindB || other->kind == kThrowFinisherKindC;
    if (!takesByController && !takesByKind) return outcome;
    outcome.messageTarget = other->entity;
    outcome.hit = hit;
    return outcome;
}

CollisionEventOutcome characterCollisionEvent(std::uint16_t state, std::uint32_t type, const CharacterHitSender& selfSender, std::uint32_t self,
                                              const AiVec3& selfPosition, const std::optional<ThrowFinisherOther>& other) {
    if (type != kCharacterEventCollider && type != kCharacterEventCollidee) return {};
    if (state == kCharacterStateSuperKnockback) {
        std::optional<std::uint32_t> entity;
        if (other) entity = other->entity;
        return superKnockbackEvent(type, entity);
    }
    if (state == kCharacterStateThrowFinisher) return throwFinisherEvent(selfSender, self, selfPosition, other);
    return {};
}

CarPushDelivery deliverCarPush(const CarPushCharacter& character, const std::optional<CharacterPushSender>& sender,
                               const std::optional<ThrowFinisherOther>& senderAsOther, std::uint16_t senderState, const CharacterHitSender& senderAsSender,
                               const AiVec3& senderPosition, const std::optional<ThrowFinisherOther>& asOther) {
    CarPushDelivery delivery;
    std::optional<ThrowFinisherOther> pusher;
    if (sender && sender->entity != 0) pusher = senderAsOther;
    delivery.event17 = characterCollisionEvent(character.state, kCharacterEventCollider, character.asSender, character.entity, character.position, pusher);
    delivery.push = receiveCarPush(sender, character.position, character.player);
    if (delivery.push.senderEvent18)
        delivery.event18 = characterCollisionEvent(senderState, kCharacterEventCollidee, senderAsSender, *delivery.push.senderEvent18, senderPosition,
                                                 asOther);
    return delivery;
}

std::optional<CharacterHitResult> applyCarPushToCharacter(CharacterHitTarget& target, const CarPushDelivery& delivery) {
    if (delivery.event17.destroysSelf) target.destroyed = true;
    if (!delivery.push.hit) return std::nullopt;
    return receiveHit(target, *delivery.push.hit);
}

}
