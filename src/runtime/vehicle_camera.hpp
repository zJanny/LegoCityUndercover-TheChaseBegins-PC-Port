#pragma once
// What the camera follows while the player drives (0x1597c8): the InVehicle preset block 0x376fdc with the vehicle record's ATTR_Camera*
// floats written into it, the top of the vehicle's collision box (entity +0x64 + +0x70, the "body height" of the input provider 0x32f334) and
// the half height of the model bounds (node +0xb0, read by 0x330e4c for vehicle presets). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/vehicle.hpp"

namespace chase::runtime {

struct VehicleCameraSubject {
    FollowCameraPreset preset;
    float bodyHeight = 0;
    float modelHalfHeight = 0;
};

// Vehicle record types (+0x2ec) whose preset has the automatic turn switched off (0x1597c8 clears block +0x38).
bool vehicleTypeKeepsCameraAhead(std::uint16_t vehicleType);

// `model` is the vehicle's model: its first collision item gives the box, its meshes the bounds.
VehicleCameraSubject vehicleCameraSubject(const assets::Model& model, const VehicleRecord& record);

// The preset the camera follows with while the player stands on foot (the OnFoot block 0x376f6c).
VehicleCameraSubject onFootCameraSubject();

}
