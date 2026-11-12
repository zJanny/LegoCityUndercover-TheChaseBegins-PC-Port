// Tests of src/runtime/traffic_car_body.*: the kinematic switch 0x213c74, the world membership 0x243300, the body commands 0, 1, 3 and 4
// of 0x2e3390, the wake 0x2308e4, the node copy 0x20cc30 with its test in the world update 0x2e2bc8, and the kinematic branch of the body
// step 0x33aea8 with the cell tracker 0x20c0e4. Every expected value follows from the disassembly (llvm-objdump --triple=armv6k-none-eabi
// --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261111-chase-A on 2026-11-11, docs/re/traffic-car-body.md), evaluated by hand
// in single precision. The rigid flags 0x182 of a traffic car out of the world and 0x102 of a live one are also the values recorded from
// save states by earlier rounds (docs/re/traffic-ai-driver.md §2026-10-25T12:26:09, docs/re/vehicles-physics-props.md
// §2026-10-16T09:53:49). The floor is made up. No emulator was used.
#include "runtime/traffic_car_body.hpp"

#include "runtime/pedestrian_manager.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;
using chase::assets::CollisionItem;
using chase::assets::CollisionTriangle;
using chase::assets::FloorCell;

constexpr float kGravity = -39.0f;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) <= 1e-5f; }

template <typename Function>
bool throwsLogicError(Function function) {
    try {
        function();
    } catch (const std::logic_error&) {
        return true;
    }
    return false;
}

AiMatrix nodeAt(const AiVec3& position, const AiVec3& forward) { return pedestrianPlacement(position, forward); }

TrafficCarBodies::Car car() {
    TrafficCarBodies::Car car;
    car.centreOfMass = {0.0f, -0.8f, 0.25f};
    car.flags = kTrafficBodyFlagListed | kTrafficBodyFlag8000;
    TrafficBodyWheel left;
    left.radius = 0.5f;
    left.mountX = -0.9f;
    left.mountY = 0.3f;
    left.restY = 0.1f;
    TrafficBodyWheel right = left;
    right.mountX = 0.9f;
    right.radius = 0.25f;
    car.wheels = {left, right};
    return car;
}

CollisionItem floorStrip(std::size_t count) {
    CollisionItem item;
    item.name = "collision_floor";
    item.floor.emplace();
    for (std::size_t k = 0; k < count; ++k) {
        const float x = 10.0f * float(k);
        for (const float v : {x, 0.0f, 0.0f, x, 0.0f, 1.0f, x + 1.0f, 0.0f, 0.0f}) item.vertices.push_back(v);
        CollisionTriangle triangle;
        triangle.vertex[0] = std::uint32_t(3 * k);
        triangle.vertex[1] = std::uint32_t(3 * k + 1);
        triangle.vertex[2] = std::uint32_t(3 * k + 2);
        item.triangles.push_back(triangle);
        FloorCell cell;
        cell.itemTriangle = true;
        cell.triangle = std::uint32_t(k);
        item.floor->cells.push_back(cell);
    }
    return item;
}

struct Bodies {
    TrafficCarBodies bodies;
    TrafficCarBodies::World world;
    AiMatrix node = nodeAt({10.0f, 5.0f, 20.0f}, {1.0f, 0.0f, 0.0f});
    int inputCopies = 0;

    Bodies() {
        world.node = [this](std::size_t) { return node; };
        bodies.load({car()}, {node}, kGravity);
    }
    TrafficBodyState& body() { return bodies.body(0); }
};

void loadLeavesAKinematicBodyOutOfTheWorld() {
    Bodies run;
    const TrafficBodyState& body = run.body();
    require(body.rigidFlags == 0x182u, "built 0x100, kinematic 2, out of the world 0x80");
    require(body.world == kTrafficBodyBuilt && body.queue.empty(), "0x243300 with 1 and then 0 leaves the wanted bit clear");
    require(body.flags == (kTrafficBodyFlagListed | kTrafficBodyFlagKinematic), "the switch sets 0x20 and clears 0x8000");
    require(body.gravity == 0.0f && body.activation == kRigidNeverSleeps && body.massless, "gravity 0, activation 4, mass 0");
    require(body.group == kTrafficKinematicGroup && body.mask == kTrafficKinematicMask, "command 4: group 2, mask 61");
    require(near(body.wheels[0].rayLength, 0.7f) && near(body.wheels[1].rayLength, 0.45f), "w+0xe0 = w+0x54 - w+0x60 + radius");
    require(near(body.rigid[3][0], 10.25f) && near(body.rigid[3][1], 4.2f) && near(body.rigid[3][2], 20.0f),
            "the origin is the node's image of the centre of mass: forward is +x here");
    require(body.rigid[2] == AiVec3({1.0f, 0.0f, 0.0f}) && body.matrix == body.rigid, "0x2429fc keeps the node's rotation and fills body +4");
}

void worldMembership() {
    Bodies run;
    TrafficBodyState& body = run.body();
    setTrafficBodyInWorld(body, true);
    setTrafficBodyInWorld(body, true);
    require(body.queue == std::vector<TrafficBodyCommand>({TrafficBodyCommand::Add}), "a second 0x243300 with 1 queues nothing");
    runTrafficBodyCommands(body, kGravity);
    require(body.rigidFlags == 0x102u && body.gravity == 0.0f, "command 0 clears 0x80 and leaves a kinematic body's gravity alone");
    setTrafficBodyInWorld(body, false);
    setTrafficBodyInWorld(body, false);
    require(body.queue == std::vector<TrafficBodyCommand>({TrafficBodyCommand::Remove}), "a second 0x243300 with 0 queues nothing");
    runTrafficBodyCommands(body, kGravity);
    require(body.rigidFlags == 0x182u, "command 1 sets 0x80");

    TrafficBodyState unbuilt;
    unbuilt.world = 0;
    setTrafficBodyInWorld(unbuilt, true);
    require(unbuilt.queue.empty() && unbuilt.world == kTrafficBodyWanted, "an unbuilt body only keeps the wanted bit");

    TrafficBodyState dynamic;
    dynamic.flags = kTrafficBodyFlagListed;
    setTrafficBodyInWorld(dynamic, true);
    runTrafficBodyCommands(dynamic, kGravity);
    require(dynamic.gravity == kGravity, "command 0 gives a listed dynamic body the global gravity");
    TrafficBodyState unlisted;
    setTrafficBodyInWorld(unlisted, true);
    runTrafficBodyCommands(unlisted, kGravity);
    require(unlisted.gravity == 0.0f && unlisted.rigidFlags == 0x100u, "not for a body without flag bit 0");
}

void switchOffAndOn() {
    Bodies run;
    TrafficBodyState& body = run.body();
    const AiMatrix elsewhere = nodeAt({50.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
    switchTrafficBodyKinematic(body, true, elsewhere, kGravity);
    require(body.queue.empty() && near(body.rigid[3][0], 10.25f), "a kinematic rigid body is neither moved nor queued again");

    switchTrafficBodyKinematic(body, false, elsewhere, kGravity);
    require(body.activation == kRigidActive && body.gravity == kGravity && (body.flags & kTrafficBodyFlagKinematic) == 0, "off: activation 1, gravity back");
    require(body.rigidFlags == 0x182u && body.queue == std::vector<TrafficBodyCommand>({TrafficBodyCommand::Dynamic}), "the rigid flags wait for the queue");
    require(stepTrafficBody(body, 1.0f / 30.0f, nullptr) == TrafficBodyStepKind::Kinematic, "until the queue ran the rigid body is kinematic");
    runTrafficBodyCommands(body, kGravity);
    require(body.rigidFlags == 0x180u && !body.massless && body.group == kTrafficDynamicGroup && body.mask == kTrafficDynamicMask,
            "command 3: flags 3 cleared, the mass back, group 1, mask 63");
    switchTrafficBodyKinematic(body, false, elsewhere, kGravity);
    require(body.queue.empty(), "off without the kinematic flag does nothing");

    switchTrafficBodyKinematic(body, true, elsewhere, kGravity);
    require(near(body.rigid[3][0], 50.0f) && near(body.rigid[3][2], 0.25f), "on: the transform is taken from the node");
    require(stepTrafficBody(body, 1.0f / 30.0f, nullptr) == TrafficBodyStepKind::Switching && body.wheelsOnGround == 4,
            "the kinematic flag without a kinematic rigid body: 4 wheels on the ground");
    runTrafficBodyCommands(body, kGravity);
    require(body.rigidFlags == 0x182u, "command 4");

    TrafficBodyState wide;
    wide.flags = kTrafficBodyFlagWideMask | kTrafficBodyFlagKinematic;
    wide.rigidFlags = 0x102u;
    switchTrafficBodyKinematic(wide, false, elsewhere, kGravity);
    runTrafficBodyCommands(wide, kGravity);
    require(wide.mask == kTrafficDynamicWideMask, "flag 0x40 gives the mask 0xffff");
    TrafficBodyState never;
    never.queue = {TrafficBodyCommand::Dynamic};
    runTrafficBodyCommands(never, kGravity);
    require(never.rigidFlags == 0x180u && never.group == kTrafficDynamicGroup, "command 3 leaves a dynamic body alone");
}

void wake() {
    TrafficBodyState body;
    body.deactivationTime = 0.5f;
    body.activation = kRigidSleeping;
    wakeTrafficBody(body);
    require(body.activation == kRigidActive && body.deactivationTime == 0.0f, "2 -> 1 and the time cleared");
    for (const std::uint32_t state : {kRigidNeverSleeps, kRigidNoSimulation}) {
        body.activation = state;
        body.deactivationTime = 0.5f;
        wakeTrafficBody(body);
        require(body.activation == state && body.deactivationTime == 0.0f, "4 and 5 stay, the time is cleared all the same");
    }
}

void followsNode() {
    Bodies run;
    TrafficBodyState& body = run.body();
    require(!trafficBodyFollowsNode(body, false, false), "a body out of the world is not in the world's object list");
    setTrafficBodyInWorld(body, true);
    runTrafficBodyCommands(body, kGravity);
    require(trafficBodyFollowsNode(body, false, false), "a kinematic body of the world");
    require(!trafficBodyFollowsNode(body, true, false) && !trafficBodyFollowsNode(body, false, true), "not with entity +0x10 bit 0 or bit 5");
    body.activation = kRigidSleeping;
    require(!trafficBodyFollowsNode(body, false, false), "not asleep");
    body.activation = kRigidNoSimulation;
    require(!trafficBodyFollowsNode(body, false, false), "not out of the simulation");
    body.activation = kRigidActive;
    body.rigidFlags = 0x100u;
    require(!trafficBodyFollowsNode(body, false, false), "not a dynamic body");
}

void kinematicStep() {
    Bodies run;
    TrafficBodyState& body = run.body();
    setTrafficBodyVelocity(body, {10.0f, 3.0f, 0.0f});
    const float dt = 1.0f / 30.0f;
    body.matrix = {};
    require(stepTrafficBody(body, dt, nullptr) == TrafficBodyStepKind::Kinematic, "the kinematic branch");
    require(body.matrix == body.rigid && body.forwardSpeed == 10.0f, "body +4 from the rigid body, +0xc0 = velocity . forward");
    require(body.wheels[0].spinRate == 20.0f && body.wheels[1].spinRate == 40.0f, "w+0x88 = forward speed / radius");
    require(body.wheels[0].spinAngle == 20.0f * dt && body.wheels[1].spinAngle == 40.0f * dt, "w+0x80 += rate x dt");
    require(body.wheelsOnGround == 0 && body.floor.cell == -1, "+0xda0 is not written; no floor graph, no tracker");
    stepTrafficBody(body, dt, nullptr);
    require(body.wheels[0].spinAngle == 20.0f * dt + 20.0f * dt, "the angle adds up");

    TrafficBodyState dynamic;
    require(stepTrafficBody(dynamic, dt, nullptr) == TrafficBodyStepKind::Dynamic && dynamic.wheelsOnGround == 0, "a dynamic body goes on to its own step");
}

void floorCell() {
    const CollisionItem strip = floorStrip(8);
    const FloorMesh mesh(strip);
    Bodies run;
    run.world.floor = &mesh;
    const TrafficBodyHooks hooks = run.bodies.hooks(run.world);
    require(hooks.floorCell(0) == -1, "no cell before a spawn");

    run.node = nodeAt({50.2f, 3.8f, 0.05f}, {0.0f, 0.0f, 1.0f});
    run.body().floor.last = {50.25f, 3.0f, 0.4f};
    run.body().velocity = {1.0f, 2.0f, 3.0f};
    run.body().wheels[0].spinRate = 4.0f;
    run.body().wheelsOnGround = 4;
    run.body().flags |= kTrafficBodyFlagsPlaced;
    run.bodies.spawn(0, 5, run.node, &mesh);
    const TrafficBodyState& body = run.body();
    require(near(body.rigid[3][0], 50.2f) && near(body.rigid[3][1], 3.0f) && near(body.rigid[3][2], 0.3f), "the transform from the placed node");
    require(hooks.floorCell(0) == 5 && near(body.floor.last[0], 50.2f) && near(body.floor.last[2], 0.3f),
            "the tracker of the placement walks from the last position with the spawn's cell and keeps the new position");
    require(body.velocity == AiVec3({0.0f, 0.0f, 0.0f}) && body.wheels[0].spinRate == 0.0f && body.wheelsOnGround == 0, "0x23091c clears the motion");
    require((body.flags & kTrafficBodyFlagsPlaced) == 0 && body.activation == kRigidNeverSleeps, "flags 0x88000 cleared; a kinematic body is not woken");

    run.bodies.spawn(0, -1, run.node, &mesh);
    require(hooks.floorCell(0) == 5, "a point without a cell (-1): the tracker searches with 0x1edaac at the body's position");
    run.bodies.spawn(0, -1, run.node, nullptr);
    require(hooks.floorCell(0) == -1, "no floor graph, no tracker");

    TrafficBodyState stepped = run.body();
    stepped.floor.cell = -1;
    stepTrafficBody(stepped, 1.0f / 30.0f, &mesh);
    require(stepped.floor.cell == 5 && near(stepped.floor.last[0], 50.2f), "the body step runs the same tracker");
}

void hooksReachTheBodies() {
    Bodies run;
    run.world.copyInputs = [&run](std::size_t) { ++run.inputCopies; };
    const TrafficBodyHooks hooks = run.bodies.hooks(run.world);
    require(hooks.kinematic(0), "rigid +0xcc bit 1");
    hooks.setActive(0, true);
    run.bodies.beforeWorldStep(run.world, [](std::size_t) { return false; });
    require(run.body().rigidFlags == 0x102u, "message 0x80000004 and the next world update");

    run.node = nodeAt({30.0f, 5.0f, 20.0f}, {1.0f, 0.0f, 0.0f});
    run.bodies.beforeWorldStep(run.world, [](std::size_t) { return true; });
    require(near(run.body().rigid[3][0], 10.25f), "a disabled entity's body keeps its transform");
    run.bodies.beforeWorldStep(run.world, [](std::size_t) { return false; });
    require(near(run.body().rigid[3][0], 30.25f), "the kinematic body takes the node before the world step");

    hooks.setVelocity(0, {6.0f, 0.0f, 0.0f});
    hooks.step(0, 0.5f);
    require(run.body().forwardSpeed == 6.0f && run.body().wheels[0].spinAngle == 6.0f, "the step with the mover's velocity");

    hooks.switchKinematic(0, false);
    require(hooks.kinematic(0), "the mover still sees a kinematic rigid body until the queue runs");
    hooks.normalGravity(0);
    hooks.wakeBody(0);
    run.bodies.beforeWorldStep(run.world, [](std::size_t) { return false; });
    require(!hooks.kinematic(0) && run.body().rigidFlags == 0x100u && run.body().gravity == kGravity, "dynamic after the queue");
    hooks.copyInputs(0);
    require(run.inputCopies == 1, "0x23019c is the caller's");
    hooks.setActive(0, false);
    run.bodies.beforeWorldStep(run.world, [](std::size_t) { return false; });
    require(run.body().rigidFlags == 0x180u, "message 0x80000003 and the next world update");

    Bodies bare;
    const TrafficBodyHooks unconnected = bare.bodies.hooks(bare.world);
    require(throwsLogicError([&] { unconnected.copyInputs(0); }), "the input copy of a dynamic car must be connected");
    TrafficCarBodies mismatch;
    require(throwsLogicError([&] { mismatch.load({car()}, {}, kGravity); }), "one node per car");
}

void massScales() {
    require(kPlayerVehicleMassScale == 2.0f && kChasedCarMassScale == 6.0f, "the literals 0x3449e4 and 0x3449e0 of the static initialiser 0x34498c");
}

}

int main() {
    try {
        loadLeavesAKinematicBodyOutOfTheWorld();
        worldMembership();
        switchOffAndOn();
        wake();
        followsNode();
        kinematicStep();
        floorCell();
        hooksReachTheBodies();
        massScales();
    } catch (const std::exception& error) {
        std::cerr << "traffic-car-body: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-car-body: ok\n";
    return 0;
}
