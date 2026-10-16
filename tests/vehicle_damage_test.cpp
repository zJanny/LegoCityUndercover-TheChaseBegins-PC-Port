// Test of the vehicle contact damage (src/runtime/vehicle_damage.*, docs/re/vehicles-physics-damage.md). The expectations are worked from the
// disassembly of the vehicle message handler 0x1797c4 (message 0x80000002 at 0x179ca4, message 0 at 0x1799b4), the closing speed 0x19b5c4 /
// 0x19b648, the timer tail at 0x157430 and the static initialiser 0x34498c (llvm-objdump, REA pseudo code); no emulator trace covers them.
#include "runtime/vehicle_damage.hpp"
#include "runtime/vehicle_world.hpp"

#include <cmath>
#include <cstdio>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

VehicleRecord trooper() {
    VehicleRecord record;
    record.health = 5;
    record.hearts = 5;
    return record;
}

void constants() {
    expect(kVehicleHitCooldown == 0.5f && kVehicleDebrisCooldown == 1.0f, "0x3745a4 and 0x3745c4 hold 0.5 and 1.0 (static initialiser 0x34498c)");
}

void impactSpeeds() {
    VehicleRigidBody body;
    body.velocity = {0.0f, 0.0f, 27.0f};
    ManifoldPoint wall;
    wall.normalOnB = {0.0f, 0.0f, -1.0f};
    expect(near(manifoldImpactSpeed(body, true, wall, nullptr), 27.0f, 1e-5f), "side 0 negates the normal: a wall ahead gives the approach speed");
    expect(near(manifoldImpactSpeed(body, false, wall, nullptr), -27.0f, 1e-5f), "side 1 keeps the normal");
    VehicleRigidBody parked;
    parked.velocity = {0.0f, 0.0f, 7.0f};
    expect(near(manifoldImpactSpeed(body, true, wall, &parked), 20.0f, 1e-5f), "the other body's velocity at its arm is subtracted");
    VehicleRigidBody turning;
    turning.angularVelocity = {0.0f, 2.0f, 0.0f};
    ManifoldPoint corner;
    corner.localA = {1.5f, 0.0f, 0.0f};
    corner.normalOnB = {0.0f, 0.0f, -1.0f};
    expect(near(manifoldImpactSpeed(turning, true, corner, nullptr), -3.0f, 1e-5f), "v + w x arm with the point's local coordinates (w.y 2, arm.x 1.5: z -3)");
    FloorContact floor;
    floor.point = {0.0f, -1.0f, 0.0f};
    floor.normal = {0.0f, 1.0f, 0.0f};
    VehicleRigidBody falling;
    falling.velocity = {0.0f, -25.0f, 0.0f};
    expect(near(floorImpactSpeed(falling, floor), 25.0f, 1e-5f), "a floor contact: the fall speed along the normal");
}

void brickBursts() {
    VehicleRecord record = trooper();
    expect(!vehicleContactReply(record, 10.0f, {}).brickBurst, "10 m/s is not above the burst speed");
    ContactReply slow = vehicleContactReply(record, 12.0f, {});
    expect(slow.brickBurst && slow.brickCount == 1 && !slow.damage, "12 m/s: one brick burst of 1, no damage");
    expect(record.debrisCooldown == kVehicleDebrisCooldown, "the burst starts the 1 s cool-down");
    expect(!vehicleContactReply(record, 30.0f, {}).brickBurst, "no second burst in the cool-down");
    tickVehicleDamage(record, 0.5f);
    expect(record.debrisCooldown == 0.5f, "the cool-down counts down");
    tickVehicleDamage(record, 0.6f);
    expect(record.debrisCooldown == 0.0f, "and stops at 0");
    const ContactReply mid = vehicleContactReply(record, 25.0f, {});
    expect(mid.brickBurst && mid.brickCount == 2, "25 m/s gives 1 + 2 * 15 / 30 = 2");
    record.debrisCooldown = 0.0f;
    expect(vehicleContactReply(record, 90.0f, {}).brickCount == 3, "the burst speed is capped at 40: 3");
}

void damageMessage() {
    VehicleRecord record = trooper();
    ContactReply reply = vehicleContactReply(record, 20.0f, {});
    expect(!reply.damage, "20 m/s is not above the damage speed");
    reply = vehicleContactReply(record, 27.0f, {});
    expect(reply.damage && reply.damage->hearts == 0 && near(reply.damage->fraction, 0.875f, 1e-6f) && reply.damage->kind == 11, "27 m/s: (27 - 20) * 0.025 * 5 = 0.875 of a heart, kind 11");
    reply = vehicleContactReply(record, 100.0f, {});
    expect(reply.damage && reply.damage->hearts == 5 && reply.damage->fraction == 0.0f, "the speed is capped at 60: five hearts");
    ContactOther dead;
    dead.entity = true;
    dead.destroyed = true;
    expect(!vehicleContactReply(record, 40.0f, dead).damage, "a destroyed other entity does no damage");
    ContactOther soft;
    soft.entity = true;
    soft.hasBody = true;
    soft.noDamageBody = true;
    expect(!vehicleContactReply(record, 40.0f, soft).damage, "an other body with flag 0x40000 does no damage");
    soft.hasBody = false;
    expect(vehicleContactReply(record, 40.0f, soft).damage.has_value(), "the flag only counts with a body");
    expect(vehicleContactReply(record, 3.5f, {}).impactSound && !vehicleContactReply(record, 3.0f, {}).impactSound, "the impact sound above 3 m/s");
}

void heartsLost() {
    VehicleRecord record = trooper();
    DamageMessage hit;
    hit.fraction = 0.875f;
    hit.kind = kVehicleDamageKindContact;
    DamageOutcome outcome = vehicleDamage(record, hit, true);
    expect(outcome.applied && !outcome.heartLost && record.healthPoints == 13, "87 of 100 points: the first message leaves 13");
    outcome = vehicleDamage(record, hit, true);
    expect(outcome.heartLost && record.hearts == 4 && record.healthPoints == 100 && outcome.smokeEffect == kVehicleSmokeDamaged,
           "87 is not below 13: the heart is lost, the points are back at 100, smoke 0x11e");
    expect(record.hitCooldown == kVehicleHitCooldown, "the lost heart starts the 0.5 s cool-down");
    outcome = vehicleDamage(record, hit, true);
    expect(!outcome.applied && record.healthPoints == 100 && record.hearts == 4, "the cool-down refuses damage");
    tickVehicleDamage(record, 0.5f);
    vehicleDamage(record, hit, true);
    outcome = vehicleDamage(record, hit, true);
    expect(record.hearts == 3 && outcome.heartLost, "after the cool-down the next two messages cost another heart");
    tickVehicleDamage(record, 0.5f);
    vehicleDamage(record, hit, true);
    outcome = vehicleDamage(record, hit, true);
    expect(record.hearts == 2 && outcome.smokeEffect == kVehicleSmokeDamaged, "two hearts left: smoke 0x11e");
    tickVehicleDamage(record, 0.5f);
    vehicleDamage(record, hit, true);
    outcome = vehicleDamage(record, hit, true);
    expect(record.hearts == 1 && outcome.smokeEffect == kVehicleSmokeLastHeart, "one heart left: smoke 0x11f");
    tickVehicleDamage(record, 0.5f);
    vehicleDamage(record, hit, true);
    outcome = vehicleDamage(record, hit, true);
    expect(outcome.destroyed && record.destroyed && record.hearts == 0 && (record.flags311 & kVehicleFlagRevivable) == 0, "the last heart destroys the vehicle");
    tickVehicleDamage(record, 0.5f);
    expect(!vehicleDamage(record, hit, true).applied, "a destroyed vehicle takes no more damage");
}

void rules() {
    VehicleRecord record = trooper();
    DamageMessage hit;
    hit.fraction = 0.5f;
    hit.kind = kVehicleDamageKindContact;
    DamageOutcome outcome = vehicleDamage(record, hit, false);
    expect(outcome.applied && record.healthPoints == 100 && record.hearts == 5, "a vehicle the player does not drive loses no points to a contact message");
    hit.hearts = 3;
    outcome = vehicleDamage(record, hit, true);
    expect(outcome.heartLost && record.hearts == 4, "three hearts of a contact message count as one (type is not 6)");
    VehicleRecord other = trooper();
    other.flags311 = kVehicleInvulnerable;
    hit.hearts = 1;
    expect(!vehicleDamage(other, hit, true).applied, "the invulnerable flag refuses damage");
    DamageMessage scripted;
    scripted.hearts = 2;
    scripted.type = kVehicleDamageTypeScripted;
    scripted.blocked = 1;
    other.hitCooldown = 0.3f;
    outcome = vehicleDamage(other, scripted, true);
    expect(outcome.heartLost && other.hearts == 3, "type 6 ignores the blocks and costs the full count");
    scripted.hearts = 3;
    other.hitCooldown = 0.0f;
    outcome = vehicleDamage(other, scripted, true);
    expect(outcome.destroyed && (other.flags311 & kVehicleFlagRevivable) != 0, "type 6 that destroys marks the vehicle revivable (0x80)");
    other.hitCooldown = 0.0f;
    DamageMessage again;
    again.hearts = 1;
    again.type = kVehicleDamageTypeScripted;
    outcome = vehicleDamage(other, again, true);
    expect(outcome.applied, "a revivable vehicle without cool-down takes the next message again (0x179a2c clears the entity flag first)");
}

void pairRule() {
    const chase::assets::CollisionItem item;
    VehicleFloor floorA{FloorMesh(item), FloorTracker{}, VehicleCollisionBody{}, nullptr};
    VehicleFloor floorB{FloorMesh(item), FloorTracker{}, VehicleCollisionBody{}, nullptr};
    VehicleRecord plain = trooper();
    VehicleRecord bumper = trooper();
    bumper.bumpCollision = true;
    VehicleWorldBody a;
    VehicleWorldBody b;
    a.controls.driver = false;
    b.controls.driver = false;
    a.floor = &floorA;
    b.floor = &floorB;
    a.record = &plain;
    b.record = &bumper;
    ManifoldPoint point;
    applyPairContactRule(point, a, b);
    expect(point.flags == 0, "no driven body: the flag word stays 0");
    b.controls.driver = true;
    applyPairContactRule(point, a, b);
    expect(point.flags == 2, "a driven B sets bit 1 (the oracle trooper is the pair's first body)");
    a.controls.driver = true;
    b.controls.driver = false;
    ManifoldPoint other;
    applyPairContactRule(other, a, b);
    expect(other.flags == 1 && other.friction == 0.2f && other.restitution == 1.1f,
           "a driven A sets bit 0; against a BumpCollision vehicle friction 0.2 and restitution 1.1");
    std::vector<WheelState> air(4);
    air[0].contact = true;
    b.lastWheels = &air;
    ManifoldPoint airborne;
    applyPairContactRule(airborne, a, b);
    expect(airborne.flags == 0, "one wheel on the ground is not more than one: no flag");
    air[1].contact = true;
    applyPairContactRule(airborne, a, b);
    expect(airborne.flags == 1, "two wheels on the ground");
}

}

int main() {
    constants();
    impactSpeeds();
    brickBursts();
    damageMessage();
    heartsLost();
    rules();
    pairRule();
    if (failures == 0) std::printf("vehicle damage: ok\n");
    return failures == 0 ? 0 : 1;
}
