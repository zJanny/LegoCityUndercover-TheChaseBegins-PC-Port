// Storyboard times of the audio scanner overlay (src/runtime/scanner_reticle.cpp). Rules read from the reticle update 0x2e9030, the zoom
// meter 0x263400 and the lock update 0x2ef4c8 (docs/re/abilities-scanner.md).
#include "runtime/scanner_reticle.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

void zoomMeter() {
    ScannerReticle reticle;
    ScannerReticleInput input;
    require(near(reticle.update(input).zoomMeterFraction, 0.0f), "zoom 43 (max) is the start of the meter (0x263400: 1 - (zoom - min) / (max - min))");
    input.zoom = 20.0f;
    require(near(reticle.update(input).zoomMeterFraction, 1.0f), "zoom 20 (min) is the end of the meter");
    input.zoom = 31.5f;
    require(near(reticle.update(input).zoomMeterFraction, 0.5f), "zoom 31.5 is the middle of the meter");
}

void idleAndLock() {
    ScannerReticle reticle;
    ScannerReticleInput input;
    ScannerReticlePose pose = reticle.update(input);
    require(pose.lockingOn.name == "Ret_LockingOn" && near(pose.lockingOn.fraction, 0.0f), "states 0..4 hold Ret_LockingOn at its start (0x2e9030 default branch)");
    require(pose.focus.name == "Ret_Focus" && near(pose.focus.fraction, 0.0f) && !pose.scanMeterShown, "Ret_Focus at the lock progress, no scan meter");
    require(near(pose.position[0], 200.0f) && near(pose.position[1], 120.0f), "the reticle starts at the screen centre");
    input.state = 1;
    input.hasTarget = true;
    input.targetScreen[0] = 100.0f;
    input.targetScreen[1] = 60.0f;
    input.lockProgress = 0.5f;
    pose = reticle.update(input);
    require(near(pose.focus.fraction, 0.5f), "Ret_Focus time follows the lock progress");
    require(near(pose.position[0], 150.0f) && near(pose.position[1], 90.0f), "state 1 moves the reticle from its last position towards the target by the lock progress (0x2ef4c8, 0x2119d4)");
    input.state = 2;
    input.lockProgress = 1.0f;
    pose = reticle.update(input);
    require(near(pose.position[0], 100.0f) && near(pose.position[1], 60.0f), "state 2 puts the reticle on the target");
    input.state = 3;
    input.hasTarget = false;
    pose = reticle.update(input);
    require(near(pose.position[0], 100.0f), "a lost lock leaves the reticle where it was (no write in states 3 and 4)");
}

void scanning() {
    ScannerReticle reticle;
    ScannerReticleInput input;
    input.state = 5;
    input.hasCurrent = true;
    input.currentScreen[0] = 220.0f;
    input.currentScreen[1] = 130.0f;
    input.scanProgress = 0.25f;
    ScannerReticlePose pose = reticle.update(input);
    require(pose.lockingOn.name == "Ret_LockingOn" && near(pose.lockingOn.fraction, 0.25f) && pose.focus.name.empty(), "state 5 sets Ret_LockingOn to the scan progress");
    require(pose.scanMeterShown && near(pose.scanMeterFraction, 0.25f), "the scan meter follows the scan progress");
    require(near(pose.position[0], 220.0f) && near(pose.position[1], 130.0f), "states 5..8 put the reticle on the scanned object");
    input.state = 8;
    input.scanProgress = 0.5f;
    pose = reticle.update(input);
    require(pose.focus.name == "Ret_Focus" && near(pose.focus.fraction, 0.5f), "state 8 sets Ret_Focus to the scan progress");
    input.hasCurrent = false;
    pose = reticle.update(input);
    require(near(pose.position[0], 200.0f) && near(pose.position[1], 120.0f), "without a scanned object the reticle returns to the screen centre");
}
}

int main() {
    try {
        zoomMeter();
        idleAndLock();
        scanning();
    } catch (const std::exception& error) {
        std::cerr << "scanner-reticle-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scanner-reticle-test: ok\n";
}
