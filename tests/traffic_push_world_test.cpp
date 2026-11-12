// Tests of src/runtime/traffic_push_world.*: the push vector and shape test 0x1f49d8, the squash 0x233594 and the listeners of the events 17 / 18
// (super knockback 0x3305fc, throw finisher 0x334888 -> 0x1e3040) with the delivery of message 45 (0x283d0c). Every expectation follows from the
// disassembly (llvm-objdump --triple=armv6k --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261112-traffic-C on 2026-11-12,
// docs/re/traffic-car-push.md): the control flow, the literals 0x1f4d2c (0.99), 0x1f5054 (0.5), 0x2335b8 (32000), 0x1e31d8 (10430.378), the
// flags 88 / 92 and mode 17 of 0x23d434, the state ids 0x99 / 0x199 and the listener types 17 / 18 of the initialisers 0x34eb60 and 0x34ffdc.
// Positions and boxes are made up; the expected floats repeat the instruction order. No emulator was used.
#include "runtime/traffic_push_world.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kMover = 1;
constexpr std::uint32_t kOwner = 2;

AiMatrix at(float x, float y, float z) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = {x, y, z};
    return m;
}

struct Scene {
    std::map<std::uint32_t, std::uint16_t> flags = {{kMover, 0x200}, {kOwner, 0x200}};
    std::map<std::uint32_t, std::uint8_t> kinds = {{kMover, 183}, {kOwner, 27}};
    std::map<std::uint32_t, AiMatrix> matrices = {{kMover, at(0.0f, 0.0f, 0.0f)}, {kOwner, at(0.0f, 0.0f, 1.0f)}};
    std::map<std::uint32_t, TrafficPushBox> boxes = {{kMover, {{0.0f, 0.5f, 0.0f}, {0.5f, 0.5f, 0.5f}}},
                                                     {kOwner, {{0.0f, 1.0f, 0.0f}, {0.25f, 1.0f, 0.25f}}}};
    bool mesh = false;
    bool meshHit = false;
    bool orientedHit = false;
    bool orientedSlide = true;
    float pushedClipZ = -1.0f;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, AiVec3>> moves;
    std::vector<std::uint32_t> lastHits;
    TrafficPushBox orientedBox;
    AiMatrix orientedMatrix{};
};

TrafficPushShapes shapesOf(Scene& s) {
    TrafficPushShapes shapes;
    shapes.flags = [&s](std::uint32_t e) { return s.flags[e]; };
    shapes.kind = [&s](std::uint32_t e) { return s.kinds[e]; };
    shapes.nodeMatrix = [&s](std::uint32_t e) { return s.matrices[e]; };
    shapes.box = [&s](std::uint32_t e) { return s.boxes[e]; };
    shapes.entryMatrix = [&s](const TrafficContactEntry& entry) { return s.matrices[entry.owner]; };
    shapes.meshEntry = [&s](const TrafficContactEntry&) { return s.mesh; };
    shapes.meshPush = [&s](std::uint32_t, const AiVec3&, const TrafficContactEntry&) {
        s.calls.push_back("mesh");
        return s.meshHit;
    };
    shapes.orientedPush = [&s](std::uint32_t, const AiVec3&, const TrafficContactEntry&, const AiMatrix& matrix, const TrafficPushBox& box,
                               const AiVec3&, const AiVec3&, bool slide) {
        s.calls.push_back("oriented");
        s.orientedSlide = slide;
        s.orientedBox = box;
        s.orientedMatrix = matrix;
        return s.orientedHit;
    };
    shapes.slideMove = [&s](std::uint32_t entity, AiVec3& move, std::uint32_t flags, float floorMin) {
        s.calls.push_back("slide " + std::to_string(entity) + " " + std::to_string(flags) + " " + (floorMin == 0.0f ? "0" : "floor"));
        if (entity == kOwner && s.pushedClipZ >= 0.0f && move[2] > s.pushedClipZ) move[2] = s.pushedClipZ;
    };
    shapes.moveNode = [&s](std::uint32_t entity, const AiVec3& delta) { s.moves.push_back({entity, delta}); };
    shapes.lastHit = [&s](std::uint32_t owner) { s.lastHits.push_back(owner); };
    return shapes;
}

TrafficContactEntry entry() {
    TrafficContactEntry e;
    e.owner = kOwner;
    e.ownerNode = 1002;
    return e;
}

void testBoxHitWithoutSlide() {
    Scene s;
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector = {9.0f, 9.0f, 9.0f};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry(), &vector, false), "the moved mover box overlaps the character box");
    require(vector == AiVec3{0.0f, 0.0f, 0.0f}, "0x25b390 clears the vector and nothing writes it without slide");
    require(s.calls.empty() && s.moves.empty() && s.lastHits.empty(), "the box path without slide moves nothing and leaves *0x377134");
}

void testBoxEdgesAreOpen() {
    Scene s;
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector{};
    require(!trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.25f}, entry(), &vector, false), "z -0.75 against 0.25 + 0.5 is outside");
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.5f}, entry(), &vector, false), "z -0.5 is inside");
    s.matrices[kOwner] = at(0.75f, 0.0f, 1.0f);
    require(!trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry(), &vector, false), "x exactly at the summed half extent is outside");
}

void testSlidePushesBothSides() {
    Scene s;
    s.pushedClipZ = 0.1f;
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector{};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry(), &vector, true), "slide hit");
    const float share = (-1.0f * 0.6f) * 0.5f;
    require(vector[0] == 0.0f && vector[2] == 0.1f && std::signbit(vector[1]), "the pushed character gets (-x, -0.0, -z) after its slide");
    require(s.calls == std::vector<std::string>{"slide 2 88 0", "slide 1 92 floor"}, "0x23d434 on the pushed (88, 0.0), then the mover (92, *0x376910)");
    require(s.moves.size() == 1 && s.moves[0].first == kMover, "only the mover's node moves");
    const float back = share + (0.1f - (-share));
    require(s.moves[0].second == AiVec3{0.0f, 0.0f, back}, "the mover moves back by its share plus what the pushed one could not take");
    require(s.lastHits.empty(), "the box path leaves *0x377134");
}

void testSlideDirectionFallback() {
    Scene s;
    s.matrices[kOwner] = at(0.0f, 0.0f, 0.0f);
    s.boxes[kOwner] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector{};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.4f}, entry(), &vector, true), "hit at the same place");
    require(vector[0] == 0.0f && vector[2] == -0.2f, "0x23ba14 gives (0, 1) for a zero offset: the pushed one goes -z");
    require(s.moves[0].second == AiVec3{0.0f, 0.0f, 0.2f}, "and the mover +z");
}

void testBlockersDoNotSlide() {
    Scene s;
    s.flags[kMover] = 0x300;
    s.flags[kOwner] = 0x200;
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector{};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry(), &vector, true), "a mover with 0x100 against an owner without it slides");
    require(!s.calls.empty(), "the slide runs");
    Scene b;
    b.flags[kMover] = 0x2300;
    b.flags[kOwner] = 0x300;
    b.orientedHit = true;
    const TrafficPushShapes blocked = shapesOf(b);
    require(trafficPushVector(blocked, kMover, {0.0f, 0.0f, 0.6f}, entry(), &vector, true), "the oriented hook decides");
    require(b.calls == std::vector<std::string>{"oriented"} && b.lastHits == std::vector<std::uint32_t>{kOwner}, "a hit of 0x27f174 sets *0x377134");
    require(vector == AiVec3{0.0f, 0.0f, 0.0f}, "the oriented path does not write the vector");
    require(!b.orientedSlide, "both entities with 0x100: 0x1f49d8 clears slide");
}

void testMeshAndExcludedKinds() {
    Scene s;
    s.mesh = true;
    s.meshHit = true;
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector{};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 5.0f}, entry(), &vector, true), "a mesh entry takes 0x287ac4");
    require(s.calls == std::vector<std::string>{"mesh"} && s.lastHits == std::vector<std::uint32_t>{kOwner}, "mesh hit sets *0x377134");
    Scene e;
    TrafficPushShapes excluded = shapesOf(e);
    excluded.excludedKinds = {27};
    require(!trafficPushVector(excluded, kMover, {0.0f, 0.0f, 0.6f}, entry(), &vector, true), "a kind of the list 0x58d9b0 is never hit");
}

void testTiltedBoxBecomesWorldBox() {
    const float c = 0.70710677f;
    Scene s;
    AiMatrix tilted = at(0.0f, 0.0f, 0.0f);
    tilted[0] = {c, c, 0.0f};
    tilted[1] = {-c, c, 0.0f};
    s.matrices[kOwner] = tilted;
    s.boxes[kOwner] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.1f, 0.1f}};
    s.boxes[kMover] = {{0.0f, 0.0f, 0.0f}, {0.05f, 0.05f, 0.05f}};
    s.matrices[kMover] = at(0.7f, -0.7f, 0.0f);
    const TrafficPushShapes shapes = shapesOf(s);
    AiVec3 vector{};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.0f}, entry(), &vector, false),
            "|row 1 y| 0.707 < 0.99: the test runs against the world box of 0x22ec3c (an oriented test would miss)");
    s.flags[kOwner] = 0x0a00;
    s.orientedHit = false;
    require(!trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.0f}, entry(), &vector, false), "0x800 takes the oriented hook instead");
    require(s.orientedMatrix[1][1] == c && s.orientedBox.half[0] == 1.0f, "the oriented hook gets the entry matrix and box unchanged");
}

void testNullVectorWithSlide() {
    Scene s;
    const TrafficPushShapes shapes = shapesOf(s);
    bool thrown = false;
    try {
        trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry(), nullptr, true);
    } catch (const std::logic_error&) {
        thrown = true;
    }
    require(thrown, "the original writes through the null vector; the port stops");
    require(s.calls.empty() && s.moves.empty(), "0x1f4f88 writes the vector before either slide, so nothing moves first");
}

void testSquash() {
    CharacterHitSender sender;
    sender.present = true;
    sender.kind = 64;
    const CharacterHitMessage hit = trafficSquashHit(sender, 0);
    require(!hit.move && hit.sender.present && hit.sender.kind == 64 && hit.damage == 32000 && hit.force == 0.0f && hit.kind == 0 &&
                hit.weapon == 0,
            "0x233594: {no move, sender, damage 32000, force 0, kind 0, weapon}");
}

void testSuperKnockback() {
    const CollisionEventOutcome hit = superKnockbackEvent(kCharacterEventCollider, 50u);
    require(hit.destroysSelf && hit.messageTarget == 50u && hit.hit && !hit.hit->sender.present && hit.hit->damage == 1 && hit.hit->kind == 0,
            "event 17 with a sender: message 0 {no sender, damage *0x377130} to it, then 0x23cd80(self, 0)");
    const CollisionEventOutcome alone = superKnockbackEvent(kCharacterEventCollider, std::nullopt);
    require(alone.destroysSelf && !alone.messageTarget, "without a sender only the destroy");
    const CollisionEventOutcome collidee = superKnockbackEvent(kCharacterEventCollidee, 51u);
    require(collidee.messageTarget == 51u && collidee.destroysSelf, "event 18 hits the pushed character");
}

ThrowFinisherOther characterOther(float x, float z) {
    ThrowFinisherOther other;
    other.entity = 60;
    other.kind = 27;
    other.character = true;
    other.takesCombat = true;
    other.reachable = true;
    other.position = {x, 0.0f, z};
    return other;
}

void testThrowFinisher() {
    CharacterHitSender self;
    self.present = true;
    self.character = true;
    self.kind = 27;
    const CollisionEventOutcome combat = throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, characterOther(2.0f, 0.0f));
    require(combat.combatTarget == 60u && !combat.messageTarget && combat.hit && combat.hit->kind == 15 && combat.hit->damage == 1 &&
                combat.hit->sender.kind == 27,
            "a character goes through 0x22e884 with {sender self, damage 1, kind 15}");
    require(combat.combatHeading == std::uint16_t(49152), "it faces the finisher: atan2f(-1, 0) x 10430.378 = -16384");
    ThrowFinisherOther own = characterOther(0.0f, -2.0f);
    own.target140 = 40;
    require(!throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, own).hit, "ctrl +0x140 already the finisher: nothing");
    own.target140 = 0;
    own.ability52 = true;
    require(!throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, own).hit, "ability 52: nothing");
    own.ability52 = false;
    own.reachable = false;
    require(!throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, own).hit, "0x21a2d4 refuses: nothing");

    ThrowFinisherOther car;
    car.entity = 70;
    car.kind = 183;
    require(!throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, car).hit, "a car without controller is not hit");
    car.kind = 119;
    require(throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, car).messageTarget == 70u, "kinds 85, 119, 214 take message 0");
    car.kind = 183;
    car.controlled = true;
    car.ctrlCC = 7;
    require(throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, car).messageTarget == 70u, "a controller with +0xcc 1, 7 or 11 takes message 0");
    car.ctrlBitDE0 = true;
    require(!throwFinisherEvent(self, 40, {0.0f, 0.0f, 0.0f}, car).hit, "unless its +0xde bit 0 is set");
}

void testStatesAndDelivery() {
    CharacterHitSender self;
    require(!characterCollisionEvent(1, kCharacterEventCollider, self, 40, {}, std::nullopt).destroysSelf, "other states have no listener");
    require(characterCollisionEvent(kCharacterStateSuperKnockback, kCharacterEventCollider, self, 40, {}, std::nullopt).destroysSelf, "state 0x99");

    CharacterPushSender car;
    car.entity = 70;
    car.kind = 183;
    car.node = at(0.0f, 0.0f, 0.0f);
    car.halfLength = 2.0f;
    car.forwardSpeed = 8.0f;
    ThrowFinisherOther carOther;
    carOther.entity = 70;
    carOther.kind = 183;
    CarPushCharacter walker;
    walker.entity = 40;
    walker.state = 1;
    walker.position = {0.0f, 0.0f, 3.0f};
    CarPushDelivery delivery = deliverCarPush(walker, car, carOther, 0, CharacterHitSender{}, {}, std::nullopt);
    require(!delivery.event17.destroysSelf && !delivery.event18 && delivery.push.hit && delivery.push.hit->kind == 3, "an idle walker only takes the hit");
    CharacterHitTarget target;
    const auto result = applyCarPushToCharacter(target, delivery);
    require(result && result->outcome == CharacterHitOutcome::Reacted && target.health == 4 && !target.destroyed,
            "receiveHit takes the car's hit with damage 0 for a non-player (the kind 3 push of 0x215634 is COMBAT-4)");

    walker.state = kCharacterStateSuperKnockback;
    delivery = deliverCarPush(walker, car, carOther, 0, CharacterHitSender{}, {}, std::nullopt);
    require(delivery.event17.destroysSelf && delivery.event17.messageTarget == 70u, "in state 0x99 event 17 hits the car and destroys the character");
    CharacterHitTarget flying;
    applyCarPushToCharacter(flying, delivery);
    require(flying.destroyed, "0x23cd80 sets entity +0xc bit 4 before the hit");

    CharacterPushSender pusher;
    pusher.entity = 80;
    pusher.character = true;
    ThrowFinisherOther walkerOther = characterOther(0.0f, 3.0f);
    walkerOther.entity = 40;
    walker.state = 1;
    delivery = deliverCarPush(walker, pusher, std::nullopt, kCharacterStateThrowFinisher, self, {0.0f, 0.0f, 0.0f}, walkerOther);
    require(delivery.event18 && delivery.event18->combatTarget == 40u && !delivery.push.hit,
            "a character sender in state 0x199 takes event 18 and throws its finisher at the pushed character");
}

}

int main() {
    try {
        testBoxHitWithoutSlide();
        testBoxEdgesAreOpen();
        testSlidePushesBothSides();
        testSlideDirectionFallback();
        testBlockersDoNotSlide();
        testMeshAndExcludedKinds();
        testTiltedBoxBecomesWorldBox();
        testNullVectorWithSlide();
        testSquash();
        testSuperKnockback();
        testThrowFinisher();
        testStatesAndDelivery();
    } catch (const std::exception& error) {
        std::cerr << "traffic-push-world: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-push-world: ok\n";
    return 0;
}
