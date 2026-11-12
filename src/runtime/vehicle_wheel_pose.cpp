#include "runtime/vehicle_wheel_pose.hpp"

#include <cmath>

namespace chase::runtime {

namespace {

constexpr float kPi = 3.14159274f;

void rotateY(assets::Matrix& m, float angle) {
    const float s = std::sin(angle), c = std::cos(angle);
    for (std::size_t k = 0; k < 3; ++k) {
        const float row0 = m[k], row2 = m[8 + k];
        m[k] = row0 * c - row2 * s;
        m[8 + k] = row2 * c + row0 * s;
    }
}

void rotateX(assets::Matrix& m, float angle) {
    const float s = std::sin(angle), c = std::cos(angle);
    for (std::size_t k = 0; k < 3; ++k) {
        const float row1 = m[4 + k], row2 = m[8 + k];
        m[4 + k] = row1 * c + row2 * s;
        m[8 + k] = row2 * c - row1 * s;
    }
}

}

assets::Matrix bodyOriginMatrix(const VehicleRigidBody& body) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    assets::Matrix matrix{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t k = 0; k < 3; ++k) matrix[row * 4 + k] = axes[row][k];
    for (std::size_t k = 0; k < 3; ++k) matrix[12 + k] = body.position[k];
    matrix[15] = 1.0f;
    return matrix;
}

assets::Matrix wheelPoseMatrix(const WheelSuspension& suspension, const WheelSpinState& spin, std::size_t index, float distance,
                               const assets::Matrix& bodyMatrix) {
    assets::Matrix local = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float spinAngle = spin.angle;
    float turn = 0.0f;
    if (suspension.mount[0] < 0.0f) {
        spinAngle = -spinAngle;
        turn = kPi;
    }
    if (index < 2) turn += spin.steerAngle * kWheelPoseSteerFactor;
    if (turn != 0.0f) rotateY(local, turn);
    rotateX(local, spinAngle);
    local[12] = suspension.mount[0];
    local[13] = suspension.mount[1] - (distance - suspension.radius);
    local[14] = suspension.mount[2];
    return assets::multiply(local, bodyMatrix);
}

assets::Matrix kinematicWheelPoseMatrix(float spinAngle, float mountX, const std::array<float, 3>& localPosition, const assets::Matrix& nodeMatrix) {
    assets::Matrix local = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    if (mountX < 0.0f) {
        spinAngle = -spinAngle;
        rotateY(local, kPi);
    }
    rotateX(local, spinAngle);
    local[12] = localPosition[0];
    local[13] = localPosition[1];
    local[14] = localPosition[2];
    return assets::multiply(local, nodeMatrix);
}

}
