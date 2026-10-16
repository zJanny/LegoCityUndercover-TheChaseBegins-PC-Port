#include "runtime/vehicle_damage.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {

Vector3 velocityAtArm(const VehicleRigidBody& body, const Vector3& arm) {
    const Vector3& v = body.velocity;
    const Vector3& w = body.angularVelocity;
    return {v[0] + (w[1] * arm[2] - w[2] * arm[1]), v[1] + (w[2] * arm[0] - w[0] * arm[2]), v[2] + (w[0] * arm[1] - w[1] * arm[0])};
}

float dot3(const Vector3& a, const Vector3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

}

float manifoldImpactSpeed(const VehicleRigidBody& self, bool selfIsFirst, const ManifoldPoint& point, const VehicleRigidBody* other) {
    const Vector3& selfArm = selfIsFirst ? point.localA : point.localB;
    const Vector3& otherArm = selfIsFirst ? point.localB : point.localA;
    Vector3 relative = velocityAtArm(self, selfArm);
    if (other) {
        const Vector3 otherVelocity = velocityAtArm(*other, otherArm);
        for (int i = 0; i < 3; ++i) relative[std::size_t(i)] -= otherVelocity[std::size_t(i)];
    }
    Vector3 normal = point.normalOnB;
    if (selfIsFirst)
        for (float& component : normal) component = -component;
    return dot3(normal, relative);
}

float floorImpactSpeed(const VehicleRigidBody& self, const FloorContact& contact) {
    const Vector3 arm = {contact.point[0] - self.position[0], contact.point[1] - self.position[1], contact.point[2] - self.position[2]};
    const Vector3 relative = velocityAtArm(self, arm);
    return dot3({-contact.normal[0], -contact.normal[1], -contact.normal[2]}, relative);
}

namespace {

float remapSpeed(float low, float high, float value, float from, float to) {
    float clamped = high;
    if (value < high) clamped = value <= low ? low : value;
    return from + (to - from) * ((clamped - low) / (high - low));
}

}

ContactReply vehicleContactReply(VehicleRecord& record, float speed, const ContactOther& other) {
    ContactReply reply;
    if (record.debrisCooldown == 0.0f && speed > kVehicleBrickBurstSpeed) {
        const float capped = std::min(speed, kVehicleBrickSpeedCap);
        reply.brickBurst = true;
        reply.brickCount = int(std::floor(remapSpeed(kVehicleBrickBurstSpeed, kVehicleBrickSpeedCap, capped, 1.0f, 3.0f)));
        record.debrisCooldown = kVehicleDebrisCooldown;
    }
    const bool otherAlive = !other.entity || !other.destroyed;
    const bool otherSolid = !other.entity || !other.hasBody || !other.noDamageBody;
    if (otherAlive && speed > kVehicleDamageSpeed && otherSolid) {
        const float capped = speed < kVehicleDamageSpeedCap ? speed : kVehicleDamageSpeedCap;
        const float amount = (capped - kVehicleDamageSpeed) * kVehicleDamageScale * kVehicleDamageFactor;
        const float whole = std::floor(amount);
        DamageMessage message;
        message.hearts = std::uint32_t(whole);
        message.fraction = amount - whole;
        message.kind = kVehicleDamageKindContact;
        reply.damage = message;
    }
    reply.impactSound = speed > 3.0f;
    return reply;
}

DamageOutcome vehicleDamage(VehicleRecord& record, const DamageMessage& message, bool playerVehicle) {
    DamageOutcome outcome;
    bool allowed = true;
    if (message.type != kVehicleDamageTypeScripted && (message.blocked != 0 || record.hitCooldown > 0.0f || record.invulnerable())) allowed = false;
    if (record.destroyed && (record.flags311 & kVehicleFlagRevivable) != 0 && record.hitCooldown == 0.0f) record.destroyed = false;
    if (!allowed || record.destroyed) return outcome;
    outcome.applied = true;
    std::uint32_t lost = message.hearts;
    if (playerVehicle && message.kind == kVehicleDamageKindContact) {
        const std::uint8_t points = std::uint8_t(std::int32_t(std::floor(message.fraction * 100.0f)));
        if (points < record.healthPoints) record.healthPoints = std::uint8_t(record.healthPoints - points);
        else ++lost;
    }
    if (lost == 0) return outcome;
    if (message.type != kVehicleDamageTypeScripted) lost = 1;
    record.hitCooldown = kVehicleHitCooldown;
    outcome.heartLost = true;
    if (lost < record.hearts) {
        record.hearts = std::uint8_t(record.hearts - lost);
        record.healthPoints = kVehicleFullHealthPoints;
        outcome.smokeEffect = record.hearts == 1 ? kVehicleSmokeLastHeart : kVehicleSmokeDamaged;
    } else {
        record.flags311 = message.type == kVehicleDamageTypeScripted ? std::uint8_t(record.flags311 | kVehicleFlagRevivable)
                                                                      : std::uint8_t(record.flags311 & ~kVehicleFlagRevivable);
        record.hearts = 0;
        record.destroyed = true;
        outcome.destroyed = true;
    }
    return outcome;
}

void tickVehicleDamage(VehicleRecord& record, float dt) {
    record.hitCooldown = std::max(record.hitCooldown - dt, 0.0f);
    if (record.debrisCooldown > 0.0f) record.debrisCooldown = std::max(record.debrisCooldown - dt, 0.0f);
}

}
