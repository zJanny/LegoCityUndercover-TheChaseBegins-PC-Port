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

namespace {

bool wheelsOnGround(const VehicleWorldBody& entry) {
    if (!entry.lastWheels || entry.lastWheels->empty()) return true;
    int grounded = 0;
    for (const WheelState& wheel : *entry.lastWheels) grounded += wheel.contact ? 1 : 0;
    return grounded > 1;
}

}

void applyPairContactRule(ManifoldPoint& point, const VehicleWorldBody& a, const VehicleWorldBody& b) {
    if (!a.floor || !b.floor || !a.floor->shape.spheres || !b.floor->shape.spheres) return;
    if (!wheelsOnGround(a) || !wheelsOnGround(b)) return;
    if (a.controls.driver) {
        point.flags = std::uint16_t(point.flags | 1);
        if (b.record && b.record->bumpCollision) {
            point.friction = 0.2f;
            point.restitution = 1.1f;
        }
    }
    if (b.controls.driver) {
        point.flags = std::uint16_t(point.flags | 2);
        if (a.record && a.record->bumpCollision) {
            point.friction = 0.2f;
            point.restitution = 1.1f;
        }
    }
}

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
            if (entry.floor && entry.impact)
                entry.floor->contactObserver = [&entry](const VehicleRigidBody& rigid, const FloorContact& contact) { entry.impact(floorImpactSpeed(rigid, contact), ContactOther{}); };
            std::vector<WheelState> wheels = stepVehiclePhysics(body, drive, *entry.record, entry.controls, woke, ray, kVehicleWorldStep, entry.floor);
            if (entry.floor) entry.floor->contactObserver = nullptr;
            if (entry.lastWheels) *entry.lastWheels = std::move(wheels);
        }
        for (VehicleWorldBody& entry : bodies) {
            if (!*entry.awake || !entry.floor || !entry.floor->boxes) continue;
            const ManifoldPointObserver observer = [&entry](ManifoldPoint& point) {
                if (entry.impact) entry.impact(manifoldImpactSpeed(*entry.body, true, point, nullptr), ContactOther{});
                return false;
            };
            collideVehicleWithBoxes(*entry.body, *entry.floor->boxes, &observer);
        }
        std::vector<std::pair<std::size_t, std::size_t>> active;
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            for (std::size_t j = 0; j < bodies.size(); ++j) {
                if (i == j || bodies[i].id >= bodies[j].id) continue;
                if (!bodies[i].floor || !bodies[i].floor->boxes || !bodies[j].floor || !bodies[j].floor->boxes) continue;
                if (!*bodies[i].awake && !*bodies[j].awake) continue;
                std::optional<ContactManifold>& manifold = pairs[{bodies[i].id, bodies[j].id}];
                const auto otherOf = [&](const VehicleWorldBody& entry) { return ContactOther{true, entry.destroyed, true, false}; };
                const ManifoldPointObserver observer = [&](ManifoldPoint& point) {
                    applyPairContactRule(point, bodies[j], bodies[i]);
                    if (bodies[j].impact) bodies[j].impact(manifoldImpactSpeed(*bodies[j].body, true, point, bodies[i].body), otherOf(bodies[i]));
                    if (bodies[i].impact) bodies[i].impact(manifoldImpactSpeed(*bodies[i].body, false, point, bodies[j].body), otherOf(bodies[j]));
                    return false;
                };
                collideVehiclePair(*bodies[i].body, *bodies[i].floor->boxes, *bodies[j].body, *bodies[j].floor->boxes, manifold, &observer);
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
