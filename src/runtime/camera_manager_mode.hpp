#pragma once
// The mode slots of the camera manager 0x376870 (docs/re/script-natives-audio.md, CAMERA-22; docs/re/camera-vehicle.md, CAMERA-40): +0x28 the
// requested mode that 0x226a30 stores, +0x2c the current one, +0x30 the previous one. The manager update 0x23f174 switches when the request differs
// from the current mode (leave of the current mode, current to previous, the request becomes current, enter); a request equal to the current mode does
// nothing. The setters BulletTime's restore calls (docs/re/scripts-missions-1-4-roof.md): 0x221054 requests `*(0x3770c8 + 0x10)`, the gameplay camera
// object 0x377110 in the oracle, 0x1fc060 requests `*0x372d74`, the preset object of CameraRotFixedSet. No SDL dependency.
#include "runtime/bullet_time_scale.hpp"

#include <optional>

namespace chase::runtime {

enum class CameraMode : std::uint8_t { Gameplay, RotFixedPreset };

struct CameraModeSwitch {
    CameraMode from = CameraMode::Gameplay;
    CameraMode to = CameraMode::Gameplay;
};

class CameraManagerModes {
public:
    // 0x226a30: manager +0x28 = mode. The level start passes the gameplay camera (0x182170), so a level begins with it requested and current.
    void request(CameraMode mode) { requested_ = mode; }
    // The switch of 0x23f174, once per camera update.
    std::optional<CameraModeSwitch> update();
    CameraMode requested() const { return requested_; }
    CameraMode current() const { return current_; }
    std::optional<CameraMode> previous() const { return previous_; }

private:
    CameraMode requested_ = CameraMode::Gameplay;
    CameraMode current_ = CameraMode::Gameplay;
    std::optional<CameraMode> previous_;
};

// The mode the restore's call requests: 0x221054 the gameplay camera, 0x1fc060 the CameraRotFixedSet preset; none for BulletTimeModeCall::None.
std::optional<CameraMode> bulletTimeRequestedMode(BulletTimeModeCall call);

const char* cameraModeName(CameraMode mode);

}
