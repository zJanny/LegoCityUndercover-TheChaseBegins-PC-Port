// Pushables (src/runtime/pushable.cpp) on synthetic objects: the handle frames of 0x19aa88, the handle zone of 0x221570 with FaceTowards 4,
// the grab of 0x2aff3c, the player's handle states of 0x289d28 (blend, message 9 only with the blend complete, exit on a released stick) and
// the pushable's states of 0x281434 / 0x19aecc (move by PushSpeed, Destination within TriggerDistance, SinkHeight 0, TriggerObject at state 2).
// All rules from the code (docs/re/pushables.md, 20261026-crane-B); no values measured in the oracle.
#include "runtime/pushable.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

PushableObject cart() {
    PushableObject object;
    object.instance = "cart";
    object.destination = "cart_to";
    object.triggerObject = "cart_done";
    object.triggerDistance = 0.1f;
    object.pushSpeed = 0.06f;
    object.sinkHeight = 0.0f;
    object.autoPush = true;
    object.noGravity = true;
    object.canPush = true;
    object.soundPush = 337;
    object.node = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 2, 20, 1};
    const float centre[3] = {0, 1, 0}, half[3] = {1, 1, 2};
    std::copy_n(centre, 3, object.boundsCentre);
    std::copy_n(half, 3, object.boundsHalf);
    for (int slot = 0; slot < 4; ++slot) {
        PushHandle handle;
        handle.instance = "cart.handle0" + std::to_string(slot + 1);
        handle.local = pushHandleLocal(slot, object.boundsCentre, object.boundsHalf);
        handle.active = slot == 1;
        const float useCentre[3] = {0, -1, 0.75f}, useHalf[3] = {1, 2, 0.75f};
        std::copy_n(useCentre, 3, handle.useCentre);
        std::copy_n(useHalf, 3, handle.useHalf);
        object.handles.push_back(handle);
    }
    return object;
}

void handleFrames() {
    const float centre[3] = {0.5f, 1, -0.25f}, half[3] = {1, 1, 2};
    const PushMatrix front = pushHandleLocal(0, centre, half), back = pushHandleLocal(1, centre, half), right = pushHandleLocal(2, centre, half),
                     left = pushHandleLocal(3, centre, half);
    require(near(front[14], 1.75f) && near(front[8], 0) && near(front[10], 1) && near(front[0], 1), "front: z = centre + half, rows x and z");
    require(near(back[14], -2.25f) && near(back[10], -1) && near(back[0], -1), "back: z = centre - half, turned by pi");
    require(near(right[12], 1.5f) && near(right[8], 1) && near(right[2], -1), "right: x = centre + half, z row +x, x row -z");
    require(near(left[12], -0.5f) && near(left[8], -1) && near(left[2], 1), "left: x = centre - half, z row -x, x row +z");
}

void zoneAndGrab() {
    const PushableObject object = cart();
    const float behind[3] = {10.0f, 2.0f, 17.5f};
    const std::int16_t facingCart = 0, facingAway = std::int16_t(32767);
    require(pushHandleZoneHolds(object, object.handles[1], behind, facingCart), "behind the back handle, facing the cart: the zone holds");
    require(!pushHandleZoneHolds(object, object.handles[1], behind, facingAway), "facing away (FaceTowards 4): no");
    const float farBehind[3] = {10.0f, 2.0f, 16.0f};
    require(!pushHandleZoneHolds(object, object.handles[1], farBehind, facingCart), "more than 1.5 behind the face: no");
    require(!pushHandleZoneHolds(object, object.handles[0], behind, facingCart), "an inactive handle has no zone");
    const std::optional<std::size_t> grabbed = pushGrabTest(object, behind, facingCart);
    require(grabbed && *grabbed == 1, "the touching player in the back zone grabs handle02");
    require(!pushGrabTest(object, farBehind, facingCart), "no touch, no grab");
    PushableObject manual = cart();
    manual.autoPush = false;
    require(!pushGrabTest(manual, behind, facingCart), "without AutoPush the walk-in does not grab");
}

void pushToDestination() {
    PushableSet set;
    set.add(cart());
    PushGrip grip;
    const float start[3] = {10.0f, 2.0f, 17.5f};
    pushGripStart(grip, 0, 1, start);
    PushGripInput input;
    std::copy_n(start, 3, input.position);
    input.stickActive = true;
    input.dt = 1.0f / 30.0f;
    const auto noneDisabled = [](const std::string&) { return false; };
    const auto destination = [](const std::string& name, float out[3]) {
        if (name != "cart_to") return false;
        out[0] = 10.0f;
        out[1] = 2.0f;
        out[2] = 22.0f;
        return true;
    };
    int moves = 0, updates = 0;
    bool done = false, arrived = false, soundStarted = false;
    std::optional<std::uint16_t> firstState;
    while (!done && updates < 200) {
        const PushGripStep step = pushGripUpdate(grip, set, input);
        require(!step.exited, "the held stick keeps the grip");
        if (step.enteredState && !firstState) firstState = step.enteredState;
        input.position[0] = step.position[0];
        input.position[2] = step.position[2];
        input.facing = step.facing;
        if (updates < 6) require(!set.objects()[0].pushed, "no message 9 before the blend reaches 1.0 (update " + std::to_string(updates) + ")");
        for (const PushableEvent& event : set.update(noneDisabled, destination)) {
            arrived = arrived || event.kind == PushableEventKind::Arrived;
            done = done || event.kind == PushableEventKind::Done;
            soundStarted = soundStarted || event.kind == PushableEventKind::SoundStart;
            if (event.kind == PushableEventKind::Moved) ++moves;
        }
        ++updates;
    }
    require(firstState && *firstState == kPlayerStateHandlePush, "the first state entered is 85 HANDLE PUSH");
    require(arrived && done && soundStarted, "the pushable arrives, starts its push sound and reaches state 2");
    require(moves == 32, "32 moves of 0.06 bring it from z 20 to within 0.1 of z 22, got " + std::to_string(moves));
    require(updates == 6 + 32 + 1, "6 blend updates, 32 moves, then state 1 -> 2 in the next update, got " + std::to_string(updates));
    require(!set.objects()[0].handles[1].usable, "state 2 makes the handles unusable");
    require(!pushHandleZoneHolds(set.objects()[0], set.objects()[0].handles[1], input.position, input.facing), "so the zone no longer holds");
    require(near(input.position[2], 21.92f - 2.0f - 0.75f, 0.01f), "the player stays at the moving handle's use point");
}

void stickReleased() {
    PushableSet set;
    set.add(cart());
    PushGrip grip;
    const float start[3] = {10.0f, 2.0f, 17.5f};
    pushGripStart(grip, 0, 1, start);
    PushGripInput input;
    std::copy_n(start, 3, input.position);
    input.dt = 1.0f / 30.0f;
    const PushGripStep step = pushGripUpdate(grip, set, input);
    require(step.exited && !grip.active, "an AutoPush grip without the stick goes back to state 1");
    pushGripStart(grip, 0, 1, start);
    input.stickActive = true;
    input.stickHeading = std::int16_t(32767);
    require(pushGripUpdate(grip, set, input).exited, "a stick pointing away from the cart (no pull) exits");
}

}

int main() {
    try {
        handleFrames();
        zoneAndGrab();
        pushToDestination();
        stickReleased();
    } catch (const std::exception& error) {
        std::cerr << "pushable-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pushable-test: ok\n";
    return 0;
}
