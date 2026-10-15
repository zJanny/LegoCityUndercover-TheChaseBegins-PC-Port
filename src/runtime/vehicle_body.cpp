#include "runtime/vehicle_body.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {

Vector3 cross(const Vector3& a, const Vector3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float dot(const Vector3& a, const Vector3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

std::array<Vector3, 3> rotationRows(const std::array<float, 4>& q) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float s = 2.0f / (x * x + y * y + z * z + w * w);
    const float ys = y * s, zs = z * s, wxs = w * x * s, xxs = x * x * s;
    return {Vector3{1.0f - (y * ys + z * zs), x * ys - w * zs, x * zs + w * ys},
            Vector3{x * ys + w * zs, 1.0f - (xxs + z * zs), y * zs - wxs},
            Vector3{x * zs - w * ys, y * zs + wxs, 1.0f - (xxs + y * ys)}};
}

}

float vehicleLinearDamping(float dampingLinear, float speed) {
    const float scaled = std::min(speed, kVehicleDampingSpeedCap) * kVehicleDampingSpeedScale;
    return std::clamp(dampingLinear * scaled * scaled, 0.0f, 1.0f);
}

std::array<Vector3, 3> bodyAxes(const VehicleRigidBody& body) {
    const std::array<Vector3, 3> m = rotationRows(body.orientation);
    return {Vector3{m[0][0], m[1][0], m[2][0]}, Vector3{m[0][1], m[1][1], m[2][1]}, Vector3{m[0][2], m[1][2], m[2][2]}};
}

Vector3 bodyToWorld(const VehicleRigidBody& body, const Vector3& local) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    Vector3 world = body.position;
    for (int a = 0; a < 3; ++a)
        for (int k = 0; k < 3; ++k) world[std::size_t(k)] += local[std::size_t(a)] * axes[std::size_t(a)][std::size_t(k)];
    return world;
}

std::array<float, 4> headingOrientation(float heading) { return {0.0f, std::sin(heading * 0.5f), 0.0f, std::cos(heading * 0.5f)}; }

void addForceAtPoint(VehicleRigidBody& body, const Vector3& force, const Vector3& point) {
    const Vector3 arm = {point[0] - body.position[0], point[1] - body.position[1], point[2] - body.position[2]};
    const Vector3 turn = cross(arm, force);
    for (std::size_t a = 0; a < 3; ++a) {
        body.force[a] += force[a];
        body.torque[a] += turn[a];
    }
}

Vector3 pointVelocity(const VehicleRigidBody& body, const Vector3& point) {
    const Vector3 arm = {point[0] - body.position[0], point[1] - body.position[1], point[2] - body.position[2]};
    const Vector3 spin = cross(body.angularVelocity, arm);
    return {body.velocity[0] + spin[0], body.velocity[1] + spin[1], body.velocity[2] + spin[2]};
}

void stepRigidBody(VehicleRigidBody& body, float linearDamping, float angularDamping, float dt) {
    applyRigidBodyForces(body, dt);
    integrateRigidBody(body, linearDamping, angularDamping, dt);
}

Vector3 worldInverseInertia(const VehicleRigidBody& body, const Vector3& value) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    Vector3 out{};
    for (std::size_t a = 0; a < 3; ++a) {
        const float local = dot(axes[a], value) * body.inverseInertia[a];
        for (std::size_t k = 0; k < 3; ++k) out[k] += axes[a][k] * local;
    }
    return out;
}

void applyImpulse(VehicleRigidBody& body, const Vector3& impulse, const Vector3& arm) {
    if (body.inverseMass == 0.0f) return;
    for (std::size_t a = 0; a < 3; ++a) body.velocity[a] += impulse[a] * body.inverseMass;
    const Vector3 turn = worldInverseInertia(body, cross(arm, impulse));
    for (std::size_t a = 0; a < 3; ++a) body.angularVelocity[a] += turn[a];
}

void applyRigidBodyForces(VehicleRigidBody& body, float dt) {
    body.force[1] += kVehicleBodyGravity / body.inverseMass;
    const float push = body.inverseMass * dt;
    for (std::size_t a = 0; a < 3; ++a) body.velocity[a] += body.force[a] * push;
    const std::array<Vector3, 3> axes = bodyAxes(body);
    Vector3 local{};
    for (std::size_t a = 0; a < 3; ++a) local[a] = dot(axes[a], body.torque) * body.inverseInertia[a];
    for (std::size_t k = 0; k < 3; ++k)
        for (std::size_t a = 0; a < 3; ++a) body.angularVelocity[k] += axes[a][k] * local[a] * dt;
    const float spin = std::sqrt(dot(body.angularVelocity, body.angularVelocity));
    if (spin * dt > kVehicleAngularStepLimit) {
        const float scale = kVehicleAngularStepLimit / dt / spin;
        for (float& value : body.angularVelocity) value *= scale;
    }
    body.force = {0, 0, 0};
    body.torque = {0, 0, 0};
}

void integrateRigidBody(VehicleRigidBody& body, float linearDamping, float angularDamping, float dt) {
    const float keepLinear = std::pow(1.0f - linearDamping, dt), keepAngular = std::pow(1.0f - angularDamping, dt);
    for (std::size_t a = 0; a < 3; ++a) {
        body.velocity[a] *= keepLinear;
        body.angularVelocity[a] *= keepAngular;
    }
    for (std::size_t a = 0; a < 3; ++a) body.position[a] += body.velocity[a] * dt;
    std::array<float, 4>& q = body.orientation;
    const float h = dt * 0.5f, ax = body.angularVelocity[0], ay = body.angularVelocity[1], az = body.angularVelocity[2];
    std::array<float, 4> next = {q[0] + (ax * q[3] + ay * q[2] - az * q[1]) * h, q[1] + (ay * q[3] + az * q[0] - ax * q[2]) * h,
                                 q[2] + (az * q[3] + ax * q[1] - ay * q[0]) * h, q[3] + (-q[0] * ax - ay * q[1] - az * q[2]) * h};
    const float length = std::sqrt(next[0] * next[0] + next[1] * next[1] + next[2] * next[2] + next[3] * next[3]);
    for (std::size_t a = 0; a < 4; ++a) q[a] = next[a] / length;
    body.force = {0, 0, 0};
    body.torque = {0, 0, 0};
}

int vehicleWorldSteps(float& leftover, float dt) {
    leftover += dt;
    if (kVehicleWorldStep > leftover) return 0;
    const int steps = int(leftover / kVehicleWorldStep);
    leftover -= float(steps) * kVehicleWorldStep;
    return std::min(steps, kVehicleMaxWorldSteps);
}

bool updateVehicleActivation(VehicleRigidBody& body, float dt) {
    if (dot(body.velocity, body.velocity) < kVehicleSleepSpeed * kVehicleSleepSpeed &&
        dot(body.angularVelocity, body.angularVelocity) < kVehicleSleepTurn * kVehicleSleepTurn)
        body.deactivationTime += dt;
    else
        body.deactivationTime = 0.0f;
    if (!(body.deactivationTime > kVehicleSleepTime)) return true;
    body.velocity = {0.0f, 0.0f, 0.0f};
    body.angularVelocity = {0.0f, 0.0f, 0.0f};
    return false;
}

float forwardSpeed(const VehicleRigidBody& body) {
    const Vector3 forward = bodyAxes(body)[2];
    return body.velocity[0] * forward[0] + body.velocity[1] * forward[1] + body.velocity[2] * forward[2];
}

void limitForwardSpeed(VehicleRigidBody& body, float limit) {
    const float speed = forwardSpeed(body);
    if (speed == 0.0f) return;
    const float scale = std::min(std::fabs(speed), limit) / std::fabs(speed);
    for (float& value : body.velocity) value *= scale;
}

}
