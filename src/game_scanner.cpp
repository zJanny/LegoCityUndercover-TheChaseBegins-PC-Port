#include "game_level.hpp"
#include "runtime/follow_camera.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
unsigned clipUpdates(const game_player::PlayerRig& player, int role, unsigned fallback) {
    const auto found = player.animations.find(role);
    if (found == player.animations.end()) return fallback;
    return unsigned(std::ceil(float(assets::animationLength(found->second)) * 30.0f / assets::animationFramesPerSecond(found->second) - 1e-3f));
}

const char* phaseName(runtime::ScannerPhase phase) {
    switch (phase) {
    case runtime::ScannerPhase::Idle: return "idle";
    case runtime::ScannerPhase::TakingOut: return "taking-out";
    case runtime::ScannerPhase::Entering: return "entering";
    case runtime::ScannerPhase::Active: return "active";
    case runtime::ScannerPhase::Exiting: return "exiting";
    }
    return "?";
}
}

void LevelSession::updateScanner() {
    const unsigned calls = levelScripts.objectives().recordedCalls(runtime::ObjectiveNativeKind::EnableScanner);
    if (calls != scannerCallsSeen) {
        scannerCallsSeen = calls;
        scanner.requestEnabled(levelScripts.objectives().scannerFlag());
    }
    runtime::ScannerInput input;
    input.dt = 1.0f / 30.0f;
    input.player[0] = player->motion.x;
    input.player[1] = player->vertical.y;
    input.player[2] = player->motion.z;
    input.facing = player->motion.facing;
    input.playerFree = !levelScripts.playerControlTaken() && !cutscenePlaying() && !carry.busy() && carry.carryingMode() == 0 &&
                       player->vertical.posture == runtime::Posture::Ground;
    input.usePressed = pad.slot(kSlotConfirmButton).press > 0;
    if (!scanner.zones().empty()) {
        const std::string& zone = scanner.zone() >= 0 ? scanner.zones()[std::size_t(scanner.zone())].instance : scanner.zones().front().instance;
        input.zoneEnabled = !objectState.isDisabled(zone);
        const runtime::ObjectNativeFlags* flags = levelScripts.objectNatives().flags(zone);
        input.usable = !flags || !flags->usable || *flags->usable;
    }
    input.exitPressed = pad.slot(kSlotJumpButton).press > 0;
    input.scanHeld = pad.slot(6).value;
    input.cancelPressed = pad.slot(kSlotButtonY).press > 0;
    input.stickX = pad.channel(0).value;
    input.stickY = pad.channel(1).value;
    input.dpadUp = pad.slot(0).value != 0.0f;
    input.dpadDown = pad.slot(1).value != 0.0f;
    input.dpadLeft = pad.slot(2).value != 0.0f;
    input.dpadRight = pad.slot(3).value != 0.0f;
    input.shoulderL = pad.slot(kSlotLeftShoulder).value;
    input.shoulderR = pad.slot(kSlotRightShoulder).value;
    input.takeOutUpdates = clipUpdates(*player, runtime::kAnimationScannerOut, input.takeOutUpdates);
    input.enabled = [this](const std::string& instance) { return !objectState.isDisabled(instance); };
    if (shared.audio) input.soundPlaying = [this](std::uint32_t id) { return shared.audio->playing(id, std::uint16_t(runtime::kScannerSoundOwner)); };
    const runtime::ScannerPhase before = scanner.phase();
    for (const runtime::ScannerEvent& event : scanner.update(input)) {
        switch (event.kind) {
        case runtime::ScannerEventKind::Trigger:
            std::cout << "scanner fired " << event.instance << " at update " << updates << '\n';
            levelScripts.triggerObjectEnabling(event.instance);
            break;
        case runtime::ScannerEventKind::Sound:
            if (shared.audio) shared.audio->applySound(event.sound);
            break;
        case runtime::ScannerEventKind::PlayerPose:
            player->motion.x = scanner.playerPosition()[0];
            player->vertical.y = scanner.playerPosition()[1];
            player->motion.z = scanner.playerPosition()[2];
            player->motion.facing = player->motion.lastIntent = scanner.playerFacing();
            break;
        case runtime::ScannerEventKind::PlayerAnimation:
            break;
        }
    }
    if (scanner.phase() != before) std::cout << "scanner " << phaseName(scanner.phase()) << " at update " << updates << '\n';
}

bool LevelSession::scannerView(float eye[3], float& yaw, float& pitch, float& fieldOfView) const {
    if (!scanner.viewShown()) return false;
    const runtime::ScannerView view = scanner.view();
    for (int a = 0; a < 3; ++a) eye[a] = view.eye[a];
    yaw = view.yaw;
    pitch = view.pitch;
    fieldOfView = runtime::FollowCamera::verticalFieldOfView(view.zoomDegrees);
    return true;
}

}
