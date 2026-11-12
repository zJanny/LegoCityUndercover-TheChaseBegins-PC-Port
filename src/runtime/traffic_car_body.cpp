#include "runtime/traffic_car_body.hpp"

#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr float kWheelTwoPi = 6.2831854820251465f;  // literal 0x40c90fdb at 0x33b1e8

void runCommand(TrafficBodyState& body, TrafficBodyCommand command, float globalGravity) {
    switch (command) {
    case TrafficBodyCommand::Add:
        body.rigidFlags &= ~kRigidFlagOutOfWorld;
        if ((body.flags & kTrafficBodyFlagListed) != 0 && (body.rigidFlags & (kRigidFlagStatic | kRigidFlagKinematic)) == 0) body.gravity = globalGravity;
        return;
    case TrafficBodyCommand::Remove:
        body.rigidFlags |= kRigidFlagOutOfWorld;
        return;
    case TrafficBodyCommand::Dynamic:
        if ((body.rigidFlags & (kRigidFlagStatic | kRigidFlagKinematic)) == 0) return;
        body.rigidFlags &= ~(kRigidFlagStatic | kRigidFlagKinematic);
        body.massless = false;
        body.mask = (body.flags & kTrafficBodyFlagWideMask) != 0 ? kTrafficDynamicWideMask : kTrafficDynamicMask;
        body.group = kTrafficDynamicGroup;
        return;
    case TrafficBodyCommand::Kinematic:
        if ((body.rigidFlags & kRigidFlagKinematic) != 0) return;
        body.massless = true;
        body.rigidFlags = (body.rigidFlags & ~kRigidFlagStatic) | kRigidFlagKinematic;
        body.mask = kTrafficKinematicMask;
        body.group = kTrafficKinematicGroup;
        return;
    }
    throw std::logic_error("traffic body: unknown command " + std::to_string(static_cast<unsigned>(command)));
}

}

void setTrafficBodyInWorld(TrafficBodyState& body, bool on) {
    const bool wanted = (body.world & kTrafficBodyWanted) != 0;
    const bool built = (body.world & kTrafficBodyBuilt) != 0;
    if (on) {
        if (!wanted && built) body.queue.push_back(TrafficBodyCommand::Add);
        body.world = static_cast<std::uint8_t>(body.world | kTrafficBodyWanted);
        return;
    }
    if (wanted && built) body.queue.push_back(TrafficBodyCommand::Remove);
    body.world = static_cast<std::uint8_t>(body.world & ~kTrafficBodyWanted);
}

void switchTrafficBodyKinematic(TrafficBodyState& body, bool on, const AiMatrix& node, float globalGravity) {
    if (on) {
        body.gravity = 0.0f;
        body.activation = kRigidNeverSleeps;
        for (TrafficBodyWheel& wheel : body.wheels) wheel.rayLength = wheel.mountY - wheel.restY + wheel.radius;
        body.flags = (body.flags & ~kTrafficBodyFlag8000) | kTrafficBodyFlagKinematic;
        if ((body.rigidFlags & kRigidFlagKinematic) != 0) return;
        placeTrafficBodyAtNode(body, node);
        body.queue.push_back(TrafficBodyCommand::Kinematic);
        return;
    }
    if ((body.flags & kTrafficBodyFlagKinematic) == 0) return;
    body.activation = kRigidActive;
    body.gravity = globalGravity;
    body.flags &= ~kTrafficBodyFlagKinematic;
    body.queue.push_back(TrafficBodyCommand::Dynamic);
}

void runTrafficBodyCommands(TrafficBodyState& body, float globalGravity) {
    const std::vector<TrafficBodyCommand> queue = std::move(body.queue);
    body.queue.clear();
    for (const TrafficBodyCommand command : queue) runCommand(body, command, globalGravity);
}

void wakeTrafficBody(TrafficBodyState& body) {
    if (body.activation != kRigidNeverSleeps && body.activation != kRigidNoSimulation) body.activation = kRigidActive;
    body.deactivationTime = 0.0f;
}

void setTrafficBodyVelocity(TrafficBodyState& body, const AiVec3& velocity) {
    body.velocity = velocity;
}

void placeTrafficBodyAtNode(TrafficBodyState& body, const AiMatrix& node) {
    AiMatrix transform = node;
    for (std::size_t axis = 0; axis < 3; ++axis)
        transform[3][axis] = body.centreOfMass[0] * node[0][axis] + body.centreOfMass[1] * node[1][axis] + body.centreOfMass[2] * node[2][axis] + node[3][axis];
    body.rigid = transform;
    body.matrix = transform;
}

bool trafficBodyFollowsNode(const TrafficBodyState& body, bool entityDisabled, bool entityFlag20) {
    if ((body.rigidFlags & kRigidFlagOutOfWorld) != 0) return false;
    if (body.activation == kRigidSleeping || body.activation == kRigidNoSimulation) return false;
    if (entityDisabled || entityFlag20) return false;
    return (body.rigidFlags & kRigidFlagKinematic) != 0;
}

TrafficBodyStepKind stepTrafficBody(TrafficBodyState& body, float dt, const FloorMesh* floor) {
    body.matrix = body.rigid;
    body.forwardSpeed = body.matrix[2][0] * body.velocity[0] + body.matrix[2][1] * body.velocity[1] + body.matrix[2][2] * body.velocity[2];
    if (floor != nullptr) trackFloorCell(*floor, body.floor, body.rigid[3]);
    if ((body.rigidFlags & kRigidFlagKinematic) != 0) {
        for (TrafficBodyWheel& wheel : body.wheels) {
            wheel.spinRate = body.forwardSpeed / (wheel.radius * kWheelTwoPi) * kWheelTwoPi;
            wheel.spinAngle += wheel.spinRate * dt;
        }
        return TrafficBodyStepKind::Kinematic;
    }
    if ((body.flags & kTrafficBodyFlagKinematic) != 0) {
        body.wheelsOnGround = kTrafficSwitchingWheelsOnGround;
        return TrafficBodyStepKind::Switching;
    }
    return TrafficBodyStepKind::Dynamic;
}

void TrafficCarBodies::load(const std::vector<Car>& cars, const std::vector<AiMatrix>& nodes, float globalGravity) {
    if (cars.size() != nodes.size()) throw std::logic_error("traffic bodies: the cars and their nodes differ in number");
    globalGravity_ = globalGravity;
    bodies_.clear();
    for (std::size_t index = 0; index < cars.size(); ++index) {
        TrafficBodyState body;
        body.flags = cars[index].flags;
        body.centreOfMass = cars[index].centreOfMass;
        body.wheels = cars[index].wheels;
        body.gravity = globalGravity;
        placeTrafficBodyAtNode(body, nodes[index]);
        switchTrafficBodyKinematic(body, true, nodes[index], globalGravity);
        runTrafficBodyCommands(body, globalGravity);
        bodies_.push_back(body);
    }
}

TrafficBodyHooks TrafficCarBodies::hooks(const World& world) {
    TrafficBodyHooks hooks;
    hooks.switchKinematic = [this, &world](std::size_t car, bool on) { switchTrafficBodyKinematic(bodies_.at(car), on, world.node(car), globalGravity_); };
    hooks.wakeBody = [this](std::size_t car) { wakeTrafficBody(bodies_.at(car)); };
    hooks.normalGravity = [this](std::size_t car) { bodies_.at(car).gravity = globalGravity_; };
    hooks.copyInputs = [&world](std::size_t car) {
        if (!world.copyInputs) throw std::logic_error("traffic bodies: the input copy 0x23019c of a dynamic car is not connected");
        world.copyInputs(car);
    };
    hooks.step = [this, &world](std::size_t car, float dt) { stepTrafficBody(bodies_.at(car), dt, world.floor); };
    hooks.setActive = [this](std::size_t car, bool on) { setTrafficBodyInWorld(bodies_.at(car), on); };
    hooks.floorCell = [this](std::size_t car) { return static_cast<std::int32_t>(bodies_.at(car).floor.cell); };
    hooks.kinematic = [this](std::size_t car) { return (bodies_.at(car).rigidFlags & kRigidFlagKinematic) != 0; };
    hooks.setVelocity = [this](std::size_t car, const AiVec3& velocity) { setTrafficBodyVelocity(bodies_.at(car), velocity); };
    return hooks;
}

void TrafficCarBodies::spawn(std::size_t car, std::int32_t floorCell, const AiMatrix& node, const FloorMesh* floor) {
    TrafficBodyState& body = bodies_.at(car);
    body.floor.cell = floorCell;
    placeTrafficBodyAtNode(body, node);
    body.flags &= ~kTrafficBodyFlagsPlaced;
    body.velocity = {};
    for (TrafficBodyWheel& wheel : body.wheels) wheel.spinRate = 0.0f;
    body.wheelsOnGround = 0;
    if ((body.rigidFlags & (kRigidFlagStatic | kRigidFlagKinematic)) == 0) wakeTrafficBody(body);
    if (floor != nullptr) trackFloorCell(*floor, body.floor, body.rigid[3]);
}

void TrafficCarBodies::beforeWorldStep(const World& world, const std::function<bool(std::size_t car)>& skip) {
    for (std::size_t car = 0; car < bodies_.size(); ++car) {
        TrafficBodyState& body = bodies_[car];
        runTrafficBodyCommands(body, globalGravity_);
        const bool skipped = skip && skip(car);
        if (trafficBodyFollowsNode(body, skipped, false)) placeTrafficBodyAtNode(body, world.node(car));
    }
}

}
