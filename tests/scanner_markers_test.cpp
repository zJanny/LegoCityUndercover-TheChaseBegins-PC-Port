// Rules of the scanner marker update 0x2e9030 (REA pseudocode, docs/re/abilities-scanner.md): fade in at 2 per second, distance fade
// between 20 and 200 pixels from the screen centre, three pulse rings with speeds 1, 1.2, 1.4 that restart 0.3 s after all have ended.
// Storyboard lengths 0.459 (SoundCentre Pulsing) and 1.1 (SoundPulse Pulse) are read from the .bxaml layouts with the layout decoder.
#include "runtime/scanner_markers.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }
}

int main() {
    try {
        ScannerMarkers markers;
        std::vector<ScannerMarkerInput> records(2);
        records[0].visible = true;
        records[0].screen[0] = 200;
        records[0].screen[1] = 120;
        records[1].visible = false;
        const float dt = kScannerUpdateSeconds;
        auto pose = markers.update(dt, records);
        require(pose.size() == 2 && pose[0].shown && !pose[1].shown, "only the visible record shows a marker");
        require(near(pose[0].opacity, 0.0f), "a marker starts at opacity 0 (timer 0 when first shown)");
        for (int i = 0; i < 20; ++i) pose = markers.update(dt, records);
        require(near(pose[0].opacity, 1.0f), "the fade timer reaches 1 after 0.5 s at 2 per second");
        records[0].screen[0] = 200 + 20 + 90;
        records[0].screen[1] = 120;
        pose = markers.update(dt, records);
        require(near(pose[0].opacity, 0.5f), "110 pixels from the centre fades to 1 - 90 / 180");
        records[0].screen[0] = 200 + 400;
        pose = markers.update(dt, records);
        require(near(pose[0].opacity, 0.0f), "beyond 200 pixels the marker is invisible");

        records[0].screen[0] = 200;
        ScannerMarkers pulses;
        std::vector<ScannerMarkerPose> last;
        int restarts = 0;
        float previousFirst = -1;
        for (int i = 0; i < 200; ++i) {
            last = pulses.update(dt, records);
            if (last[0].pulseTime[0] < previousFirst) ++restarts;
            previousFirst = last[0].pulseTime[0];
            require(last[0].pulseTime[0] <= kScannerPulseSeconds, "a pulse never passes its storyboard length");
        }
        require(restarts >= 2, "the three rings restart after all have ended");
        ScannerMarkers speeds;
        for (int i = 0; i < 10; ++i) last = speeds.update(dt, records);
        require(last[0].pulseTime[0] > last[0].pulseTime[1] && last[0].pulseTime[1] > last[0].pulseTime[2] - 1e-6f, "later rings start later");
        std::cout << "scanner-markers-test: ok\n";
    } catch (const std::exception& error) {
        std::cerr << "scanner-markers-test: " << error.what() << '\n';
        return 1;
    }
}
