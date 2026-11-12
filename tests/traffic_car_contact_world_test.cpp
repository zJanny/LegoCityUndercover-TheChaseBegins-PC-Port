// Tests of src/runtime/traffic_car_contact_world.*: the kinematic traffic body in the vehicle world step and the driver's part of the contact
// message 0x80000002 (0x1797c4 at 0x179cec..0x179db0). The branch's expected values follow from the disassembly (llvm-objdump
// --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261112-traffic-A on 2026-11-12, docs/re/traffic-car-body.md);
// the cmp / strb / tst sequence is evaluated by hand. The bodies of the world test are made up (the trooper's record and box of
// tests/vehicle_trooper.hpp, an overlap of 0.45 m across the width, no ground); what the solver answers is the port's own solver, so the test
// pins only what the rules say: the kinematic body stays where it is, the dynamic body is pushed away and each side gets its contact message
// with the other side's kind. No emulator was used.
#include "runtime/traffic_car_contact_world.hpp"

#include "runtime/vehicle_wheel_pose.hpp"
#include "runtime/vehicle_world.hpp"
#include "vehicle_trooper.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

void testOrientation() {
    const float c = std::cos(0.7f), s = std::sin(0.7f);
    const float pc = std::cos(0.2f), ps = std::sin(0.2f);
    const std::array<Vector3, 3> axes = {Vector3{c, 0.0f, -s}, Vector3{s * ps, pc, c * ps}, Vector3{s * pc, -ps, c * pc}};
    VehicleRigidBody rigid;
    rigid.orientation = orientationFromAxes(axes);
    const std::array<Vector3, 3> back = bodyAxes(rigid);
    for (std::size_t a = 0; a < 3; ++a)
        for (std::size_t k = 0; k < 3; ++k) require(near(back[a][k], axes[a][k], 1e-5f), "orientationFromAxes round trip");
    const std::array<Vector3, 3> half = {Vector3{-1.0f, 0.0f, 0.0f}, Vector3{0.0f, 1.0f, 0.0f}, Vector3{0.0f, 0.0f, -1.0f}};
    rigid.orientation = orientationFromAxes(half);
    const std::array<Vector3, 3> turned = bodyAxes(rigid);
    for (std::size_t a = 0; a < 3; ++a)
        for (std::size_t k = 0; k < 3; ++k) require(near(turned[a][k], half[a][k]), "half turn");
}

AiMatrix nodeAt(float x, float y, float z) { return {AiVec3{1, 0, 0}, AiVec3{0, 1, 0}, AiVec3{0, 0, 1}, AiVec3{x, y, z}}; }

TrafficBodyState bodyAtNode(const AiMatrix& node) {
    TrafficBodyState body;
    body.centreOfMass = {0.0f, -0.75f, 0.0f};
    placeTrafficBodyAtNode(body, node);
    body.rigidFlags = kRigidFlagBuilt | kRigidFlagKinematic;
    return body;
}

void testKinematicRigidBody() {
    TrafficBodyState body = bodyAtNode(nodeAt(10.0f, 2.0f, 5.0f));
    body.velocity = {0.0f, 0.0f, 8.0f};
    const VehicleRigidBody rigid = trafficKinematicRigidBody(body);
    require(near(rigid.position[0], 10.0f) && near(rigid.position[1], 1.25f) && near(rigid.position[2], 5.0f), "centre of mass image of the node");
    require(near(rigid.velocity[2], 8.0f), "stored velocity");
    require(rigid.inverseMass == 0.0f && rigid.inverseInertia[0] == 0.0f && rigid.inverseInertia[1] == 0.0f && rigid.inverseInertia[2] == 0.0f, "no inverse mass");
    require(near(rigid.orientation[3], 1.0f), "identity orientation");
}

struct Called {
    int now = 0, queued = 0, left = 0;
    std::uint32_t signal = 0;
};

TrafficCarContactHooks hooksFor(Called& called) {
    TrafficCarContactHooks hooks;
    hooks.dynamicNow = [&called] { ++called.now; };
    hooks.dynamicQueued = [&called] { ++called.queued; };
    hooks.leaveSignalQueue = [&called](std::uint32_t signal) {
        ++called.left;
        called.signal = signal;
    };
    return hooks;
}

void testContactMessage() {
    TrafficCarContactMessage message;
    message.otherEntity = true;
    message.now = 12.5f;
    for (const std::uint8_t state : {kAiStateRoute, kAiStateJunction}) {
        AiDriverRecord driver;
        driver.state = state;
        driver.signal = 7;
        driver.flags = 1 | 8;
        Called called;
        trafficCarContact(driver, true, message, hooksFor(called));
        require(driver.requestedState == 4, "state 2 and 3 request state 4");
        require(called.queued == 1 && called.now == 0, "a kinematic body goes dynamic by the queue");
        require(called.left == 1 && called.signal == 7, "bit 3 leaves the junction queue of the signal");
        require(driver.contactTime == 12.5f, "contact time stamped");
    }
    {
        AiDriverRecord driver;
        driver.state = kAiStateRoute;
        Called called;
        TrafficCarContactMessage flagged = message;
        flagged.otherBodyFlag20000 = true;
        trafficCarContact(driver, true, flagged, hooksFor(called));
        require(called.now == 1 && called.queued == 0, "flag 0x20000 of the other body: dynamic at once");
        require(called.left == 0, "no queue without bit 3");
    }
    {
        AiDriverRecord driver;
        driver.state = kAiStateRoute;
        Called called;
        trafficCarContact(driver, false, message, hooksFor(called));
        require(called.queued == 0 && called.now == 0 && driver.requestedState == 4, "a dynamic body is not switched again");
    }
    {
        AiDriverRecord driver;
        driver.state = kAiStateRoute;
        driver.contactTime = 3.0f;
        Called called;
        TrafficCarContactMessage floorContact;
        floorContact.now = 9.0f;
        trafficCarContact(driver, true, floorContact, hooksFor(called));
        require(driver.requestedState == 4 && called.queued == 0 && driver.contactTime == 3.0f, "a contact without another entity: only the request");
    }
    for (const std::uint8_t state : {std::uint8_t(0), std::uint8_t(1), std::uint8_t(4), std::uint8_t(5), std::uint8_t(8)}) {
        AiDriverRecord driver;
        driver.state = state;
        driver.requestedState = state;
        driver.flags = 8;
        Called called;
        trafficCarContact(driver, true, message, hooksFor(called));
        require(driver.requestedState == state && called.queued == 0 && called.left == 0, "other states leave the request alone");
        require(driver.contactTime == 12.5f, "other states still stamp the contact time");
    }
}

void testDynamicNow() {
    TrafficBodyState body = bodyAtNode(nodeAt(0.0f, 0.0f, 0.0f));
    body.massless = true;
    body.group = kTrafficKinematicGroup;
    body.mask = kTrafficKinematicMask;
    body.queue = {TrafficBodyCommand::Remove};
    makeTrafficBodyDynamicNow(body, -39.0f);
    require((body.rigidFlags & kRigidFlagKinematic) == 0 && !body.massless, "dynamic at once");
    require(body.group == kTrafficDynamicGroup && body.mask == kTrafficDynamicMask, "dynamic filter");
    require(body.queue.size() == 1 && body.queue[0] == TrafficBodyCommand::Remove, "the waiting queue is kept");
}

void testFields() {
    VehicleRecord record;
    record.healthPoints = 40;
    record.hearts = 3;
    record.health = 5;
    record.forcedStop = 1;
    AiDriverVehicle driver;
    driver.steerInput = 0.5f;
    driverFieldsFromRecord(driver, record);
    require(driver.health == 40 && driver.hearts == 3 && driver.maxHearts == 5 && driver.forcedStop == 1, "the record's fields reach the driver");
    require(driver.steerInput == 0.5f, "the inputs stay in the driver's copy");
    driver.health = 100;
    driver.hearts = 5;
    driver.forcedStop = 0;
    driver.maxHearts = 9;
    recordFieldsFromDriver(record, driver);
    require(record.healthPoints == 100 && record.hearts == 5 && record.forcedStop == 0 && record.health == 5, "the driver's writes reach the record, the table row's hearts stay");
}

void testWheelPose() {
    const chase::assets::Matrix node = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 0, 5, 1};
    const chase::assets::Matrix right = kinematicWheelPoseMatrix(1.5707963f, 0.85f, {0.85f, 0.07f, 1.5f}, node);
    require(near(right[4], 0.0f) && near(right[5], 0.0f, 1e-6f) && near(right[6], 1.0f), "a quarter turn about x tips the up row onto forward");
    require(near(right[12], 10.85f) && near(right[13], 0.07f) && near(right[14], 6.5f), "the node's local position is kept");
    const chase::assets::Matrix left = kinematicWheelPoseMatrix(1.5707963f, -0.85f, {-0.85f, 0.07f, 1.5f}, node);
    require(near(left[0], -1.0f, 1e-6f) && near(left[5], 0.0f, 1e-6f) && near(left[6], 1.0f, 1e-5f), "a negative mount x turns half about y and spins the other way");
    require(near(left[12], 9.15f), "left wheel position");
    const chase::assets::Matrix still = kinematicWheelPoseMatrix(0.0f, 0.85f, {0.0f, 0.0f, 0.0f}, node);
    require(near(still[0], 1.0f) && near(still[5], 1.0f) && near(still[10], 1.0f), "no spin keeps the node's axes");
}

void testWorldStep() {
    const VehicleRecord record = trooper::record();
    Vector3 low, high;
    trooper::box(low, high);
    const std::vector<BulletBox> boxes;
    chase::assets::CollisionItem item;
    item.floor.emplace();
    VehicleRigidBody player = vehicleRigidBody(record, {0.0f, 0.95f, 0.0f}, 0.0f, low, high, kVehicleBodyMassFactor);
    const TrafficBodyState traffic = bodyAtNode(nodeAt(1.5f, 0.95f, 0.0f));
    VehicleRigidBody parked = trafficKinematicRigidBody(traffic);
    const Vector3 parkedAt = parked.position;

    DrivetrainState playerDrive = vehicleDrivetrain(record), trafficDrive = vehicleDrivetrain(record);
    VehicleBoxContacts playerShape = vehicleBoxContacts(low, high, &boxes), trafficShape = vehicleBoxContacts(low, high, &boxes);
    VehicleFloor playerFloor{FloorMesh(item), FloorTracker{}, vehicleCollisionBody(record, low, high, kVehicleBodyMassFactor), &playerShape};
    VehicleFloor trafficFloor{FloorMesh(item), FloorTracker{}, vehicleCollisionBody(record, low, high, 1.0f), &trafficShape};
    bool playerAwake = true, trafficAwake = true;
    BodyInputs held;
    held.driver = true;
    BodyInputs idle;
    idle.driver = false;

    std::vector<VehicleWorldBody> bodies(2);
    bodies[0] = {0, &player, &playerDrive, &playerAwake, &record, held, &playerFloor, nullptr};
    bodies[1] = {1, &parked, &trafficDrive, &trafficAwake, &record, idle, &trafficFloor, nullptr};
    bodies[1].kinematic = true;
    bodies[0].playerVehicle = true;
    int playerHits = 0, trafficHits = 0;
    bool trafficSawPlayer = false, playerSawEntity = false;
    bodies[0].impact = [&](float, const ContactOther& other, const Vector3&) {
        ++playerHits;
        playerSawEntity = other.entity && !other.playerVehicle;
    };
    bodies[1].impact = [&](float, const ContactOther& other, const Vector3&) {
        ++trafficHits;
        trafficSawPlayer = other.entity && other.playerVehicle;
    };
    VehiclePairManifolds pairs;
    const WheelRay ray = trooper::flatGround;
    stepVehicleBodies(bodies, pairs, ray, 1);

    require(trafficHits > 0 && playerHits > 0, "both sides get their contact message");
    require(trafficSawPlayer, "the traffic body sees the player's car as the other entity");
    require(playerSawEntity, "the player's car sees an entity that is not the player's car");
    require(parked.position == parkedAt && parked.velocity[0] == 0.0f && parked.velocity[2] == 0.0f, "the kinematic body does not move");
    require(player.velocity[0] < -0.01f, "the player's car is pushed away from the traffic body");
    require(trafficAwake, "a kinematic body stays awake");

    std::vector<VehicleWorldBody> twoParked(2);
    VehicleRigidBody second = parked;
    second.position[0] += 0.2f;
    bool secondAwake = true;
    twoParked[0] = {0, &parked, &trafficDrive, &trafficAwake, &record, idle, &trafficFloor, nullptr};
    twoParked[1] = {1, &second, &trafficDrive, &secondAwake, &record, idle, &trafficFloor, nullptr};
    twoParked[0].kinematic = twoParked[1].kinematic = true;
    int bumps = 0;
    twoParked[0].impact = twoParked[1].impact = [&bumps](float, const ContactOther&, const Vector3&) { ++bumps; };
    VehiclePairManifolds none;
    stepVehicleBodies(twoParked, none, ray, 1);
    require(bumps == 0, "two kinematic bodies share group 2 and do not collide");
}

}

int main() {
    try {
        testOrientation();
        testKinematicRigidBody();
        testContactMessage();
        testDynamicNow();
        testFields();
        testWheelPose();
        testWorldStep();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic car contact world: ok\n";
    return 0;
}
