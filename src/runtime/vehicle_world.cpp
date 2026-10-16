#include "runtime/vehicle_world.hpp"

#include <algorithm>

namespace chase::runtime {

VehicleShapeBoxes vehicleShapeBoxes(const VehicleRecord& record, const float entityCentre[3], const float entityHalf[3]) {
    const Vector3 centre = {entityCentre[0], entityCentre[1], entityCentre[2]};
    const Vector3 half = {entityHalf[0], entityHalf[1], entityHalf[2]};
    VehicleShapeBoxes boxes;
    vehicleShapeBox(centre, half, record.centreOfMass, boxes.shapeLow, boxes.shapeHigh);
    vehicleCollisionBox(record.className, centre, half, record.centreOfMass, boxes.collisionLow, boxes.collisionHigh);
    return boxes;
}

bool vehicleHasDynamicBody(const VehicleRecord& record, bool hasBody, bool disabled) {
    if (disabled) return false;
    if (hasBody) return true;
    const auto parked = [](std::uint16_t state) { return state == 4 || state == 10; };
    return parked(record.state) || parked(record.requestedState);
}

float vehicleMassScale(bool playerVehicle) { return playerVehicle ? kVehicleBodyMassFactor : 1.0f; }

void stepVehicleBodies(std::vector<VehicleWorldBody>& bodies, VehiclePairManifolds& pairs, const WheelRay& ray, int steps) {
    for (int step = 0; step < steps; ++step) {
        for (VehicleWorldBody& entry : bodies) {
            VehicleRigidBody& body = *entry.body;
            DrivetrainState& drive = *entry.drive;
            if (entry.floor) trackFloorCell(entry.floor->mesh, entry.floor->tracker, body.position);
            bool woke = false;
            const float speedSquared = body.velocity[0] * body.velocity[0] + body.velocity[1] * body.velocity[1] + body.velocity[2] * body.velocity[2];
            if (vehicleWakes(entry.controls, forwardSpeed(body), speedSquared, drive.previousSteer)) {
                woke = !*entry.awake;
                *entry.awake = true;
                body.deactivationTime = 0.0f;
            }
            if (!*entry.awake) {
                sleepVehicle(drive, entry.controls);
                continue;
            }
            std::vector<WheelState> wheels = stepVehiclePhysics(body, drive, *entry.record, entry.controls, woke, ray, kVehicleWorldStep, entry.floor);
            if (entry.lastWheels) *entry.lastWheels = std::move(wheels);
        }
        for (VehicleWorldBody& entry : bodies)
            if (*entry.awake && entry.floor && entry.floor->boxes) collideVehicleWithBoxes(*entry.body, *entry.floor->boxes);
        std::vector<std::pair<std::size_t, std::size_t>> active;
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            for (std::size_t j = 0; j < bodies.size(); ++j) {
                if (i == j || bodies[i].id >= bodies[j].id) continue;
                if (!bodies[i].floor || !bodies[i].floor->boxes || !bodies[j].floor || !bodies[j].floor->boxes) continue;
                if (!*bodies[i].awake && !*bodies[j].awake) continue;
                std::optional<ContactManifold>& manifold = pairs[{bodies[i].id, bodies[j].id}];
                collideVehiclePair(*bodies[i].body, *bodies[i].floor->boxes, *bodies[j].body, *bodies[j].floor->boxes, manifold);
                if (manifold) active.push_back({i, j});
            }
        }
        std::vector<VehicleRigidBody*> list;
        std::vector<SolverManifold> manifolds;
        for (VehicleWorldBody& entry : bodies) list.push_back(entry.body);
        for (std::size_t i = 0; i < bodies.size(); ++i)
            if (*bodies[i].awake && bodies[i].floor && bodies[i].floor->boxes)
                for (ContactManifold& manifold : bodies[i].floor->boxes->manifolds) manifolds.push_back({&manifold, int(i), -1});
        std::sort(active.begin(), active.end(), [&](const auto& a, const auto& b) {
            return std::make_pair(bodies[a.first].id, bodies[a.second].id) < std::make_pair(bodies[b.first].id, bodies[b.second].id);
        });
        for (const auto& [first, second] : active)
            manifolds.push_back({&*pairs[{bodies[first].id, bodies[second].id}], int(second), int(first)});
        BulletSolverInfo info;
        info.timeStep = kVehicleWorldStep;
        const std::vector<bool> activated = solveContactGroup(list, manifolds, info);
        for (std::size_t i = 0; i < bodies.size(); ++i)
            if (activated[i]) *bodies[i].awake = true;
        for (VehicleWorldBody& entry : bodies)
            if (*entry.awake) *entry.awake = updateVehicleActivation(*entry.body, kVehicleWorldStep);
    }
}

}
