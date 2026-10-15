#include "runtime/vehicle_camera.hpp"
#include <algorithm>

namespace chase::runtime {

bool vehicleTypeKeepsCameraAhead(std::uint16_t vehicleType) {
    return vehicleType == 0x2b || vehicleType == 0x2d || vehicleType == 0x2e || vehicleType == 0x2f;
}

VehicleCameraSubject vehicleCameraSubject(const assets::Model& model, const VehicleRecord& record) {
    VehicleCameraSubject subject;
    subject.preset = FollowCameraPreset::inVehicle();
    if ((record.flags311 & kVehicleCameraOverride) != 0)
        subject.preset.withVehicleOverrides(record.cameraPosY, record.cameraPosZ, record.cameraLookY, record.cameraLookZ);
    if (vehicleTypeKeepsCameraAhead(record.vehicleType)) subject.preset.autoTurnGain = 0.0f;
    if (!model.collision.empty() && !model.collision.front().nodes.empty()) {
        const assets::CollisionNode& root = model.collision.front().nodes.front();
        subject.bodyHeight = root.centre[1] + root.half[1];
    }
    if (!model.meshes.empty()) {
        float low = 1.0e30f, high = -1.0e30f;
        for (const assets::ModelMesh& mesh : model.meshes) {
            low = std::min(low, mesh.centre[1] - mesh.half[1]);
            high = std::max(high, mesh.centre[1] + mesh.half[1]);
        }
        subject.modelHalfHeight = (high - low) * 0.5f;
    }
    return subject;
}

VehicleCameraSubject onFootCameraSubject() {
    VehicleCameraSubject subject;
    subject.preset = FollowCameraPreset::onFoot();
    subject.bodyHeight = kPlayerBodyHeight;
    return subject;
}

}
