#include "runtime/traffic_driver_states.hpp"

#include "runtime/traffic_driver_connect.hpp"
#include "runtime/traffic_route.hpp"
#include "runtime/traffic_route_functions.hpp"
#include "runtime/traffic_route_mover.hpp"
#include "runtime/vehicle_chase_events.hpp"
#include "runtime/vehicle_drive.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

std::uint32_t floatWord(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

AiVec3 transformPoint(const AiVec3& point, const AiMatrix& m) {
    AiVec3 out{};
    for (int i = 0; i < 3; ++i) {
        float rotated = point[1] * m[1][i];
        rotated += point[2] * m[2][i];
        float moved = m[3][i];
        moved += point[0] * m[0][i];
        out[i] = rotated + moved;
    }
    return out;
}

constexpr float kVehicleBrakeReleaseSpeed = 0.1f;  // 0x230568

}

void requestTrafficDriverState(TrafficDrivers& drivers, std::size_t car, std::uint8_t state) {
    drivers.mutableCar(car).driver.requestedState = state;
}

void setVehicleThrowObjects(AiDriverRecord* driver, float flag) {
    if (driver == nullptr) return;
    if (floatWord(flag) != 0)
        driver->flags = static_cast<std::uint8_t>(driver->flags | kAiDriverFlagEffect);
    else
        driver->flags = static_cast<std::uint8_t>(driver->flags & ~kAiDriverFlagEffect);
}

AiVec3 vehicleThrowTarget(const AiVec3& local, const AiMatrix& node) { return transformPoint(local, node); }

AiVec3 vehicleThrowLaunch(const AiVec3& from, const AiVec3& to, float speed, float gravity) {
    float x = to[0] - from[0];
    float z = to[2] - from[2];
    const float dy = to[1] - from[1];
    float squared = x * x;
    squared += z * z;
    const float h = std::sqrt(squared);
    if (floatWord(h) <= floatWord(kVehicleThrowShortest)) {
        x = 0.0f;
        z = 1.0f;
    } else {
        const float inverse = 1.0f / h;
        x = x * inverse;
        z = z * inverse;
    }
    const float twice = 2.0f;
    float tangent = dy / h + gravity * h / (speed * speed * twice);
    if (static_cast<std::int32_t>(floatWord(tangent)) > static_cast<std::int32_t>(floatWord(kVehicleThrowTangentLimit))) {
        if (dy < h) speed = std::sqrt(gravity * h * h / ((h - dy) * twice));
        tangent = kVehicleThrowTangentLimit;
    }
    return {x * speed, speed * tangent, z * speed};
}

VehicleThrowResult throwVehicleObject(std::vector<VehicleThrowObject>& objects, const AiVec3& target, std::uint32_t car, std::uint32_t carParent,
                                      const AiMatrix& node, float scale) {
    VehicleThrowResult result;
    for (std::size_t index = 0; index < objects.size(); ++index) {
        VehicleThrowObject& object = objects[index];
        if (!object.disabled) continue;
        object.disabled = false;
        object.frozen = false;
        object.timer = static_cast<std::uint8_t>(object.maxTime & 0x7f);
        object.node = node;
        const AiVec3 from = transformPoint(object.centre, node);
        object.velocity = vehicleThrowLaunch(from, target, object.speed * scale, object.gravity);
        if (object.parent == 0 || object.parent != carParent) {
            object.parent = carParent;
            result.relinked = true;
        }
        object.owner = car;
        result.thrown = index;
        return result;
    }
    return result;
}

BodyInputs copyVehicleRecordInputs(AiDriverVehicle& vehicle, std::uint16_t vehicleState, int gear, float bodySpeed) {
    if (vehicleState == kVehicleStateHandBrake || vehicleState == kVehicleStateParked || vehicleState == kVehicleStateParkedHandBrake)
        vehicle.handBrakeInput = 1.0f;
    if (vehicle.forcedStop != 0) {
        vehicle.handBrakeInput = 1.0f;
        vehicle.accelerateInput = 0.0f;
        vehicle.brakeInput = 1.0f;
        if (gear == 0 || !(bodySpeed > kVehicleBrakeReleaseSpeed)) vehicle.brakeInput = 0.0f;
    }
    BodyInputs inputs;
    inputs.steer = vehicle.steerInput;
    inputs.accelerate = vehicle.accelerateInput;
    inputs.brake = vehicle.brakeInput;
    inputs.handBrake = vehicle.handBrakeInput;
    vehicle.steerInput = 0.0f;
    vehicle.accelerateInput = 0.0f;
    vehicle.brakeInput = 0.0f;
    vehicle.handBrakeInput = 0.0f;
    return inputs;
}

DamageMessage trafficDestroyMessage() {
    DamageMessage message;
    message.hearts = kAiDestroyHearts;
    return message;
}

TrafficStoppingEntry enterTrafficStopping(std::uint8_t& forcedStop, bool globalObject) {
    forcedStop = 1;
    TrafficStoppingEntry entry;
    entry.event = kVehicleChaseEndedEvent;
    entry.globalEvent = globalObject;
    entry.smokeEffect = kVehicleSmokeLastHeart;
    return entry;
}

void connectTrafficDriverStates(AiDriverHooks& hooks, const TrafficStateContext& context) {
    if (context.driver == nullptr || context.vehicle == nullptr || context.steering == nullptr || context.node == nullptr || context.events == nullptr)
        throw std::logic_error("traffic driver states: the context lacks the driver, vehicle, steering, node or event list");
    const TrafficStateContext c = context;
    hooks.steerToward = [c](const AiMatrix& target, float dt) {
        TrafficSteerFrame frame;
        frame.node = *c.node;
        frame.dt = dt;
        frame.forcedStop = c.vehicle->forcedStop != 0;
        frame.gearSpeed = c.gearSpeed;
        const TrafficSteerResult result = steerTrafficCar(*c.driver, *c.steering, target, frame);
        for (const AiMatrix& placed : result.placements) {
            *c.node = placed;
            if (c.moved) *c.moved = true;
            TrafficStateEvent event;
            event.kind = TrafficStateEventKind::Placed;
            event.matrix = placed;
            c.events->push_back(event);
        }
        c.vehicle->steerInput = result.steer;
        if (result.accelerate) c.vehicle->accelerateInput = *result.accelerate;
        if (result.brake) c.vehicle->brakeInput = *result.brake;
    };
    hooks.copyInputs = [c] {
        if (c.copyInputs) c.copyInputs();
    };
    hooks.normalGravity = [c] {
        if (c.normalGravity) c.normalGravity();
    };
    hooks.wakeBody = [c] {
        if (c.wakeBody) c.wakeBody();
    };
    hooks.setListed = [c](bool on) {
        if (c.chaseMarkers) c.chaseMarkers(on);
    };
    hooks.steerStep = [c] { runTrafficContactStep(*c.driver, (*c.node)[3], c.radius, c.contacts); };
    hooks.enterStopping = [c] {
        TrafficStateEvent event;
        event.kind = TrafficStateEventKind::Stopping;
        event.stopping = enterTrafficStopping(c.vehicle->forcedStop, c.globalObject);
        c.events->push_back(event);
    };
    hooks.destroy = [c] {
        TrafficStateEvent event;
        event.kind = TrafficStateEventKind::Destroy;
        event.message = trafficDestroyMessage();
        c.events->push_back(event);
    };
    hooks.rearEffect = [c](const AiVec3& local) {
        TrafficStateEvent event;
        event.kind = TrafficStateEventKind::Throw;
        event.target = vehicleThrowTarget(local, *c.node);
        c.events->push_back(event);
    };
}

namespace {

AiDriverHooks scriptCarHooks(const ScriptCarDriver& car, const ScriptCarFrame& frame, AiMatrix& node, bool& moved, std::vector<TrafficStateEvent>& events) {
    if (car.driver == nullptr || car.vehicle == nullptr || car.steering == nullptr || car.lights == nullptr || car.kinematic == nullptr ||
        car.bodyInputs == nullptr)
        throw std::logic_error("script car driver: a part of the car is missing");
    TrafficStateContext context;
    context.driver = car.driver;
    context.vehicle = car.vehicle;
    context.steering = car.steering;
    context.node = &node;
    context.moved = &moved;
    context.gearSpeed = frame.gearSpeed;
    context.radius = frame.radius;
    context.globalObject = frame.globalObject;
    AiDriverVehicle* vehicle = car.vehicle;
    BodyInputs* inputs = car.bodyInputs;
    const std::uint16_t state = frame.vehicleState;
    const int gear = frame.gear;
    const float speed = frame.bodySpeed;
    context.copyInputs = [vehicle, inputs, state, gear, speed] { *inputs = copyVehicleRecordInputs(*vehicle, state, gear, speed); };
    context.chaseMarkers = frame.chaseMarkers;
    context.contacts.gather = [](const AiVec3&, const AiVec3&, const TrafficContactDescriptor&, unsigned) { return std::uint32_t(0); };
    context.contacts.slide = [](const AiVec3&, std::uint32_t) {};
    context.events = &events;
    AiDriverHooks hooks;
    connectTrafficDriverStates(hooks, context);
    bool* kinematic = car.kinematic;
    hooks.setKinematic = [kinematic](bool on) { *kinematic = on; };
    hooks.snapTarget = frame.snapTarget;
    hooks.setNodeMatrix = [&node, &moved](const AiMatrix& matrix) {
        node = matrix;
        moved = true;
    };
    const AiRouteFunctions* routes = frame.routes;
    const AiVec3 position = frame.node[3];
    hooks.chooseNextRoute = [routes, position](AiDriverRecord& driver, std::uint32_t& next) {
        TrafficJunctionWorld none;
        GameRandom unused(1);
        return chooseTrafficJunction(driver, {0, position, 0.0f, routes}, none, unused, next);
    };
    hooks.leaveTraffic = [] {};
    hooks.stopAll = [] { return false; };
    TrafficBrakeLights* lights = car.lights;
    hooks.lights = [lights](bool stopped, float dt) {
        TrafficBrakeLightFrame light;
        light.brake = stopped ? 1.0f : 0.0f;
        light.dt = dt;
        runTrafficBrakeLights(*lights, light);
    };
    hooks.nodeHidden = [] { return true; };
    return hooks;
}

}

AiRouteFunctions levelPathRouteFunctions(const assets::Level& level) {
    const auto path = [&level](std::uint32_t route) -> const assets::LevelPath& {
        for (const assets::LevelPath& candidate : level.paths)
            if (candidate.nameHash == route) return candidate;
        throw std::invalid_argument("script car route " + std::to_string(route) + " is no path of the level");
    };
    AiRouteFunctions functions;
    functions.evaluate = [path](std::uint32_t route, float distance, AiVec3* position, AiVec3* direction) {
        const TrafficRouteSample sample = sampleTrafficRoute(path(route), distance);
        if (position) *position = sample.position;
        if (direction) *direction = sample.tangent;
    };
    functions.ahead = [path](std::uint32_t route, float ahead, float step, float from) { return trafficRouteAhead(path(route), ahead, step, from); };
    functions.nearest = [path](std::uint32_t route, const AiVec3& position, unsigned samples, float scale) {
        return trafficRouteNearest(path(route), position, samples, scale);
    };
    functions.length = [path](std::uint32_t route) { return static_cast<std::uint16_t>(path(route).points.size()); };
    functions.signal = [](std::uint32_t) { return 0u; };
    functions.follow = [](const AiVec3&, AiMatrix&, AiVec3&, std::uint32_t&, float&) { return false; };
    return functions;
}

ScriptCarStep runScriptCarDriver(const ScriptCarDriver& car, const ScriptCarFrame& frame, GameRandom& random) {
    ScriptCarStep step;
    AiMatrix node = frame.node;
    bool moved = false;
    const AiDriverHooks hooks = scriptCarHooks(car, frame, node, moved, step.events);
    AiDriverInputFrame input;
    input.node = node;
    input.halfExtents = frame.halfExtents;
    input.dt = frame.dt;
    input.playerInVehicle = frame.playerInVehicle;
    input.gearSpeed = frame.gearSpeed;
    input.routes = frame.routes;
    runAiDriverInput(*car.driver, *car.vehicle, input, hooks);
    AiDriverStateFrame change;
    change.node = node;
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) change.body[column][row] = node[row][column];
    change.body[3] = node[3];
    change.bodySpeed = frame.bodySpeed;
    change.routes = frame.routes;
    runAiDriverStateChange(*car.driver, *car.vehicle, change, hooks, random);
    if (moved) step.node = node;
    return step;
}

ScriptCarStep runScriptCarPostStep(const ScriptCarDriver& car, const ScriptCarFrame& frame, GameRandom& random) {
    ScriptCarStep step;
    AiMatrix node = frame.node;
    bool moved = false;
    const AiDriverHooks hooks = scriptCarHooks(car, frame, node, moved, step.events);
    AiDriverPostFrame post;
    post.node = node;
    post.dt = frame.dt;
    post.bodySpeed = frame.bodySpeed;
    post.routes = frame.routes;
    runAiDriverPostStep(*car.driver, *car.vehicle, post, hooks, random);
    if (moved) step.node = node;
    return step;
}

}
