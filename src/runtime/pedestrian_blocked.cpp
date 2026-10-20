#include "runtime/pedestrian_blocked.hpp"

#include "runtime/camera_reset_probe.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

std::int32_t bitsOf(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float dot(const PedestrianVec3& a, const PedestrianVec3& b) {
    float sum = a[0] * b[0];
    sum = sum + a[1] * b[1];
    sum = sum + a[2] * b[2];
    return sum;
}

PedestrianVec3 subtract(const PedestrianVec3& a, const PedestrianVec3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

float distanceSquared(const PedestrianVec3& a, const PedestrianVec3& b) {
    const PedestrianVec3 d = subtract(a, b);
    float sum = d[0] * d[0];
    sum = sum + d[1] * d[1];
    sum = sum + d[2] * d[2];
    return sum;
}

PedestrianVec3 toLocal(const PedestrianVec3& point, const std::array<PedestrianVec3, 4>& matrix) {
    const PedestrianVec3 d = subtract(point, matrix[3]);
    return {dot(d, matrix[0]), dot(d, matrix[1]), dot(d, matrix[2])};
}

void slideAlong(const PedestrianVec3& normal, PedestrianVec3& vector) {
    const float d = dot(normal, vector);
    if (!(d < 0.0f)) return;
    for (int a = 0; a < 3; ++a) vector[a] = vector[a] - normal[a] * d;
}

void slideAtFace(PedestrianVec3 normal, float now, float next, float limit, PedestrianVec3& vector) {
    if (now > limit && next < limit) {
        slideAlong(normal, vector);
    } else if (now < -limit && next > -limit) {
        normal[0] = -normal[0];
        normal[2] = -normal[2];
        slideAlong(normal, vector);
    }
}

}

PedestrianBlockVehicle pedestrianBlockVehicle(const assets::Matrix& node, const PedestrianVec3& halfExtents, float speed, bool kinematic) {
    PedestrianBlockVehicle vehicle;
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) vehicle.matrix[row][axis] = node[row * 4 + axis];
    vehicle.halfExtents = halfExtents;
    float squared = halfExtents[0] * halfExtents[0];
    squared = squared + halfExtents[1] * halfExtents[1];
    squared = squared + halfExtents[2] * halfExtents[2];
    vehicle.radius = std::sqrt(squared);
    vehicle.speed = speed;
    vehicle.kinematic = kinematic;
    return vehicle;
}

bool pedestrianDodgeRayBlocked(const CollisionWorld& world, const PedestrianVec3& from, const PedestrianVec3& to) {
    return CameraResetProbe(world, "", kPedestrianDodgeRayLayer, kPedestrianDodgeRaySurfaces).segmentBlocked(from.data(), to.data());
}

int pedestrianVehicleBlock(PedestrianBody& body, float radius, const PedestrianVec3& position, PedestrianVec3& vector, float dt,
                           const PedestrianBlockVehicle& vehicle, const PedestrianLineTest& lineBlocked, GameRandom& random) {
    const float limitX = vehicle.halfExtents[0] + kPedestrianBlockMarginX;
    const float limitZ = vehicle.halfExtents[2] + kPedestrianBlockMarginZ;
    const float step = std::fabs(vehicle.speed * dt);
    const float reach = limitZ + step * kPedestrianBlockReachPerStep;
    const float range = (vehicle.radius + radius) + reach;
    const PedestrianVec3 next{position[0] + vector[0], position[1] + vector[1], position[2] + vector[2]};
    if (distanceSquared(next, vehicle.matrix[3]) > range * range) return 0;
    const PedestrianVec3 local = toLocal(next, vehicle.matrix);

    if (bitsOf(step) <= bitsOf(kPedestrianBlockSlowStep)) {
        if (!(std::fabs(local[0]) < limitX)) return 0;
        if (!(std::fabs(local[2]) < limitZ)) return 0;
        int result = 0;
        if (dot(subtract(vehicle.matrix[3], position), subtract(body.target, position)) > 0.0f) result = kPedestrianBlockTurnBack;
        const PedestrianVec3 now = toLocal(position, vehicle.matrix);
        slideAtFace({vehicle.matrix[0][0], 0.0f, vehicle.matrix[0][2]}, now[0], local[0], limitX, vector);
        slideAtFace({vehicle.matrix[2][0], 0.0f, vehicle.matrix[2][2]}, now[2], local[2], limitZ, vector);
        return result;
    }

    if (!(std::fabs(local[0]) < limitX)) return 0;
    int result = 0;
    if (body.walkStarted && reach > std::fabs(local[2])) {
        std::array<float, 2> offsets{kPedestrianDodgeOffset, -kPedestrianDodgeOffset};
        std::array<std::uint16_t, 2> states{kPedestrianDodgeRightState, kPedestrianDodgeLeftState};
        if (local[0] < 0.0f) {
            offsets = {-kPedestrianDodgeOffset, kPedestrianDodgeOffset};
            states = {kPedestrianDodgeLeftState, kPedestrianDodgeRightState};
        }
        for (std::size_t side = 0; side < offsets.size(); ++side) {
            PedestrianVec3 from = position;
            from[1] = from[1] + kPedestrianDodgeRayHeight;
            PedestrianVec3 to{};
            for (int a = 0; a < 3; ++a) to[a] = from[a] + vehicle.matrix[0][a] * offsets[side];
            to[1] = from[1];
            if (lineBlocked && lineBlocked(from, to)) continue;
            body.requested = states[side];
            if (side == 1 || reach <= vehicle.speed) body.dodgeSide = 1;
            else body.dodgeSide = static_cast<std::uint8_t>(random.next() % 2);
            result = kPedestrianBlockDodged;
            break;
        }
    }
    if (body.requested != kPedestrianWalkState) return result;
    if (!(std::fabs(local[0]) < vehicle.halfExtents[0])) return result;
    if (!(std::fabs(local[2]) < vehicle.halfExtents[2])) return result;
    body.requested = kPedestrianSplatState;
    body.walkStarted = false;
    return result;
}

int pedestrianBlocked(PedestrianBody& body, float radius, const PedestrianVec3& position, PedestrianVec3& vector, float dt,
                      const std::vector<PedestrianBlockVehicle>& vehicles, const PedestrianLineTest& lineBlocked, GameRandom& random) {
    for (const PedestrianBlockVehicle& vehicle : vehicles) {
        if (vehicle.kinematic) continue;
        const int result = pedestrianVehicleBlock(body, radius, position, vector, dt, vehicle, lineBlocked, random);
        if (result != 0) return result;
    }
    return 0;
}

bool pedestrianSpawnBlocked(const PedestrianBody& body, float radius, const PedestrianVec3& position,
                            const std::vector<PedestrianBlockVehicle>& vehicles, const PedestrianLineTest& lineBlocked, GameRandom& random) {
    PedestrianBody copy = body;
    PedestrianVec3 zero{};
    return pedestrianBlocked(copy, radius, position, zero, 0.0f, vehicles, lineBlocked, random) != 0;
}

}
