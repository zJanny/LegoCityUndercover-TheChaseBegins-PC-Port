// The camera manager's mode slots and the modes BulletTime's restore requests (runtime/camera_manager_mode.hpp). Rules from llvm-objdump of 0x226a30,
// 0x221054 and 0x1fc060 (20261112-enemy-E, 2026-11-12) and the reading of 0x23f174 in docs/re/script-natives-audio.md (CAMERA-22); "a request
// equal to the current mode does nothing" is the oracle run of CAMERA-40 (docs/re/camera-vehicle.md).
#include "runtime/camera_manager_mode.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void levelStartsWithTheGameplayCamera() {
    CameraManagerModes modes;
    require(modes.requested() == CameraMode::Gameplay && modes.current() == CameraMode::Gameplay && !modes.previous(), "0x182170 at the level start");
    modes.request(CameraMode::Gameplay);
    require(!modes.update(), "a request equal to the current mode does nothing");
}

void switchMovesCurrentToPrevious() {
    CameraManagerModes modes;
    modes.request(CameraMode::RotFixedPreset);
    require(modes.requested() == CameraMode::RotFixedPreset && modes.current() == CameraMode::Gameplay, "0x226a30 only stores +0x28");
    const std::optional<CameraModeSwitch> change = modes.update();
    require(change && change->from == CameraMode::Gameplay && change->to == CameraMode::RotFixedPreset, "0x23f174 switches");
    require(modes.current() == CameraMode::RotFixedPreset && modes.previous() == CameraMode::Gameplay, "current to previous");
    require(!modes.update(), "one switch per request");
    modes.request(CameraMode::Gameplay);
    require(modes.update() && modes.current() == CameraMode::Gameplay && modes.previous() == CameraMode::RotFixedPreset, "and back");
}

void bulletTimeRestoreCalls() {
    require(bulletTimeRequestedMode(bulletTimeModeCall(0)) == CameraMode::Gameplay, "mode 0: 0x221054");
    require(bulletTimeRequestedMode(bulletTimeModeCall(4)) == CameraMode::Gameplay, "mode 4: 0x221054");
    require(bulletTimeRequestedMode(bulletTimeModeCall(3)) == CameraMode::RotFixedPreset, "mode 3: 0x1fc060");
    require(!bulletTimeRequestedMode(bulletTimeModeCall(kBulletTimeNoMode)), "mode 255: no call");
    require(!bulletTimeRequestedMode(bulletTimeModeCall(1)), "mode 1: no call");
}
}

int main() {
    try {
        levelStartsWithTheGameplayCamera();
        switchMovesCurrentToPrevious();
        bulletTimeRestoreCalls();
    } catch (const std::exception& error) {
        std::cerr << "camera-manager-mode-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "camera-manager-mode-test ok\n";
    return 0;
}
