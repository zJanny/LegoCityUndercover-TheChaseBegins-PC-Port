#include "runtime/vehicle_body.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

float vehicleLinearDamping(float dampingLinear, float speed) {
    const float scaled = std::min(speed, kVehicleDampingSpeedCap) * kVehicleDampingSpeedScale;
    return std::clamp(dampingLinear * scaled * scaled, 0.0f, 1.0f);
}

void stepVehicleFall(VehicleFall& fall, float dampingLinear, float speed, float dt) {
    fall.velocity += kVehicleBodyGravity * dt;
    fall.velocity *= std::pow(1.0f - vehicleLinearDamping(dampingLinear, speed), dt);
    fall.height += fall.velocity * dt;
}

void settleOnGround(VehicleFall& fall, float groundTarget) {
    if (fall.height > groundTarget + kVehicleContactReach) return;
    fall.height = groundTarget;
    fall.velocity = 0.0f;
}

}
