#include "runtime/vehicle_break.hpp"
#include "runtime/vehicle_drive.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {

float length3(const Vector3& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

Vector3 normalised(const Vector3& v, float& length) {
    length = length3(v);
    if (length <= 1.1920929e-7f) return {0.0f, 0.0f, 1.0f};
    const float inverse = 1.0f / length;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

Vector3 transformPoint(const assets::Matrix& m, const Vector3& p) {
    return {p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12], p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
            p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]};
}

assets::Matrix eulerRotation(const Vector3& angles) {
    const float sx = std::sin(angles[0]), cx = std::cos(angles[0]);
    const float sy = std::sin(angles[1]), cy = std::cos(angles[1]);
    const float sz = std::sin(angles[2]), cz = std::cos(angles[2]);
    assets::Matrix m = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    m[0] = cy * cz;
    m[1] = sx * sy * cz + cx * sz;
    m[2] = sx * sz - cx * sy * cz;
    m[4] = -(cy * sz);
    m[5] = cx * cz - sx * sy * sz;
    m[6] = sx * cz + cx * sy * sz;
    m[8] = sy;
    m[9] = -(sx * cy);
    m[10] = cx * cy;
    return m;
}

assets::Matrix translation(const Vector3& t) { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t[0], t[1], t[2], 1}; }

}

void stepDrivenStuckTimer(DrivenStuckTimer& timer, float forwardSpeed, float accelerate, float brake, float yawRate, float dt) {
    const float speed = std::fabs(forwardSpeed);
    const bool anyInput = accelerate != 0.0f || brake != 0.0f;
    if (speed < kStuckDestroySpeed && anyInput && std::fabs(yawRate) < kStuckDestroyYawRate) {
        if (accelerate != 0.0f && brake != 0.0f) {
            timer.seconds = 0.0f;
            return;
        }
        timer.seconds += dt;
        if (timer.seconds > kStuckDestroySeconds) {
            timer.seconds = 0.0f;
            timer.destroyRequest = true;
        }
        return;
    }
    if (speed > kStuckDecaySpeed || !anyInput) {
        timer.seconds = 0.0f;
        return;
    }
    timer.seconds = std::max(timer.seconds - dt * kStuckDecayRate, 0.0f);
}

std::optional<DamageMessage> stuckDestroyMessage(DrivenStuckTimer& timer, bool forcedStop) {
    if (forcedStop) {
        timer.seconds = 0.0f;
        return std::nullopt;
    }
    if (!timer.destroyRequest) return std::nullopt;
    timer.destroyRequest = false;
    DamageMessage message;
    message.hearts = kStuckDestroyHearts;
    message.type = kVehicleDamageTypeScripted;
    return message;
}

Vector3 wheelLaunchVector(const Vector3& vehicleNode, const Vector3& wheelNode, GameRandom& random) {
    float length = 0.0f;
    Vector3 launch = normalised({wheelNode[0] - vehicleNode[0], wheelNode[1] - vehicleNode[1], wheelNode[2] - vehicleNode[2]}, length);
    launch[0] *= kWheelLaunchSideScale;
    launch[2] *= kWheelLaunchSideScale;
    launch[0] += (gameRandomUnit(random) - 0.5f) * kWheelLaunchJitter;
    launch[1] += kWheelLaunchLift + (gameRandomUnit(random) - 0.5f) * kWheelLaunchJitter;
    launch[2] += (gameRandomUnit(random) - 0.5f) * kWheelLaunchJitter;
    return launch;
}

bool stepWheelPiece(assets::Matrix& world, const Vector3& centre, float radius, Vector3& velocity, const WheelPieceSweep& sweep, GameRandom& random) {
    Vector3 spin = kWheelPieceSpin;
    const Vector3 from = transformPoint(world, centre);
    float speed = 0.0f;
    const Vector3 direction = normalised(velocity, speed);
    const Vector3 to = {from[0] + velocity[0] + direction[0] * radius, from[1] + velocity[1] + direction[1] * radius,
                        from[2] + velocity[2] + direction[2] * radius};
    const std::optional<WheelPieceHit> hit = sweep ? sweep(from, to) : std::nullopt;
    if (hit) {
        const Vector3 front = {from[0] + radius * direction[0], from[1] + radius * direction[1], from[2] + radius * direction[2]};
        const Vector3 toContact = {hit->point[0] - front[0], hit->point[1] - front[1], hit->point[2] - front[2]};
        const float remaining = (speed - length3(toContact)) * kWheelPieceBounce;
        for (int i = 0; i < 3; ++i) world[std::size_t(12 + i)] += toContact[std::size_t(i)];
        const float along = -2.0f * (velocity[0] * hit->normal[0] + velocity[1] * hit->normal[1] + velocity[2] * hit->normal[2]);
        for (int i = 0; i < 3; ++i) velocity[std::size_t(i)] = (velocity[std::size_t(i)] + hit->normal[std::size_t(i)] * along) * kWheelPieceBounce;
        if (speed < kWheelPieceRestSpeed && std::fabs(velocity[0]) < kWheelPieceRestSide && std::fabs(velocity[2]) < kWheelPieceRestSide) return true;
        float bounced = 0.0f;
        const Vector3 away = normalised(velocity, bounced);
        for (int i = 0; i < 3; ++i) world[std::size_t(12 + i)] += away[std::size_t(i)] * remaining;
        const float first = gameRandomUnit(random), second = gameRandomUnit(random), third = gameRandomUnit(random);
        spin = {(third - 0.5f) / 2.0f, (second - 0.5f) / 2.0f, (first - 0.5f) / 2.0f};
    } else {
        for (int i = 0; i < 3; ++i) world[std::size_t(12 + i)] += velocity[std::size_t(i)];
    }
    velocity[1] -= kWheelPieceFall;
    const assets::Matrix aroundCentre = assets::multiply(assets::multiply(translation({-centre[0], -centre[1], -centre[2]}), eulerRotation(spin)), translation(centre));
    world = assets::multiply(aroundCentre, world);
    return false;
}

bool wheelPieceMoving(const Vector3& velocity) {
    return velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2] > kWheelPieceMinSquare;
}

VehicleWreck breakVehicle(const assets::Matrix& vehicleNode, const std::vector<assets::Matrix>& wheelWorld, GameRandom& random) {
    VehicleWreck wreck;
    wreck.active = true;
    wreck.wheels = std::min(wheelWorld.size(), kWheelLaunchCount);
    for (std::size_t n = 0; n < wreck.wheels; ++n) {
        wreck.wheelWorld[n] = wheelWorld[n];
        wreck.launch[n] = wheelLaunchVector({vehicleNode[12], vehicleNode[13], vehicleNode[14]}, {wheelWorld[n][12], wheelWorld[n][13], wheelWorld[n][14]}, random);
    }
    return wreck;
}

void stepWreckTimer(VehicleWreck& wreck, float dt) {
    wreck.stateSeconds += dt;
    if (wreck.stateSeconds > kWreckSeconds) wreck.returnDue = true;
}

void restoreVehicleRecord(VehicleRecord& record) {
    record.hearts = record.health;
    record.healthPoints = kVehicleFullHealthPoints;
    record.debrisCooldown = 0.0f;
    record.destroyed = false;
    record.state = record.requestedState = kVehicleStateDriven;
    record.statePending = false;
}

bool vehicleReturnPlaceGood(std::size_t wheelsOnGround, std::size_t wheelCount, float upDot, float verticalSpeed, bool dead) {
    return wheelsOnGround >= wheelCount && std::cos(kReturnPlaceMaxTilt) <= upDot && kReturnPlaceMinVerticalSpeed <= verticalSpeed && !dead;
}

VehicleMatrixHistory::VehicleMatrixHistory(const assets::Matrix& placementStandIn) : return_(placementStandIn) { ring_.fill(placementStandIn); }

void VehicleMatrixHistory::record(const assets::Matrix& node) {
    ring_[index_ % kVehicleMatrixHistory] = node;
    ++index_;
    return_ = ring_[index_ % kVehicleMatrixHistory];
}

}
