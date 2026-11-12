#include "game_level.hpp"
#include "runtime/traffic_ground_world.hpp"
#include "runtime/traffic_leave_snap.hpp"
#include "runtime/vehicle_chase_events.hpp"

#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

runtime::AiMatrix nodeRows(const chase::render::Mat4& matrix) {
    runtime::AiMatrix node{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) node[row][axis] = matrix[row * 4 + axis];
    return node;
}

chase::render::Mat4 nodeMatrix(const runtime::AiMatrix& node) {
    chase::render::Mat4 matrix{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) matrix[row * 4 + axis] = node[row][axis];
    matrix[15] = 1.0f;
    return matrix;
}

}

void LevelSession::chasedCarImpact(std::size_t vehicleIndex, float speed, const runtime::ContactOther& other) {
    const std::string& instance = boardingVehicles[vehicleIndex].instance;
    runtime::VehicleAiRecord* ai = levelScripts.vehicleAi().mutableRecord(instance);
    if (!ai || !ai->chased) return;
    runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[vehicleIndex]);
    const std::optional<runtime::DamageMessage> message =
        runtime::chasedCarContactDamage(ai->driver, {other.playerVehicle, other.destroyed, speed});
    if (!message) return;
    const runtime::ChasedCarDamageReply reply = runtime::chasedCarDamage(ai->driver, record, *message);
    if (!reply.consumed) {
        const runtime::DamageOutcome outcome = runtime::vehicleDamage(record, *message, false);
        if (outcome.heartLost) {
            vehicleDamageEffects(vehicleIndex, outcome);
            std::cout << "vehicle " << instance << ": heart lost (" << speed << " m/s), " << unsigned(record.hearts) << " left\n";
        }
        return;
    }
    if (reply.chaseEnded) std::cout << "vehicle " << instance << ": driver state 6 requested at update " << updatesForLog << '\n';
}

std::optional<runtime::BodyInputs> LevelSession::driveScriptCar(std::size_t vehicleIndex, float dt) {
    return runScriptCar(vehicleIndex, dt, false);
}

void LevelSession::postStepScriptCar(std::size_t vehicleIndex, float dt) { runScriptCar(vehicleIndex, dt, true); }

std::optional<runtime::BodyInputs> LevelSession::runScriptCar(std::size_t vehicleIndex, float dt, bool postStep) {
    const runtime::BoardingVehicle& vehicle = boardingVehicles[vehicleIndex];
    runtime::VehicleAiRecord* ai = levelScripts.vehicleAi().mutableRecord(vehicle.instance);
    // STAND-IN SCRIPT-S323: only a car put into the chase by VehicleSetChased is driven; a record of VehicleSetAI alone is not stepped.
    if (!ai || !ai->chased || (ai->driver.flags & runtime::kAiDriverFlagAi) == 0) return std::nullopt;
    runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[vehicleIndex]);
    const VehicleDrive& drive = vehicleDrives[vehicleIndex];

    runtime::ScriptCarFrame frame;
    frame.node = nodeRows(vehicle.matrix);
    frame.halfExtents = {vehicle.boxHalf[0], vehicle.boxHalf[1], vehicle.boxHalf[2]};
    frame.radius = std::sqrt(frame.halfExtents[0] * frame.halfExtents[0] + frame.halfExtents[1] * frame.halfExtents[1] + frame.halfExtents[2] * frame.halfExtents[2]);
    frame.dt = dt;
    frame.gearSpeed = record.topSpeed;
    const runtime::Vector3& velocity = drive.rigid.velocity;
    frame.bodySpeed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
    frame.vehicleState = record.state;
    frame.gear = drive.drivetrain.gear;
    frame.playerInVehicle = boarding && boarding->seatedVehicle().has_value();
    frame.routes = &*scriptCarRoutes;
    runtime::AiDriverRecord* driver = &ai->driver;
    const runtime::TrafficGroundWorld ground = runtime::trafficGroundWorld(collision, [](std::uint32_t) {});
    frame.snapTarget = [driver, &ground] { runtime::snapTrafficTarget(*driver, ground); };
    const std::string instance = vehicle.instance;
    frame.chaseMarkers = [this, instance](bool on) { levelScripts.setChaseMarkers(instance, on); };

    runtime::ScriptCarDriver car{&ai->driver, &ai->vehicle, &ai->steering, &ai->lights, &ai->kinematic, &ai->bodyInputs};
    ai->vehicle.forcedStop = record.forcedStop;
    ai->vehicle.hearts = record.hearts;
    const std::uint8_t stateBefore = ai->driver.state;
    const runtime::ScriptCarStep step = postStep ? runtime::runScriptCarPostStep(car, frame, scriptCarRandomStandIn)
                                                 : runtime::runScriptCarDriver(car, frame, scriptCarRandomStandIn);
    record.forcedStop = ai->vehicle.forcedStop;
    if (ai->driver.state != stateBefore)
        std::cout << "vehicle " << instance << ": driver state " << unsigned(ai->driver.state) << " at update " << updatesForLog << '\n';
    if (step.node) placeVehicleAt(vehicleIndex, nodeMatrix(*step.node));
    for (const runtime::TrafficStateEvent& event : step.events) {
        switch (event.kind) {
        case runtime::TrafficStateEventKind::Placed:
            break;
        case runtime::TrafficStateEventKind::Stopping: {
            ai->kinematic = false;
            runtime::DamageOutcome smoke;
            smoke.smokeEffect = event.stopping.smokeEffect;
            vehicleDamageEffects(vehicleIndex, smoke);
            std::cout << "vehicle " << instance << ": chase ended at update " << updatesForLog << '\n';
            levelScripts.fireObjectEvent(event.stopping.event, instance);
            break;
        }
        case runtime::TrafficStateEventKind::Destroy: {
            const runtime::ChasedCarDamageReply reply = runtime::chasedCarDamage(ai->driver, record, event.message);
            if (reply.consumed) break;
            const runtime::DamageOutcome outcome = runtime::vehicleDamage(record, event.message, false);
            std::cout << "vehicle " << instance << ": destroyed by its driver at update " << updatesForLog << '\n';
            if (outcome.destroyed) breakVehicleNode(vehicleIndex);
            break;
        }
        case runtime::TrafficStateEventKind::Throw:
            std::cout << "vehicle " << instance << ": throw at " << event.target[0] << ' ' << event.target[1] << ' ' << event.target[2] << " at update "
                      << updatesForLog << " (no throw objects)\n";
            break;
        }
    }
    if (postStep) return std::nullopt;
    return ai->bodyInputs;
}

}
