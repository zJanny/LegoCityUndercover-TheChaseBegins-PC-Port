#include "runtime/traffic_driver_connect.hpp"
#include "runtime/traffic_car_push.hpp"

#include "runtime/traffic_route.hpp"
#include "runtime/traffic_route_mover.hpp"

#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

TrafficJunctionRecord& needJunction(TrafficJunctionWorld& world, std::uint32_t signal) {
    TrafficJunctionRecord* junction = trafficSignalJunction(world, signal);
    if (junction == nullptr) throw std::logic_error("traffic driver: unknown signal object " + std::to_string(signal));
    return *junction;
}

AiMatrix bodyBasis(const AiMatrix& node) {
    AiMatrix body{};
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) body[column][row] = node[row][column];
    body[3] = node[3];
    return body;
}

std::optional<std::uint32_t> routeIndex(const TrafficRouteTable& table, std::uint32_t route) {
    const std::optional<std::size_t> index = trafficRouteIndex(table, route);
    if (!index) return std::nullopt;
    return static_cast<std::uint32_t>(*index);
}

}

AiRouteFunctions trafficDriverRouteFunctions(const TrafficJunctions& junctions, const assets::PregenSpawns& spawns, const TrafficRouteTable& table) {
    AiRouteFunctions functions = trafficRouteFunctions(junctions);
    functions.follow = [&spawns, &table](const AiVec3& probe, AiMatrix& matrix, AiVec3& target, std::uint32_t& route, float& distance) {
        const TrafficRouteHit hit = findTrafficRoutePoint(trafficRouteCell(spawns, probe[0], probe[2]), table, {probe, matrix[2]});
        if (!hit.found) return false;
        target = hit.position;
        route = hit.route;
        distance = hit.parameter;
        return true;
    };
    return functions;
}

void TrafficDrivers::load(const std::vector<float>& heightOffsets) {
    cars_.clear();
    for (const float heightOffset : heightOffsets) {
        TrafficDriverCar car;
        AiDriverHooks hooks;
        hooks.setKinematic = [](bool) {};
        car.driver = createAiDriverRecord(heightOffset, AiMatrix{}, hooks);
        cars_.push_back(car);
    }
}

void TrafficDrivers::start(std::size_t car, std::uint32_t route, std::uint32_t signal, float parameter, const AiMatrix& node) {
    AiDriverRecord& driver = cars_.at(car).driver;
    driver.requestedState = kAiStateStart;
    driver.state = kAiStateHold;
    driver.flags = static_cast<std::uint8_t>(driver.flags | kAiDriverFlagAi);
    driver.route = route;
    driver.signal = signal;
    driver.routeDistance = parameter;
    driver.path = node;
}

void TrafficDrivers::leaveQueue(std::size_t car, TrafficJunctionWorld& junctions) {
    TrafficDriverCar& entry = cars_.at(car);
    if ((entry.driver.flags & kAiDriverFlagAi) != 0 && entry.driver.signal != 0)
        leaveTrafficJunctionQueue(needJunction(junctions, entry.driver.signal), entry.driver, static_cast<std::uint32_t>(car));
}

void TrafficDrivers::removed(std::size_t car, TrafficJunctionWorld& junctions) {
    TrafficDriverCar& entry = cars_.at(car);
    leaveQueue(car, junctions);
    TrafficRaceTrimCar trim;
    trim.entity = static_cast<std::uint32_t>(car + 1);
    trim.driver = &entry.driver;
    trim.steering = &entry.steering;
    releaseTrafficHeldCar(trim, held_);
}

void TrafficDrivers::removed(const std::vector<TrafficEvent>& events, TrafficJunctionWorld& junctions) {
    for (const TrafficEvent& event : events)
        if (event.kind == TrafficEventKind::Despawn) removed(event.car, junctions);
}

TrafficDriveResult TrafficDrivers::drive(TrafficManager& manager, TrafficJunctionWorld& world, const AiRouteFunctions& routes, const TrafficDriveStep& step,
                                         GameRandom& random) {
    std::vector<TrafficCar>& views = manager.cars();
    if (views.size() != cars_.size()) throw std::logic_error("traffic driver: the manager's cars and the driver records differ");
    TrafficDriveResult result;
    for (std::size_t index = 0; index < cars_.size(); ++index) {
        if (views[index].disabled) continue;
        const std::optional<TrafficCarBody> body = step.body ? step.body(index) : std::nullopt;
        if (!body) continue;
        TrafficDriverCar& car = cars_[index];
        const std::uint32_t entity = static_cast<std::uint32_t>(index);
        AiMatrix node = body->node;
        bool moved = false;

        std::vector<TrafficStateEvent> stateEvents;
        std::vector<TrafficContactEntry> contacts;
        TrafficStateContext states;
        states.driver = &car.driver;
        states.vehicle = &car.vehicle;
        states.steering = &car.steering;
        states.node = &node;
        states.moved = &moved;
        states.gearSpeed = body->gearSpeed;
        states.radius = body->radius;
        states.globalObject = step.globalObject;
        if (step.bodies.copyInputs) states.copyInputs = [&step, index] { step.bodies.copyInputs(index); };
        if (step.bodies.normalGravity) states.normalGravity = [&step, index] { step.bodies.normalGravity(index); };
        if (step.bodies.wakeBody) states.wakeBody = [&step, index] { step.bodies.wakeBody(index); };
        if (step.chaseMarkers) states.chaseMarkers = [&step, index](bool on) { step.chaseMarkers(index, on); };
        states.contacts.gather = [&](const AiVec3& centre, const AiVec3& halfExtents, const TrafficContactDescriptor& descriptor, unsigned maxEntries) {
            contacts.clear();
            if (step.gatherContacts) contacts = step.gatherContacts(index, centre, halfExtents, descriptor, maxEntries);
            return static_cast<std::uint32_t>(contacts.size());
        };
        states.contacts.slide = [&](const AiVec3& distance, std::uint32_t) {
            TrafficContactCar self;
            self.node = entity + 1;
            self.radius = body->radius;
            self.pusher = (step.carFlags & kEntityFlagBlocker) != 0;
            TrafficContactSlideHooks slideHooks = step.slide;
            if (step.pushOf) slideHooks.push = step.pushOf(index);
            AiVec3 move = distance;
            slideTrafficCarContacts(self, move, contacts, slideHooks);
        };
        states.events = &stateEvents;

        AiDriverHooks hooks;
        connectTrafficDriverStates(hooks, states);
        hooks.setKinematic = [&](bool on) {
            if (step.bodies.switchKinematic) step.bodies.switchKinematic(index, on);
        };
        hooks.signalLetsPass = [&](std::uint32_t signal) { return trafficJunctionAllowsCar(needJunction(world, signal).layout, car.driver.route); };
        hooks.leaveSignalQueue = [&](std::uint32_t signal) { leaveTrafficJunctionQueue(needJunction(world, signal), car.driver, entity); };
        hooks.snapTarget = [&] { snapTrafficTarget(car.driver, step.ground); };
        hooks.chooseNextRoute = [&](AiDriverRecord& driver, std::uint32_t& next) {
            return chooseTrafficJunction(driver, {entity, node[3], 0.0f, &routes}, world, random, next);
        };
        hooks.leaveTraffic = [&] { leaveTraffic(manager, index, &result.traffic); };
        hooks.stopAll = [&] { return trafficWhistleStops(whistle_, step.clock); };
        hooks.lights = [&](bool stopped, float dt) {
            if (step.lights) {
                step.lights->run(index, stopped ? 1.0f : 0.0f, dt);
                return;
            }
            TrafficBrakeLightFrame frame;
            frame.brake = stopped ? 1.0f : 0.0f;
            frame.dt = dt;
            runTrafficBrakeLights(car.brakeLights, frame);
        };
        hooks.stopAtRouteEnd = [&] { return trafficStopAtLights(car.driver, world); };
        hooks.setNodeMatrix = [&](const AiMatrix& matrix) {
            node = matrix;
            moved = true;
        };
        // STAND-IN TRAFFIC-S35: without the lights object (the drivers' own tests) 0x23de70 is taken as true.
        hooks.nodeHidden = [&step, index] { return step.lights ? step.lights->nodeCulled(index) : true; };
        hooks.moveAlongRoute = [&](AiDriverRecord& driver) {
            TrafficMoverFrame frame;
            frame.car = entity;
            frame.node = node;
            frame.dt = step.dt;
            frame.radius = body->radius;
            frame.kinematic = step.bodies.kinematic && step.bodies.kinematic(index);
            frame.gearSpeed = body->gearSpeed;
            frame.routes = &routes;
            const TrafficMoverResult moverResult = runTrafficRouteMover(driver, frame, world, hooks, random);
            node = moverResult.node;
            if (moverResult.velocity && step.bodies.setVelocity) step.bodies.setVelocity(index, *moverResult.velocity);
            moved = true;
        };

        const std::uint8_t stateBefore = car.driver.state;

        AiDriverInputFrame input;
        input.node = node;
        input.halfExtents = body->halfExtents;
        input.dt = step.dt;
        input.gearSpeed = body->gearSpeed;
        input.routes = &routes;
        for (const TrafficObstacle& obstacle : step.obstacles)
            if (obstacle.car != index) input.obstacles.push_back(obstacle.entity);
        runAiDriverInput(car.driver, car.vehicle, input, hooks);

        AiDriverStateFrame change;
        change.node = node;
        change.body = bodyBasis(node);
        change.bodySpeed = body->forwardSpeed;
        change.activationState = body->activationState;
        change.routes = &routes;
        runAiDriverStateChange(car.driver, car.vehicle, change, hooks, random);
        if (car.driver.state != stateBefore && car.driver.state == kAiStateJunction)
            result.events.push_back({TrafficDriveEventKind::Junction, index, car.driver.signal});
        if (car.driver.state != stateBefore && stateBefore == kAiStateJunction && car.driver.state == kAiStateRoute)
            result.events.push_back({TrafficDriveEventKind::Road, index, car.driver.signal});

        const float brakeBefore = car.vehicle.brakeInput;
        AiDriverPostFrame post;
        post.node = node;
        post.dt = step.dt;
        post.bodySpeed = body->forwardSpeed;
        post.bodyMoveSpeed = body->moveSpeed;
        post.now = step.now;
        post.wheelCount = body->wheelCount;
        post.groundedWheels = body->groundedWheels;
        post.routes = &routes;
        runAiDriverPostStep(car.driver, car.vehicle, post, hooks, random);
        const bool lightsStop = car.driver.state != kAiStateKnocked;
        if (lightsStop && brakeBefore != 1.0f && car.vehicle.brakeInput == 1.0f) result.events.push_back({TrafficDriveEventKind::Stop, index, car.driver.signal});
        if (lightsStop && brakeBefore == 1.0f && car.vehicle.brakeInput != 1.0f) result.events.push_back({TrafficDriveEventKind::Go, index, car.driver.signal});

        TrafficCar& view = views[index];
        view.aiDriven = (car.driver.flags & kAiDriverFlagAi) != 0;
        view.driveState = car.driver.state;
        view.driveStage = car.driver.requestedState;
        view.keepWhenFar = car.driver.keep != 0;
        view.driveValue = car.driver.speed;
        view.pathParameter = car.driver.routeDistance;
        view.route = routeIndex(manager.routeTable(), car.driver.route);
        view.position = node[3];
        if (moved) result.moves.push_back({index, node});
        for (const TrafficStateEvent& event : stateEvents) result.states.push_back({index, event});
    }
    return result;
}

}
