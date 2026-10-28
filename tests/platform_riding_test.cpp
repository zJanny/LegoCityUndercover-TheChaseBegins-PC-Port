// Riding moving platforms (docs/re/props-rocking-platforms.md): the platform natives SetPlatformSpeed (0x1b5d38), SetPlatformOrigin / SetPlatformTarget
// (0x1c416c / 0x1c41a4), MovePlatform (message 26), the reset message 8 of 0x1a8d74, the ground entity ride 0x2b01e4, the rise of the mover 0x20cd30 and the
// lever zone turned into the node frame (0x23e130 with last argument 0). Values from the code read with REA (Ghidra) by 20261027-mission13-B on 2026-10-28.
#include "runtime/platform_riding.hpp"
#include "runtime/player_motion.hpp"
#include "runtime/switch_set.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void speedNative() {
    PlatformRecord record;
    setPlatformSpeed(record, 0.013f);
    require(record.speed == 0.013f, "SetPlatformSpeed stores the speed at R+0x74");
    setPlatformSpeed(record, 0.0f);
    require(record.speed == kPlatformSpeedMinimum, "a speed whose bits are below 0x34000000 becomes 2^-23");
    setPlatformSpeed(record, -1.0f);
    require(record.speed == kPlatformSpeedMinimum, "negative speeds have negative bits and become 2^-23 too");
}

void originTargetAndMove() {
    PlatformRecord record;
    record.motion.type = 1;
    setPlatformOrigin(record, {1, 2, 3});
    setPlatformTarget(record, {4, 5, 6});
    require(record.motion.origin == PlatformVector{1, 2, 3} && record.motion.target == PlatformVector{4, 5, 6}, "origin R+0x18 and target R+0x24");
    movePlatform(record, 0.25f, false);
    require(record.requested == kPlatformMovingState && record.value == 0.25f, "MovePlatform forwards: 0x2150bc requests state 2 and sets the progress");
    record.requested = kPlatformArrivedState;
    movePlatform(record, 1.0f, true);
    require(record.requested == kPlatformReturningState && record.value == 1.0f, "MovePlatform backwards on a type 1 platform: state 3");
}

void resetMessage() {
    PlatformRecord lift;
    lift.motion.type = 1;
    lift.motion.origin = {966.705f, 30.0791f, 1010.82f};
    lift.value = 1.0f;
    lift.requested = kPlatformArrivedState;
    require(platformReset(lift) == lift.motion.origin && lift.value == 0.0f && lift.requested == kPlatformStoppedState,
            "message 8: types 1 to 6 request 0, the value is 0 and the node goes to the origin");
    PlatformRecord continuous;
    continuous.motion.type = 0;
    platformReset(continuous);
    require(continuous.requested == kPlatformMovingState, "message 8: type 0 requests 2");
    PlatformRecord steps;
    steps.motion.type = 7;
    steps.requested = kPlatformControlledState;
    platformReset(steps);
    require(steps.requested == kPlatformControlledState, "message 8 leaves the requested state of type 7");
}

void groundEntity() {
    GroundEntityLink link;
    require(groundEntityRide(link, {1, 2, 3}) == PlatformVector{}, "no ground entity, no ride");
    groundEntityContact(link, "1-3_ledge_platform", {966.705f, 17.2241f, 1010.82f});
    require(link.owner == "1-3_ledge_platform" && link.remembered[1] == 17.2241f, "0x20b78c stores the platform and its translation");
    require(groundEntityRide(link, link.remembered) == PlatformVector{}, "a squared move of 0 is not ridden");
    const PlatformVector risen{966.705f, 17.3910f, 1010.82f};
    const PlatformVector move = groundEntityRide(link, risen);
    require(move[0] == 0.0f && move[1] == risen[1] - 17.2241f && move[2] == 0.0f, "the move is the translation now minus the remembered one");
    require(groundEntityRide(link, {966.705f, 22.2241f, 1010.82f}) == PlatformVector{}, "a squared move of exactly 25 is not ridden");
    require(groundEntityRide(link, {966.705f, 22.2f, 1010.82f})[1] > 4.9f, "a squared move just below 25 is ridden");
}

void orientedRide() {
    GroundEntityLink link;
    PlatformFrame before;
    before.translation = {10.0f, 0.0f, 0.0f};
    groundEntityContactOriented(link, "1-3_push_platform", before, {10.0f, 0.5f, 2.0f}, {0.0f, 0.0f, 1.0f});
    require(link.oriented && link.remembered == PlatformVector{0.0f, 0.5f, 2.0f} && link.localForward == PlatformVector{0.0f, 0.0f, 1.0f},
            "0x20b78c on an orient platform stores the position and the forward axis in the platform's frame");
    require(groundEntityRide(link, {11.0f, 0.0f, 0.0f}) == PlatformVector{}, "an orient platform is not ridden by its translation alone");
    PlatformFrame after;
    after.rows = {0, 0, -1, 0, 1, 0, 1, 0, 0};
    after.translation = {12.0f, 0.0f, 0.0f};
    const GroundEntityTurn ride = groundEntityRideOriented(link, after, {10.0f, 0.5f, 2.0f}, {0.0f, 0.0f, 1.0f});
    require(ride.move == PlatformVector{4.0f, 0.0f, -2.0f}, "0x2b01e4: the local position taken back to the world minus the position, y 0");
    require(ride.turn == 16384, "a quarter turn of the platform turns the character by 16384");
}

void riseWithoutProbe() {
    CollisionWorld world;
    chase::assets::CollisionItem floor;
    floor.vertices = {-10, 0, -10, 10, 0, -10, 0, 0, 10};
    chase::assets::CollisionTriangle triangle;
    triangle.normal[1] = 1.0f;
    triangle.vertex[0] = 0;
    triangle.vertex[1] = 1;
    triangle.vertex[2] = 2;
    floor.triangles.push_back(triangle);
    chase::assets::CollisionNode root;
    root.half[0] = root.half[2] = 10.0f;
    root.half[1] = 0.1f;
    root.info = 1u | (1u << 1);
    floor.nodes.push_back(root);
    world.add(floor);

    LocomotionState motion;
    VerticalState vertical;
    vertical.y = 0.0f;
    vertical.groundEntityMove[1] = 0.167f;
    const PlayerMotionStep step = playerMotionUpdate(motion, vertical, JumpProfiles{}, world, StickIntent{}, false, 1.0f / 30.0f);
    require(std::fabs(vertical.y - 0.167f) < 1e-6f && vertical.posture == Posture::Ground, "0x20cd30 raises the node by the ground entity's upward move");
    require(vertical.groundKept && !vertical.groundProbed, "a rise without horizontal movement runs no probe and keeps the ground entity");
    require(vertical.groundEntityMove[1] == 0.0f && !step.vertical.landed, "the move is taken once");

    vertical.y = 0.1f;
    vertical.groundEntityMove[1] = -0.05f;
    playerMotionUpdate(motion, vertical, JumpProfiles{}, world, StickIntent{}, false, 1.0f / 30.0f);
    require(vertical.groundProbed && vertical.groundFound && std::fabs(vertical.y) < 0.01f, "a downward move makes the standing character probe for the ground");
}

void turnedLeverZone() {
    SwitchObject sw;
    sw.position[0] = 967.116f;
    sw.position[1] = 17.42f;
    sw.position[2] = 1010.97f;
    const float yaw = 247.546f * 3.14159265f / 180.0f;
    sw.matrix = {std::cos(yaw), 0, -std::sin(yaw), 0, 0, 1, 0, 0, std::sin(yaw), 0, std::cos(yaw), 0, sw.position[0], sw.position[1], sw.position[2], 1};
    LeverPart lever;
    lever.hasZone = true;
    lever.zoneCentre[1] = 1.0f;
    lever.zoneCentre[2] = 1.4f;
    lever.zoneHalf[0] = 1.0f;
    lever.zoneHalf[1] = 1.5f;
    lever.zoneHalf[2] = 1.0f;
    sw.lever = lever;
    const float plate[3] = {966.185f, 17.463f, 1010.26f};
    require(SwitchSet::inZone(sw, plate), "the plate of the lift (oracle save state) is inside the turned UseBound of 1-3_Ledge_Platform_Switch");
    const float unturned[3] = {967.116f, 17.463f, 1012.37f};
    require(!SwitchSet::inZone(sw, unturned), "the unturned zone centre is outside the turned zone");
    sw.carried[1] = 12.855f;
    const float top[3] = {966.185f, 30.318f, 1010.26f};
    require(SwitchSet::inZone(sw, top) && !SwitchSet::inZone(sw, plate), "the zone follows the node the platform carries");
}

}

int main() {
    try {
        speedNative();
        originTargetAndMove();
        resetMessage();
        groundEntity();
        orientedRide();
        riseWithoutProbe();
        turnedLeverZone();
    } catch (const std::exception& error) {
        std::cerr << "platform-riding-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "platform-riding-test: ok\n";
    return 0;
}
