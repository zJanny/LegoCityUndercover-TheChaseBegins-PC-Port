// The platform node move (docs/re/props-rocking.md): the trigger messages of 0x1a8d74 with the request 0x2150bc, the travel and its ends of
// 0x2807c8 and the node hierarchy of AttachObject, with made-up records. Expected values are hand evaluations of that code (20261026-crane-A,
// 2026-10-26, llvm-objdump of 0x2807c8..0x281418, 0x2150bc..0x215264, 0x1a8e7c..0x1a8f3c, 0x1a945c..0x1a9500); the crane values were recorded
// by RPC 20261025-garage-C-1 (recorded from a save state on the generator trigger spot of mission 1-3, the game's dt 0.033).
#include "runtime/platform_node_move.hpp"
#include "runtime/platform_wait.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

constexpr float kTraceDt = 0.033f;

PlatformRecord crane() {
    PlatformRecord record;
    record.motion.type = 1;
    record.motion.placement = record.motion.origin = {948.259f, 16.6251f, 1147.005f};
    record.motion.target = {948.259f, 5.9968f, 1147.005f};
    return record;
}

PlatformRecord travel(std::uint8_t type) {
    PlatformRecord record;
    record.motion.type = type;
    record.speed = 0.25f;
    record.motion.origin = record.motion.placement = {0, 0, 0};
    record.motion.target = {0, 4, 0};
    return record;
}

void craneLowersToItsTarget() {
    PlatformRecord record = crane();
    record.value = 0.5f;
    platformMessage(record, 0xff);
    require(record.requested == kPlatformMovingState && record.value == 0.0f,
            "u90: the trigger requests state 2 and ResetProgressOnTrigger (default 1) sets the value to 0.0 (literal 0x1a906c)");
    require(!platformUpdate(record, kTraceDt) && record.state == kPlatformMovingState && record.value == 0.0f,
            "u90: no step in the trigger's update, the state is 2 at its end");
    std::optional<PlatformVector> node;
    int moves = 0;
    float firstFall = 0;
    while (record.state == kPlatformMovingState && moves < 200) {
        node = platformUpdate(record, kTraceDt);
        require(node.has_value(), "the node moves in every update of state 2");
        ++moves;
        if (moves == 1) firstFall = 16.6251f - (*node)[1];
        if (moves == 60) require(near(record.value, 0.594f, 1e-4f), "u150: 0.594 as the oracle read");
        if (moves == 101) require(near(record.value, 0.9999f, 1e-4f) && near((*node)[1], 16.6251f - 10.6283f * 0.9999f, 1e-3f), "u191: 0.9999");
    }
    require(moves == 102, "u91..u192: 102 node moves (oracle), got " + std::to_string(moves));
    require(near(firstFall, 0.10522f, 1e-4f), "the node falls 0.105 per update (oracle 0.105; 10.6283 x 0.0099)");
    require(record.value == 1.0f && record.state == kPlatformArrivedState && near((*node)[1], 5.9968f, 1e-5f) && (*node)[0] == 948.259f,
            "u192: type 1 at 1.0 requests state 5 and the node stands on the target (oracle y 5.9968, x and z unchanged)");
    require(!platformUpdate(record, kTraceDt), "state 5 does not move the node");
    platformMessage(record, 0xff);
    require(record.requested == kPlatformArrivedState, "type 1: a trigger in state 5 changes nothing (0x215250)");
}

void untriggerTakesTypeOneBack() {
    PlatformRecord record = crane();
    record.state = record.requested = kPlatformArrivedState;
    record.value = 1.0f;
    platformMessage(record, 0xfe);
    require(record.requested == kPlatformReturningState && record.value == 1.0f, "0xfe: 0x2150bc backwards with progress 1.0 (literal 0x1a9068)");
    platformUpdate(record, kTraceDt);
    std::optional<PlatformVector> node;
    int moves = 0;
    while (record.state == kPlatformReturningState && moves < 200) {
        node = platformUpdate(record, kTraceDt);
        ++moves;
    }
    require(moves == 102 && record.state == kPlatformStoppedState && record.value == 0.0f && near((*node)[1], 16.6251f, 1e-5f),
            "type 1 at 0.0 requests state 0 and stands on the origin (0x280c18), got " + std::to_string(moves));
    PlatformRecord stop = crane();
    stop.motion.stopOnUntrigger = true;
    stop.state = stop.requested = kPlatformMovingState;
    stop.value = 0.4f;
    platformMessage(stop, 0xfe);
    require(stop.requested == kPlatformStoppedState, "StopOnUntrigger: 0xfe requests state 0 (0x1a8ee4)");
    platformMessage(stop, 0xff);
    require(stop.requested == kPlatformMovingState && stop.value == 0.4f, "StopOnUntrigger: 0xff goes on from the value it has");
    PlatformRecord lift = travel(6);
    lift.value = 0.2f;
    platformMessage(lift, 0xff);
    require(lift.requested == kPlatformStoppedState, "type 6 above 0.0 ignores the trigger (0x1a8e94)");
    platformMessage(lift, 0xfe);
    require(lift.requested == kPlatformStoppedState, "type 6 ignores the untrigger");
}

void travelEndsByType() {
    PlatformRecord bounce = travel(2);
    bounce.state = bounce.requested = kPlatformMovingState;
    for (int i = 0; i < 4; ++i) platformUpdate(bounce, 1.0f / 30.0f);
    require(bounce.state == kPlatformReturningState && bounce.value == 1.0f, "type 2 at 1.0 turns back: state 3 with 2.0 - 1.0 (0x280afc)");
    std::optional<PlatformVector> node = platformUpdate(bounce, 1.0f / 30.0f);
    require(near(bounce.value, 0.75f, 1e-6f) && node && near((*node)[1], 3.0f, 1e-5f), "state 3 subtracts the step");
    for (int i = 0; i < 3; ++i) platformUpdate(bounce, 1.0f / 30.0f);
    require(bounce.state == kPlatformArrivedState && bounce.value == 0.0f, "type 2 at 0.0 requests state 5 (0x280bf0)");

    PlatformRecord ping = travel(3);
    ping.state = ping.requested = kPlatformMovingState;
    for (int i = 0; i < 8; ++i) platformUpdate(ping, 1.0f / 30.0f);
    require(ping.state == kPlatformMovingState && ping.value == 0.0f, "type 3 at 0.0 goes forwards again: state 2 with -0.0 (0x280bd8)");

    PlatformRecord loop = travel(3);
    loop.motion.looped = true;
    loop.speed = 0.3f;
    loop.state = loop.requested = kPlatformMovingState;
    for (int i = 0; i < 4; ++i) platformUpdate(loop, 1.0f / 30.0f);
    require(loop.state == kPlatformMovingState && near(loop.value, 0.2f, 1e-5f), "Looped: 1.2 - 1.0 with the unclamped value (0x280ad8)");

    PlatformRecord continuous = travel(0);
    continuous.motion.continuousWait = 3;
    continuous.state = continuous.requested = kPlatformMovingState;
    for (int i = 0; i < 4; ++i) platformUpdate(continuous, 1.0f / 30.0f);
    require(continuous.state == kPlatformMovingState && continuous.value == 1.0f && continuous.motion.continuousCount == 1, "type 0 waits at 1.0 (R+0xac < R+0xae)");
    platformUpdate(continuous, 1.0f / 30.0f);
    platformUpdate(continuous, 1.0f / 30.0f);
    require(continuous.state == kPlatformReturningState && continuous.motion.continuousCount == 0, "then turns back like type 2 (0x280acc)");

    PlatformRecord controlled = travel(1);
    controlled.state = controlled.requested = kPlatformControlledState;
    controlled.value = 0.5f;
    node = platformUpdate(controlled, 1.0f / 30.0f);
    require(controlled.value == 0.5f && node && near((*node)[1], 2.0f, 1e-6f), "state 4: no step, the node follows the value (0x280c4c)");
}

void attachedNodesFollow() {
    NodeCarry carry;
    carry.setParent("girder", "platform");
    carry.attach("platform", "girder");
    carry.setParent("guy", "girder");
    carry.setParent("crow", "girder");
    carry.move("platform", {0.0f, -10.6283f, 0.0f});
    require(carry.offset("platform")[1] == -10.6283f && carry.offset("girder")[1] == -10.6283f && carry.offset("guy")[1] == -10.6283f,
            "children follow the moved node (0x245008 keeps the child under the parent node)");
    const std::vector<std::string> moved = carry.takeMoved();
    require(moved.size() == 4 && moved[0] == "platform" && moved[1] == "girder", "the moved objects in order");
    carry.setParent("guy", "");
    carry.move("platform", {0.0f, 1.0f, 0.0f});
    require(carry.offset("guy")[1] == -10.6283f && carry.offset("girder")[1] == -9.6283f, "a detached node stays where it was carried");
    for (const char* name : {"girder", "a", "b", "c", "d", "e"}) carry.attach("platform", name);
    require(carry.attached("platform").size() == 5 && carry.attached("platform")[0] == "girder", "message 43 keeps each object once, at most five (0x1a94e8)");
    require(carry.attached("girder").empty() && carry.offset("nothing")[1] == 0.0f, "nothing for objects without a list or a move");
}

}

int main() {
    try {
        craneLowersToItsTarget();
        untriggerTakesTypeOneBack();
        travelEndsByType();
        attachedNodesFollow();
    } catch (const std::exception& error) {
        std::cerr << "platform-node-move: " << error.what() << '\n';
        return 1;
    }
    std::cout << "platform-node-move: ok\n";
    return 0;
}
