// Tests of src/runtime/traffic_car_push.*: the push of a gathered character 0x2baef0 (hook `push` of slideTrafficCarContacts), the character's
// handler of message 45 (0x283d0c) and the cop whistle state 0x1b2 (request 0x200354, enter 0x276338 / 0x26e254, update 0x29a2e0). Every
// expectation follows from the disassembly (llvm-objdump --triple=armv6k --mattr=+vfp2 of artifacts/game/game.arm.elf, read by
// 20261111-chase-F on 2026-11-12, docs/re/traffic-car-push.md): the control flow of 0x2baef0, the literals 0x283ea8 (0.5), 0x283eac (70.0),
// 0x283eb0 (5.0), cosf(70.0) = 0.6333192 evaluated in double precision, and the literals 0x26e260 / 0x26e270 (60, 120). No emulator was used.
#include "runtime/traffic_car_push.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kCar = 100;

struct Recorder {
    std::map<std::uint32_t, std::uint16_t> flags;
    std::map<std::uint32_t, bool> bit3;
    std::map<std::uint32_t, bool> hit;           // owners the vector hook accepts
    std::map<std::uint32_t, bool> blocked;       // other owners the test call accepts
    std::vector<std::string> calls;
    bool carIsCharacter = false;
    AiVec3 vectorOut = {0.0f, 0.0f, 1.5f};
    AiVec3 secondOut = {0.25f, 0.0f, 0.0f};
    std::vector<std::pair<std::uint32_t, AiVec3>> moves;
};

TrafficPushWorld worldOf(Recorder& r) {
    TrafficPushWorld world;
    world.flags = [&r](std::uint32_t e) { return r.flags[e]; };
    world.bit3 = [&r](std::uint32_t e) { return r.bit3[e]; };
    world.ignorePair = [&r](std::uint32_t, std::uint32_t) { return false; };
    world.pushVector = [&r](std::uint32_t mover, AiVec3&, const TrafficContactEntry& entry, AiVec3* vector, bool slide) {
        r.calls.push_back("vector " + std::to_string(mover) + " " + std::to_string(entry.owner) + (vector ? " out" : " test") + (slide ? " slide" : ""));
        if (vector && mover == kCar) {
            if (!r.hit[entry.owner]) return false;
            *vector = r.vectorOut;
            return true;
        }
        if (vector) {
            if (!r.hit[mover * 1000 + entry.owner]) return false;
            *vector = r.secondOut;
            return true;
        }
        return r.blocked[entry.owner];
    };
    world.message45 = [&r](std::uint32_t character, std::uint32_t pusher, const AiVec3* vector) {
        r.calls.push_back("message45 " + std::to_string(character) + " " + std::to_string(pusher) + (vector ? " vec" : " none"));
    };
    world.isCharacter = [&r](std::uint32_t) { return r.carIsCharacter; };
    world.squash = [&r](std::uint32_t car, std::uint32_t character) { r.calls.push_back("squash " + std::to_string(car) + " " + std::to_string(character)); };
    world.moveNode = [&r](std::uint32_t entity, const AiVec3& delta) {
        r.calls.push_back("move " + std::to_string(entity));
        r.moves.push_back({entity, delta});
    };
    return world;
}

TrafficContactEntry entryOf(std::uint32_t owner, std::uint32_t word = 0) {
    TrafficContactEntry entry;
    entry.owner = owner;
    entry.word = word;
    entry.ownerNode = owner + 1000;
    return entry;
}

bool run(const TrafficPushHook& hook, std::vector<TrafficContactEntry>& entries, std::size_t index, bool slide) {
    AiVec3 move = {0.0f, 0.0f, 1.0f};
    return hook(move, index, entries, slide);
}

void testPairGates() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush};
    r.flags[7] = kEntityFlagTakesPush;
    r.hit[7] = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7)};
    const TrafficPushHook hook = makeTrafficCarPush(car, world);

    world.ignorePair = [](std::uint32_t, std::uint32_t) { return true; };
    require(!run(makeTrafficCarPush(car, world), entries, 0, true) && r.calls.empty(), "a pair without collision does nothing");
    world = worldOf(r);

    TrafficPushCar noFlag{kCar, 0};
    require(!run(makeTrafficCarPush(noFlag, world), entries, 0, false) && r.calls.empty(), "the car needs hword +0x10 bit 0x200");
    r.flags[7] = 0;
    require(!run(hook, entries, 0, false) && r.calls.empty(), "the character needs hword +0x10 bit 0x200");
    r.flags[7] = kEntityFlagTakesPush;
    r.hit[7] = false;
    require(!run(hook, entries, 0, false) && r.calls.size() == 1, "without a push vector nothing is sent");
    require(r.moves.empty(), "and nothing moves");
}

void testNoSlideSendsMessage() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush};
    r.flags[7] = kEntityFlagTakesPush;
    r.hit[7] = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7)};
    require(run(makeTrafficCarPush(car, world), entries, 0, false), "without slide the push result is 1 after the message");
    require(r.calls.size() == 2 && r.calls[1] == "message45 7 100 vec", "message 45 goes to the character with the car and the vector");
    require(r.moves.empty(), "the character is not moved by the car's push");
}

void testSquash() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush};
    r.flags[7] = kEntityFlagTakesPush;
    r.hit[7] = true;
    r.bit3[7] = true;
    r.carIsCharacter = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7)};
    require(!run(makeTrafficCarPush(car, world), entries, 0, true), "the squash path gives 0");
    require(r.calls.back() == "squash 100 7" && r.moves.empty(), "0x233594(car, character, 0) runs and nothing else");
    r.calls.clear();
    r.carIsCharacter = false;
    run(makeTrafficCarPush(car, world), entries, 0, true);
    require(r.calls.back() == "move 7", "bit 3 without a character car does not squash");
}

void testBlockerAnswersCar() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush | kEntityFlagReturnsPush};
    r.flags[7] = kEntityFlagTakesPush;
    r.hit[7] = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7)};
    require(run(makeTrafficCarPush(car, world), entries, 0, true), "a car with bit 0x2000 gets the answer and the result 1");
    require(r.calls.back() == "message45 100 7 none" && r.moves.empty(), "message 45 with (character, 0) goes back to the car");

    r.calls.clear();
    car.flags = kEntityFlagTakesPush;
    r.flags[7] = kEntityFlagTakesPush | kEntityFlagBlocker;
    require(run(makeTrafficCarPush(car, world), entries, 0, true) && r.calls.back() == "message45 100 7 none", "a character with bit 0x100 answers the car too");
}

void testBlockingEntriesAreTried() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush};
    for (std::uint32_t o : {7u, 8u, 9u, 10u, 11u, 12u}) r.flags[o] = kEntityFlagTakesPush;
    r.flags[8] |= kEntityFlagBlocker;
    r.flags[9] |= kEntityFlagBlocker;
    r.flags[10] |= kEntityFlagBlocker;
    r.flags[11] = kEntityFlagBlocker;
    r.hit[7] = true;
    r.blocked[9] = true;
    r.blocked[10] = true;
    r.blocked[11] = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7), entryOf(kCar, 0), entryOf(8, kTrafficContactSkipBit), entryOf(9), entryOf(10), entryOf(12)};
    run(makeTrafficCarPush(car, world), entries, 0, true);
    std::string tests;
    for (const std::string& call : r.calls)
        if (call.rfind("vector 100", 0) == 0 && call.find("test") != std::string::npos) tests += call + ";";
    require(tests == "vector 100 9 test slide;", "the test loop skips the car, the character, skip-bit entries and non-blockers, and stops at the first hit");
}

void testSecondPushMovesBlockedEntity() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush};
    r.flags[7] = kEntityFlagTakesPush;
    r.flags[20] = kEntityFlagTakesPush;
    r.flags[21] = kEntityFlagTakesPush | kEntityFlagBlocker;
    r.hit[7] = true;
    r.hit[7 * 1000 + 20] = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7), entryOf(20)};
    require(!run(makeTrafficCarPush(car, world), entries, 0, true), "the plain push gives 0");
    require(r.moves.size() == 2 && r.moves[0].first == 20 && r.moves[0].second == r.secondOut, "the entity the character would hit moves by the second vector first");
    require(r.moves[1].first == 7 && r.moves[1].second == r.vectorOut, "then the character moves by the car's vector");

    Recorder blocker;
    TrafficPushWorld w2 = worldOf(blocker);
    blocker.flags[7] = kEntityFlagTakesPush;
    blocker.flags[21] = kEntityFlagTakesPush | kEntityFlagBlocker;
    blocker.hit[7] = true;
    blocker.hit[7 * 1000 + 21] = true;
    std::vector<TrafficContactEntry> second = {entryOf(7), entryOf(21)};
    run(makeTrafficCarPush(car, w2), second, 0, true);
    require(blocker.moves.size() == 1 && blocker.moves[0].first == 7, "a blocker with bit 0x100 is not moved, only the character");

    Recorder none;
    TrafficPushWorld w3 = worldOf(none);
    none.flags[7] = kEntityFlagTakesPush;
    none.hit[7] = true;
    std::vector<TrafficContactEntry> alone = {entryOf(7)};
    run(makeTrafficCarPush(car, w3), alone, 0, true);
    require(none.moves.size() == 1 && none.moves[0].first == 7, "without another entity only the character moves");
}

void testSlideThroughContactHook() {
    Recorder r;
    TrafficPushWorld world = worldOf(r);
    TrafficPushCar car{kCar, kEntityFlagTakesPush};
    r.flags[7] = kEntityFlagTakesPush;
    r.hit[7] = true;
    std::vector<TrafficContactEntry> entries = {entryOf(7)};
    TrafficContactCar contact;
    contact.node = 5000;
    contact.nodeParent = 5001;
    contact.radius = 2.0f;
    TrafficContactSlideHooks hooks;
    hooks.ignorePair = [](std::uint32_t) { return false; };
    hooks.push = makeTrafficCarPush(car, world);
    AiVec3 move = {0.0f, 0.0f, 10.0f};
    require(!slideTrafficCarContacts(contact, move, entries, hooks), "a plain slide push returns 0 to 0x2c8f80");
    require(move[2] == 10.0f, "and does not cut the move");
}

AiMatrix identityAt(float x, float y, float z) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = {x, y, z};
    return m;
}

CharacterPushSender vehicle(float speed) {
    CharacterPushSender sender;
    sender.entity = kCar;
    sender.kind = kEntityKindVehicle;
    sender.node = identityAt(10.0f, 0.0f, 20.0f);
    sender.localPoint = {0.0f, 0.0f, 0.0f};
    sender.halfLength = 4.0f;
    sender.forwardSpeed = speed;
    return sender;
}

void testCharacterHandler() {
    const AiVec3 ahead = {10.0f, 0.0f, 24.0f};
    CharacterPushResult none = receiveCarPush(std::nullopt, ahead, false);
    require(none.stateEvent17 && !none.hit && !none.senderEvent18 && none.clearsPushBit, "without a sender only event 17 goes to the state and bit 2 is cleared");

    CharacterPushSender walker = vehicle(9.0f);
    walker.character = true;
    CharacterPushResult answer = receiveCarPush(walker, ahead, false);
    require(answer.senderEvent18 == kCar && !answer.hit, "a character sender gets event 18 back");

    CharacterPushSender prop = vehicle(9.0f);
    prop.kind = 119;
    require(!receiveCarPush(prop, ahead, false).hit, "other kinds than 179, 183 and 243 give no hit");

    const CharacterPushResult hit = receiveCarPush(vehicle(9.0f), ahead, false);
    require(hit.hit.has_value(), "a vehicle of kind 183 at forward speed 9 with the character 4 m ahead of the front point hits");
    require(hit.hit->damage == 0 && hit.hit->force == 9.0f && hit.hit->kind == 3 && hit.hit->weapon == 1 && !hit.hit->move,
            "message 0 carries damage 0, the forward speed as force, kind 3 and weapon 1");
    require(hit.hit->sender.present && hit.hit->sender.kind == kEntityKindVehicle && !hit.hit->sender.character, "the sender is the vehicle");
    require(receiveCarPush(vehicle(9.0f), ahead, true).hit->damage == 1, "the player takes damage 1");

    require(!receiveCarPush(vehicle(4.9f), ahead, false).hit, "forward speed 4.9 is below 5");
    require(receiveCarPush(vehicle(5.0f), ahead, false).hit.has_value(), "forward speed 5 is enough");
    require(receiveCarPush(vehicle(5.0f), ahead, false).hit->force == 5.0f, "the force is the speed, not its square");
    require(!receiveCarPush(vehicle(-9.0f), ahead, false).hit, "a reversing car never hits");
    require(!receiveCarPush(vehicle(9.0f), {10.0f, 0.0f, 20.0f}, false).hit, "a character behind the front point (dot -2) is not hit");
    require(!receiveCarPush(vehicle(9.0f), {10.0f, 0.0f, 22.6f}, false).hit, "dot 0.6 is below cosf(70) = 0.6333");
    require(receiveCarPush(vehicle(9.0f), {10.0f, 0.0f, 22.65f}, false).hit.has_value(), "dot 0.65 is above it");

    CharacterPushSender turned = vehicle(9.0f);
    turned.node[0] = {0.0f, 0.0f, -1.0f};
    turned.node[2] = {1.0f, 0.0f, 0.0f};
    turned.node[3] = {10.0f, 0.0f, 20.0f};
    require(receiveCarPush(turned, {14.0f, 0.0f, 20.0f}, false).hit.has_value(), "the front point follows row 2 of the node matrix");
    turned.localPoint = {0.0f, 0.0f, 1.0f};
    require(!receiveCarPush(turned, {13.5f, 0.0f, 20.0f}, false).hit && receiveCarPush(turned, {13.7f, 0.0f, 20.0f}, false).hit.has_value(),
            "the local point offset moves the front point (front point x 10 + 1 + 2 = 13 along row 2, limit 0.6333)");
}

void testWhistleGate() {
    CopWhistleGate gate;
    gate.extraActive = true;
    gate.state = kCopWhistleRequestState;
    gate.downPressed = true;
    gate.word13c = 0;
    gate.characterClass = kCopWhistleCharacterClass;
    require(copWhistleRequested(gate), "all five reads of 0x2008c4..0x200924 pass");
    CopWhistleGate g = gate;
    g.extraActive = false;
    require(!copWhistleRequested(g), "extras bit 19 off (the mask is 0 after a load)");
    g = gate;
    g.state = 2;
    require(!copWhistleRequested(g), "only in the idle state");
    g = gate;
    g.downPressed = false;
    require(!copWhistleRequested(g), "needs the D-pad Down press edge");
    g = gate;
    g.word13c = 1;
    require(!copWhistleRequested(g), "ctrl +0x13c must be 0");
    g = gate;
    g.characterClass = 2;
    require(!copWhistleRequested(g), "the character row's byte +0x20 must be 3");
}

void testWhistleEnterAndUpdate() {
    TrafficWhistleStop stop;
    GameRandom random(1);
    GameRandom expected(1);
    const std::uint32_t clock = gameClockAfter(10);
    require(clock == 10, "one clock unit per update (STAND-IN TRAFFIC-S33)");
    const CopWhistleEnter enter = enterCopWhistle(stop, clock, random);
    require(enter.clipRow == 709 && enter.blend == 0.1f && enter.speed == 1.0f, "the enter starts clip row 709 with blend 0.1 and speed 1.0");
    require(stop.until == clock + expected.next() % 60 + 120, "the stop ends at clock + random % 60 + 120");
    require(trafficWhistleStops(stop, clock) && trafficWhistleStops(stop, stop.until - 1) && !trafficWhistleStops(stop, stop.until),
            "the stop runs until its end and releases then");
    require(stop.until >= clock + 120 && stop.until < clock + 180, "between 4 and 6 seconds at 30 updates per second");
    require(!updateCopWhistle(false) && updateCopWhistle(true) == kCopWhistleRequestState, "the update returns to idle when the clip finished");
    require(kControllerStateCopWhistle == 434, "state 0x1b2");
}

}

int main() {
    try {
        testPairGates();
        testNoSlideSendsMessage();
        testSquash();
        testBlockerAnswersCar();
        testBlockingEntriesAreTried();
        testSecondPushMovesBlockedEntity();
        testSlideThroughContactHook();
        testCharacterHandler();
        testWhistleGate();
        testWhistleEnterAndUpdate();
    } catch (const std::exception& error) {
        std::cerr << "traffic-car-push: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-car-push: ok\n";
    return 0;
}
