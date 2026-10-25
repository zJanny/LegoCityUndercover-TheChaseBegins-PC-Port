// The sound scanner (src/runtime/scanner.cpp) on a made-up zone and target, no game data needed. Values and their sources: the settings block
// 0x1e06ac (read at 0x522440 in the oracle save state on the roof at the sound scanner trigger of mission 1-2), the audio sub-mode values of 0x2ef814 (read at 0x5216ac+0x34 / +0xcd0..+0xcf0), the
// state timing of the oracle runs s1/s3/s4 in docs/re/abilities-scanner.md (scanner_out ends the state 0x156, EnterScan 3 updates after the
// entry of state 0x157, toggle 4 updates after it, camera 6 updates after it).
#include "runtime/scanner.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool fired(const std::vector<ScannerEvent>& events, const std::string& instance) {
    for (const ScannerEvent& event : events)
        if (event.kind == ScannerEventKind::Trigger && event.instance == instance) return true;
    return false;
}

bool played(const std::vector<ScannerEvent>& events, std::uint32_t id) {
    for (const ScannerEvent& event : events)
        if (event.kind == ScannerEventKind::Sound && event.sound.kind == SoundRequest::Kind::Play && event.sound.id == id) return true;
    return false;
}

Scanner makeScanner() {
    Scanner scanner;
    ScannerZone zone;
    zone.instance = "zone";
    zone.yawDegrees = 90.0f;
    zone.hasUseBound = true;
    zone.useCentre[1] = 1.0f;
    zone.useHalf[0] = 1.0f;
    zone.useHalf[1] = 1.5f;
    zone.useHalf[2] = 1.0f;
    zone.mode = 1;
    zone.enterTrigger = "enter";
    zone.lockedOnTrigger = "locked";
    zone.zoomedInTrigger = "zoomed";
    zone.exitTrigger = "exit";
    scanner.add(zone);
    ScannableObject dog;
    dog.instance = "dog";
    dog.point[0] = 20.0f;
    dog.point[1] = 0.85f * kScannerEyeHeightFactor;
    dog.triggerScanned = "scanned";
    scanner.add(dog);
    return scanner;
}

ScannerInput standing() {
    ScannerInput input;
    input.player[0] = 0.5f;
    input.player[2] = 0.3f;
    return input;
}

void enterAndLock() {
    Scanner scanner = makeScanner();
    ScannerInput input = standing();
    input.player[0] = 1.5f;
    input.usePressed = true;
    scanner.update(input);
    require(scanner.phase() == ScannerPhase::Idle, "a press outside UseBound (half 1 around the zone) does nothing");
    input = standing();
    input.usePressed = true;
    input.usable = false;
    scanner.update(input);
    require(scanner.phase() == ScannerPhase::Idle, "ToggleUseable 0 keeps the zone from answering message 3");
    input.usable = true;
    std::vector<ScannerEvent> events = scanner.update(input);
    require(scanner.phase() == ScannerPhase::TakingOut, "the A press in the zone pushes state 0x156");
    bool animation = false;
    for (const ScannerEvent& event : events) animation = animation || (event.kind == ScannerEventKind::PlayerAnimation && event.animation == 0x208);
    require(animation, "state 0x156 plays role 0x208 (scanner_out, 0x322ba8)");
    input = standing();
    unsigned turned = 0;
    for (unsigned i = 1; i < kScannerTakeOutUpdates; ++i) {
        input.player[0] = scanner.playerPosition()[0];
        input.player[2] = scanner.playerPosition()[2];
        input.facing = scanner.playerFacing();
        scanner.update(input);
        if (turned == 0 && scanner.playerFacing() == 16384) turned = i;
        require(scanner.phase() == ScannerPhase::TakingOut, "still taking the scanner out at update " + std::to_string(i));
    }
    require(turned == 7, "the facing turns 0x800 per update from 0 to 90 degrees (0x2248c4), the first step in the press update: " + std::to_string(turned));
    require(std::fabs(scanner.playerPosition()[0]) < 0.01f && std::fabs(scanner.playerPosition()[2]) < 0.01f, "the player is moved onto the zone");
    events = scanner.update(input);
    require(scanner.phase() == ScannerPhase::Entering && scanner.active(), "49 updates from the press enter state 0x157 (oracle B-1: 4894 clock ticks from the toggle clear)");
    require(played(events, kScannerSoundEnter), "0x1bf2d0 plays sound 0x19");
    for (unsigned i = 1; i < kScannerEnterCallbackUpdates; ++i) require(!fired(scanner.update(input), "enter"), "no EnterScan before the callback");
    events = scanner.update(input);
    require(fired(events, "enter") && scanner.phase() == ScannerPhase::Active, "EnterScanTriggerObject fires in 0x2bee98, 20 updates after the entry (oracle B-1: 1978 ticks)");
    for (unsigned i = kScannerEnterCallbackUpdates + 1; i < kScannerCameraStartUpdates; ++i) scanner.update(input);
    require(scanner.records().empty(), "no camera before update 56 after the entry (oracle B-1: yaw valid with the toggle byte, 3619 ticks after the callback)");
    events = scanner.update(input);
    require(std::fabs(scanner.zoom() - 43.0f) < 1e-6f, "the camera starts at zoom 43 ([5])");
    require(scanner.records().size() == 1 && scanner.records()[0].visible, "the dog 20 ahead of the view is visible");
    require(scanner.target() == 0 && scanner.targetState() == 1, "the target in the reticle starts the lock (state 1)");
    unsigned lockUpdates = 0;
    for (unsigned i = 1; i < 40 && lockUpdates == 0; ++i)
        if (fired(scanner.update(input), "locked")) lockUpdates = i;
    require(lockUpdates == 20, "the lock grows by 1.5 per second (+0xcf0) and fires LockedOnTriggerObject: " + std::to_string(lockUpdates));
    require(scanner.targetState() == 2, "locked (state 2)");
    input.scanHeld = 1.0f;
    scanner.update(input);
    require(scanner.targetState() == 5, "A held on the locked target starts the scan (state 5)");
    unsigned scanUpdates = 0;
    for (unsigned i = 1; i < 60 && scanUpdates == 0; ++i)
        if (fired(scanner.update(input), "scanned")) scanUpdates = i;
    require(scanUpdates == 40 || scanUpdates == 41, "the scan grows by 0.75 per second (+0x34) above 1: " + std::to_string(scanUpdates));
    require(scanner.targetState() == 6, "scanned (state 6)");
    require(!fired(scanner.update(input), "scanned"), "TriggerScanned fires once (component flag bit 1)");
    input.scanHeld = 0;
    input.shoulderR = 1.0f;
    unsigned zoomUpdates = 0;
    for (unsigned i = 1; i < 40 && zoomUpdates == 0; ++i)
        if (fired(scanner.update(input), "zoomed")) zoomUpdates = i;
    require(zoomUpdates == 20, "R zooms 35 per second from 43 to 20 and fires ZoomedInTriggerObject: " + std::to_string(zoomUpdates));
    require(fired(scanner.update(input), "zoomed"), "ZoomedInTriggerObject fires in every update at full zoom (0x26f0b4)");
    input.shoulderR = 0;
    scanner.requestEnabled(false);
    scanner.update(input);
    require(scanner.phase() == ScannerPhase::Exiting, "EnableScanner 0 toggles the active scanner off (0x1cd9dc)");
    for (unsigned i = 1; i < kScannerExitCallbackUpdates; ++i) scanner.update(input);
    require(fired(scanner.update(input), "exit"), "ExitScanTriggerObject fires in the exit callback 0x2befc0 (SCAN-S1: placed at 20 updates)");
    for (unsigned i = kScannerExitCallbackUpdates + 1; i < kScannerExitEndUpdates; ++i) scanner.update(input);
    require(scanner.phase() == ScannerPhase::Exiting, "the exit transition runs 57 updates (oracle B-1: 5706 ticks)");
    scanner.update(input);
    require(scanner.phase() == ScannerPhase::Idle && !scanner.active(), "the end callback 0x1a2824 clears the active flag");
}

void cancelAndAim() {
    Scanner scanner = makeScanner();
    ScannerInput input = standing();
    input.player[0] = input.player[2] = 0;
    input.facing = 16384;
    input.usePressed = true;
    input.takeOutUpdates = 1;
    scanner.update(input);
    input.usePressed = false;
    for (unsigned i = 0; i < kScannerCameraStartUpdates + kScannerEnterCallbackUpdates; ++i) scanner.update(input);
    require(scanner.phase() == ScannerPhase::Active && !scanner.records().empty(), "active with a camera");
    input.stickX = 1.0f;
    scanner.update(input);
    const float expected = 60.0f * 0.017453294f / 30.0f;
    require(std::fabs(scanner.yawOffset() - expected) < 1e-6f, "full stick turns 60 degrees per second at zoom 43 ([8], 0x26f0b4)");
    input.stickX = 0;
    input.stickY = 1.0f;
    for (int i = 0; i < 60; ++i) scanner.update(input);
    require(std::fabs(scanner.pitchOffset() + 40.0f * 0.017453294f) < 1e-5f, "stick up stops at 40 degrees ([2])");
    input.stickY = -1.0f;
    for (int i = 0; i < 90; ++i) scanner.update(input);
    require(std::fabs(scanner.pitchOffset() - 25.0f * 0.017453294f) < 1e-5f, "stick down stops at 25 degrees ([1])");
    input.stickY = 0;
    input.exitPressed = true;
    scanner.update(input);
    require(scanner.phase() == ScannerPhase::Exiting, "B leaves the scanner (0x187f78)");
}

}

int main() {
    try {
        enterAndLock();
        cancelAndAim();
    } catch (const std::exception& error) {
        std::cerr << "scanner-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scanner-test: ok\n";
    return 0;
}
