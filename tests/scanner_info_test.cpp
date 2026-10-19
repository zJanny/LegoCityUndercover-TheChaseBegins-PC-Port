// State machine, box choice, opacity and icon rule of the scanner info boxes (src/runtime/scanner_info.cpp). Values from docs/re/abilities-scanner-overlay.md
// (llvm-objdump of 0x2d2e14, 0x1f3494 and 0x2ad730; constants read from the literal pools 0x2d3130..0x2d313c and 0x1f35a0..0x1f35b0).
#include "runtime/scanner_info.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        require(scannerInfoIcon(13, false) == 13 && scannerInfoIcon(5, true) == 5 && scannerInfoIcon(5, false) == 0, "icon 5 needs ability bit 28, else index 0 (0x2ad730)");
        require(kScannerInfoSwitchLine == 105.0f, "switch line is half the screen height minus 15 (0x1f3494)");

        ScannerInfo info;
        ScannerInfoRecord dog;
        dog.screen[0] = 150.0f;
        dog.screen[1] = 138.0f;
        dog.distanceSquared = 50.0f * 50.0f + 18.0f * 18.0f;
        dog.icon = 13;
        ScannerInfoInput input;
        input.reticleY = 138.0f;
        for (int i = 0; i < 5; ++i) require(info.update(input).box == ScannerInfoBox::None, "no box before the scan");

        input.state = 6;
        input.current = &dog;
        input.target = &dog;
        ScannerInfoPose pose = info.update(input);
        require(info.phase() == 1 && pose.box == ScannerInfoBox::None, "state 6 entered: phase 1, nothing shown yet");
        pose = info.update(input);
        require(info.phase() == 2 && pose.box == ScannerInfoBox::Top && !pose.leaving && pose.icon == 13, "phase 1 shows the top box (reticle below 125) with the icon");
        require(std::fabs(pose.opacity - (1.0f - (dog.distanceSquared - 625.0f) / 2975.0f)) < 1e-5f, "opacity falls from 1 at 625 to 0 at 3600");
        require(pose.offset[0] == -50.0f && pose.offset[1] == 18.0f, "offset is the screen position minus the centre");
        for (int i = 0; i < 40; ++i) pose = info.update(input);
        require(info.phase() == 3, "InfoOn (0.7 s) has ended: phase 3");

        input.reticleY = 100.0f;
        pose = info.update(input);
        require(pose.box == ScannerInfoBox::Top, "hysteresis: top stays chosen above 85");
        input.reticleY = 80.0f;
        pose = info.update(input);
        require(pose.box == ScannerInfoBox::Bottom, "below 85 the bottom box is chosen");

        input.state = 7;
        input.reticleY = 200.0f;
        pose = info.update(input);
        require(info.phase() == 4 && pose.box == ScannerInfoBox::Bottom, "scan state leaves 6: phase 4, the hysteresis 1000 keeps the box (0x1f35b0)");
        pose = info.update(input);
        require(info.phase() == 5 && pose.leaving, "InfoOff plays");
        for (int i = 0; i < 10; ++i) pose = info.update(input);
        require(info.phase() == 0 && pose.box == ScannerInfoBox::None, "InfoOff (0.2 s) has ended: boxes hidden, phase 0");

        ScannerInfoRecord close;
        close.distanceSquared = 400.0f;
        ScannerInfo second;
        input.state = 6;
        input.current = input.target = &close;
        input.reticleY = 200.0f;
        second.update(input);
        require(second.update(input).opacity == 1.0f, "opacity 1 below 625");
        close.distanceSquared = 5000.0f;
        require(second.update(input).opacity == 0.0f && second.phase() == 2, "opacity clamped at 0");
        for (int i = 0; i < 40; ++i) second.update(input);
        input.state = 0;
        for (int i = 0; i < 20; ++i) second.update(input);
        require(second.phase() == 0, "the top box has gone");
        input.state = 6;
        input.reticleY = 110.0f;
        second.update(input);
        require(second.update(input).box == ScannerInfoBox::Bottom, "hiding clears the last choice (+0x11 = 0), so the line is 125 again");
    } catch (const std::exception& error) {
        std::cerr << "scanner-info-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scanner-info-test: ok\n";
    return 0;
}
