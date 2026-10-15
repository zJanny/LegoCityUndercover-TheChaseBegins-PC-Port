#pragma once
// The pose of a wheel node of a vehicle: the pose function 0x33929c of the physics body (docs/re/vehicles-physics.md). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_body.hpp"
#include "runtime/vehicle_drivetrain.hpp"

#include <array>
#include <cstddef>

namespace chase::runtime {

// body+0xdb0, set to 1 by the body constructor 0x33ce74 and not written by the loaders: the factor of the steer angle in the wheel pose.
constexpr float kWheelPoseSteerFactor = 1.0f;

// The matrix of the physics body (rows right, up, forward in the world, translation = the centre of mass): the matrix 0x33c3e8 gives the
// wheels, in contrast to the node matrix, which sits CentreOfMass behind it.
assets::Matrix bodyOriginMatrix(const VehicleRigidBody& body);

// 0x33929c for wheel `index`: the rotation about X by the spin angle (negated for a negative mount x) and about Y by the steer angle (the two
// front wheels only) plus pi for a negative mount x, at (mount x, mount y - (distance - radius), mount z), times the body matrix.
// `distance` is w+0xe0, the distance from the wheel centre to the ground hit of the last step.
assets::Matrix wheelPoseMatrix(const WheelSuspension& suspension, const WheelSpinState& spin, std::size_t index, float distance,
                               const assets::Matrix& bodyMatrix);

}
