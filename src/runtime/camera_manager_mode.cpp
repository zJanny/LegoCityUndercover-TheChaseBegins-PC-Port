#include "runtime/camera_manager_mode.hpp"

namespace chase::runtime {

std::optional<CameraModeSwitch> CameraManagerModes::update() {
    if (requested_ == current_) return std::nullopt;
    const CameraModeSwitch change{current_, requested_};
    previous_ = current_;
    current_ = requested_;
    return change;
}

std::optional<CameraMode> bulletTimeRequestedMode(BulletTimeModeCall call) {
    switch (call) {
    case BulletTimeModeCall::GameplayRecords: return CameraMode::Gameplay;
    case BulletTimeModeCall::RotFixedPreset: return CameraMode::RotFixedPreset;
    case BulletTimeModeCall::None: break;
    }
    return std::nullopt;
}

const char* cameraModeName(CameraMode mode) {
    return mode == CameraMode::RotFixedPreset ? "rot fixed preset" : "gameplay";
}

}
