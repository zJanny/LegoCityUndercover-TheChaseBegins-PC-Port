#include "runtime/vehicle_wheels.hpp"
#include "runtime/vehicle_sphere_sweep.hpp"
#include "runtime/vehicle_world.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace chase::runtime {
namespace {

constexpr float kStiffnessTable[11] = {0.0f,          0x1.9e7c7p-9f,  0x1.2515fep-6f, 0x1.93d32ep-5f, 0x1.9e7c7p-4f, 0x1.6a09e6p-3f,
                                       0x1.1d8c1ep-2f, 0x1.a3cd72p-2f, 0x1.2515fep-1f, 0x1.89702p-1f,  1.0f};
constexpr float kDampingTable[11] = {2.0f,           0x1.fe6184p+0f, 0x1.f6d75p+0f, 0x1.e6c2cep+0f, 0x1.cc3072p+0f, 0x1.a57d86p+0f,
                                     0x1.7139fp+0f,  0x1.2e1948p+0f, 0x1.b5d404p-1f, 0x1.da3f8p-2f, 0.25f};

float tableAt(const float (&table)[11], float compression) {
    const float scaled = compression * 10.0f;
    const int index = int(scaled);
    if (index >= 10) return table[10];
    const float t = std::clamp(scaled - float(index), 0.0f, 1.0f);
    return table[index] + (table[index + 1] - table[index]) * t;
}

Vector3 subtract(const Vector3& a, const Vector3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vector3 cross(const Vector3& a, const Vector3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float dot(const Vector3& a, const Vector3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vector3 along(const Vector3& from, const Vector3& direction, float distance) {
    return {from[0] + direction[0] * distance, from[1] + direction[1] * distance, from[2] + direction[2] * distance};
}

}

float suspensionStiffnessCurve(float compression) { return tableAt(kStiffnessTable, compression); }
float suspensionDampingCurve(float compression) { return tableAt(kDampingTable, compression); }

float wheelRayLength(const WheelSuspension& wheel) { return std::max(wheel.travel - wheel.radius, kWheelRayMinimumReach) + wheel.radius; }

float suspensionForce(const WheelSuspension& wheel, float distance, float normalSpeed, float suspensionScale) {
    const float compression = std::clamp((wheel.travel - distance) / wheel.travel, 0.0f, 1.0f);
    const float spring = suspensionStiffnessCurve(compression) * (kSuspensionStiffnessSetting * kSuspensionStiffnessScale) * kSuspensionForceFactor;
    const float damper = suspensionDampingCurve(compression) * (kSuspensionDampingSetting * kSuspensionDampingScale) * kSuspensionDampingFactor;
    const float damping = normalSpeed >= 0.0f ? wheel.dampingExtend : wheel.dampingCompress;
    const float force = wheel.stiffness * suspensionScale * spring - damping * suspensionScale * (normalSpeed * damper);
    return std::max(force, 0.0f);
}

std::optional<WheelHit> castWheelRay(const CollisionWorld& world, const Vector3& from, const Vector3& to, const std::function<bool(const CollisionWorld::Entry&)>& skip) {
    const Vector3 segment = subtract(to, from);
    float low[3], high[3];
    for (std::size_t a = 0; a < 3; ++a) {
        low[a] = std::min(from[a], to[a]);
        high[a] = std::max(from[a], to[a]);
    }
    float best = 2.0f;
    WheelHit hit;
    world.queryExcept(low, high, kPlayerLayer, [&](const CollisionWorld::Entry& entry) { return skip && skip(entry); }, [&](const CollisionWorld::Triangle& triangle) {
        const Vector3 normal = {triangle.record->normal[0], triangle.record->normal[1], triangle.record->normal[2]};
        const float startSide = dot(from, normal) - triangle.record->d;
        const float endSide = dot(to, normal) - triangle.record->d;
        if (startSide < 0.0f || endSide >= 0.0f) return;
        const float t = startSide / (startSide - endSide);
        if (t >= best) return;
        const Vector3 point = along(from, segment, t);
        int positive = 0, negative = 0;
        for (int k = 0; k < 3; ++k) {
            const float* a = triangle.vertex[k];
            const float* b = triangle.vertex[(k + 1) % 3];
            const Vector3 edge = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
            const float side = dot(cross(edge, {point[0] - a[0], point[1] - a[1], point[2] - a[2]}), normal);
            positive += side > 0.0f;
            negative += side < 0.0f;
        }
        if (positive > 0 && negative > 0) return;
        best = t;
        hit.point = point;
        hit.normal = normal;
    });
    if (best > 1.0f) return std::nullopt;
    return hit;
}

std::optional<WheelHit> castFloorWheelRay(const FloorMesh& mesh, int bodyCell, const Vector3& bodyPosition, const Vector3& from, const Vector3& to) {
    if (bodyCell < 0) return std::nullopt;
    FloorPoint start = {from[0], from[1], from[2]};
    const int cell = walkFloor(mesh, bodyCell, {bodyPosition[0], bodyPosition[1], bodyPosition[2]}, start, FloorWalkEdge::Fail);
    if (cell < 0) return std::nullopt;
    const Vector3 segment = subtract(to, from);
    const float length = std::sqrt(dot(segment, segment));
    const Vector3 direction = {segment[0] / length, segment[1] / length, segment[2] / length};
    const std::optional<FloorRayHit> hit = floorRay(mesh, cell, start, direction, length);
    if (!hit) return std::nullopt;
    const assets::CollisionTriangle& triangle = mesh.triangle(hit->cell);
    return WheelHit{{hit->point[0], hit->point[1], hit->point[2]}, {triangle.normal[0], triangle.normal[1], triangle.normal[2]}};
}

std::vector<WheelState> applyWheelForces(VehicleRigidBody& body, const std::vector<VehicleWheel>& wheels, float suspensionScale,
                                         const WheelRay& ray) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    const Vector3 up = axes[1];
    const Vector3 down = {-up[0], -up[1], -up[2]};
    std::vector<WheelState> states;
    for (std::size_t index = 0; index < wheels.size(); ++index) {
        const WheelSuspension& wheel = wheels[index].suspension;
        WheelState state;
        state.centre = bodyToWorld(body, {wheel.mount[0] + wheel.sideOffset, wheel.mount[1], wheel.mount[2]});
        const float reach = wheelRayLength(wheel);
        state.distance = reach;
        state.point = along(state.centre, down, wheel.travel);
        state.normal = up;
        const std::optional<WheelHit> hit = ray(along(state.centre, up, kWheelRayLift), along(state.centre, down, reach));
        if (hit) {
            state.contact = true;
            const Vector3 point = dot(subtract(hit->point, state.centre), down) < 0.0f ? state.centre : hit->point;
            const Vector3 gap = subtract(state.centre, point);
            state.distance = std::sqrt(dot(gap, gap));
            state.point = point;
            state.normal = hit->normal;
            const float normalSpeed = dot(pointVelocity(body, state.centre), hit->normal);
            state.force = suspensionForce(wheel, state.distance, normalSpeed, suspensionScale);
            if (index < 4) addForceAtPoint(body, along({0, 0, 0}, hit->normal, state.force), state.centre);
        }
        states.push_back(state);
    }
    return states;
}

Vector3 vehicleInverseInertiaStandIn(const std::string& className) {
    const auto endsWith = [&](const char* suffix) {
        const std::string tail = suffix;
        return className.size() >= tail.size() && className.compare(className.size() - tail.size(), tail.size(), tail) == 0;
    };
    if (endsWith("vehicle_van_donut")) return {0x1.ce53cp-15f, 0x1.2a8be8p-15f, 0x1.938d24p-15f};
    return {0x1.22c1d8p-13f, 0x1.c33c44p-15f, 0x1.74dd5p-12f};
}

VehicleRigidBody vehicleRigidBody(const VehicleRecord& record, const Vector3& node, float heading) {
    VehicleRigidBody body;
    body.orientation = headingOrientation(heading);
    body.position = node;
    body.position = bodyToWorld(body, record.centreOfMass);
    body.inverseMass = 1.0f / (kVehicleBodyMassFactor * record.mass);
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    return body;
}

Vector3 vehicleLocalInertia(float mass, const Vector3& low, const Vector3& high, const std::optional<std::array<float, 3>>& inertiaScale) {
    Vector3 l{};
    for (std::size_t a = 0; a < 3; ++a) {
        const float half = (high[a] - low[a]) * 0.5f;
        const float offset = (high[a] + low[a]) * 0.5f;
        const float boxHalf = (half - kBulletBoxMargin) + kBulletBoxMargin;
        const float localLow = offset - boxHalf, localHigh = offset + boxHalf;
        const float compoundHalf = (localHigh - localLow) * 0.5f;
        const float compoundCentre = (localHigh + localLow) * 0.5f;
        l[a] = ((compoundCentre + compoundHalf) - (compoundCentre - compoundHalf)) * 0.5f * 2.0f;
    }
    const float k = mass * 0x1.555556p-4f;
    float x = l[1] * l[1];
    x += l[2] * l[2];
    float y = l[0] * l[0];
    y += l[2] * l[2];
    float z = l[0] * l[0];
    z += l[1] * l[1];
    Vector3 inertia = {x * k, y * k, z * k};
    if (inertiaScale)
        for (std::size_t a = 0; a < 3; ++a) inertia[a] *= (*inertiaScale)[a];
    return inertia;
}

void setVehicleMassScale(VehicleRigidBody& body, const VehicleRecord& record, const Vector3& shapeLow, const Vector3& shapeHigh, float massScale) {
    const Vector3 inertia = vehicleLocalInertia(record.mass, shapeLow, shapeHigh, record.inertiaScale);
    const float mass = record.mass * massScale;
    body.inverseMass = mass != 0.0f ? 1.0f / mass : 0.0f;
    for (std::size_t a = 0; a < 3; ++a) {
        const float scaled = inertia[a] * massScale;
        body.inverseInertia[a] = scaled != 0.0f ? 1.0f / scaled : 0.0f;
    }
}

VehicleRigidBody vehicleRigidBody(const VehicleRecord& record, const Vector3& node, float heading, const Vector3& shapeLow, const Vector3& shapeHigh,
                                  float massScale) {
    VehicleRigidBody body = vehicleRigidBody(record, node, heading);
    setVehicleMassScale(body, record, shapeLow, shapeHigh, massScale);
    return body;
}

VehicleCollisionBody vehicleCollisionBody(const VehicleRecord& record, const Vector3& low, const Vector3& high, float massScale) {
    VehicleCollisionBody shape;
    shape.points = vehicleProbePoints(low, high, record.wheels);
    shape.massTimesTwo = record.mass * massScale;
    if (record.collSpheres) shape.sphereSet = buildVehicleSpheres(low, high, *record.collSpheres, record.collRadius, record.extraColSpheres);
    return shape;
}

std::array<float, 16> vehicleNodeMatrix(const VehicleRigidBody& body, const std::array<float, 3>& centreOfMass) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    std::array<float, 16> matrix{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t k = 0; k < 3; ++k) matrix[row * 4 + k] = axes[row][k];
    for (std::size_t k = 0; k < 3; ++k)
        matrix[12 + k] = body.position[k] - (centreOfMass[0] * axes[0][k] + centreOfMass[1] * axes[1][k] + centreOfMass[2] * axes[2][k]);
    matrix[15] = 1.0f;
    return matrix;
}

VehicleCollisionBody vehicleCollisionBody(const VehicleRecord& record) {
    Vector3 low, high;
    vehicleCollisionBoxStandIn(record.className, low, high);
    VehicleCollisionBody shape;
    shape.points = vehicleProbePoints(low, high, record.wheels);
    shape.massTimesTwo = kVehicleBodyMassFactor * record.mass;
    if (record.collSpheres) shape.sphereSet = buildVehicleSpheres(low, high, *record.collSpheres, record.collRadius, record.extraColSpheres);
    return shape;
}

DrivetrainState vehicleDrivetrain(const VehicleRecord& record) {
    DrivetrainState drive;
    drive.rpm = record.engine.idleRpm;
    drive.wheels.resize(record.wheels.size());
    return drive;
}

std::vector<WheelState> stepVehiclePhysics(VehicleRigidBody& body, DrivetrainState& drive, const VehicleRecord& record, const BodyInputs& controls,
                                           bool wokeThisUpdate, const WheelRay& ray, float dt, VehicleFloor* floor) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    const float forwardSpeed = dot(body.velocity, axes[2]);
    const float speedSquared = dot(body.velocity, body.velocity);
    float linearDamping = vehicleLinearDamping(record.dampingLinear, std::sqrt(speedSquared));
    steerVehicle(drive, record.engine.steering, controls.steer, axes[2], body.velocity);
    if (!controlDrivetrain(drive, record.engine, controls, forwardSpeed, speedSquared)) linearDamping = kVehicleSpeedLimitDamping;
    drive.previousSteer = controls.steer;
    std::vector<WheelState> states;
    std::vector<WheelGripSpec> grips;
    for (const VehicleWheel& wheel : record.wheels) grips.push_back(wheel.grip);
    drive.wheels.resize(record.wheels.size());
    if (!wokeThisUpdate) {
        for (std::size_t i = 0; i < record.wheels.size(); ++i) setupWheel(drive.wheels[i], grips[i], i, drive, axes);
        const WheelRay floorRay = [&](const Vector3& from, const Vector3& to) {
            return castFloorWheelRay(floor->mesh, floor->tracker.cell, body.position, from, to);
        };
        states = applyWheelForces(body, record.wheels, record.suspensionScale, floor ? floorRay : ray);
        for (std::size_t i = 0; i < record.wheels.size(); ++i) {
            WheelSpinState& wheel = drive.wheels[i];
            if (i < 4) {
                applyTyreForce(body, wheel, grips[i], record.wheels[i].suspension.radius, {states[i].point, states[i].normal, states[i].force},
                               record.engine.mass);
            } else {
                wheel.rate = drive.wheels[i - 2].rate;
                wheel.skid = drive.wheels[i - 2].skid;
            }
            spinWheel(wheel, drive, dt);
        }
    }
    stepDrivetrain(drive, record.engine, grips, forwardSpeed);
    applyRigidBodyForces(body, dt);
    if (floor) floor->shape.driven = controls.driver;
    if (floor) collideVehicleWithFloor(body, floor->mesh, floor->shape, floor->tracker, kTyreSurfaceFriction, linearDamping, record.dampingAngular, dt,
                                                  &floor->contactObserver);
    else integrateRigidBody(body, linearDamping, record.dampingAngular, dt);
    return states;
}


bool stepVehicleWorld(VehicleRigidBody& body, DrivetrainState& drive, bool awake, const VehicleRecord& record, const BodyInputs& controls,
                      const WheelRay& ray, int steps, VehicleFloor* floor, std::vector<WheelState>* lastWheels) {
    std::vector<VehicleWorldBody> bodies(1);
    bodies[0].body = &body;
    bodies[0].drive = &drive;
    bodies[0].awake = &awake;
    bodies[0].record = &record;
    bodies[0].controls = controls;
    bodies[0].floor = floor;
    bodies[0].lastWheels = lastWheels;
    VehiclePairManifolds pairs;
    stepVehicleBodies(bodies, pairs, ray, steps);
    return awake;
}

}
