// Checks of the camera shake (start 0x22c500, manager update 0x23f174). Expected values are derived by hand from the
// disassembly: vector = byte * 0.1, offsets (r - 0.5) * vector drawn z, y, x, scale time / damp time, time -= dt.
#include "runtime/screen_shake.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }
}

int main() {
    try {
        GameRandom reference(1234), random(1234);
        float offset[3];
        ScreenShake shake;
        shake.update(0.1f, random, offset);
        require(offset[0] == 0 && offset[1] == 0 && offset[2] == 0, "no shake before a start");
        require(random.next() == GameRandom(1234).next(), "an idle update draws nothing");
        random = GameRandom(1234);

        shake.start(50, 30, 10, false, false, 0.25f);
        const float rz = gameRandomUnit(reference), ry = gameRandomUnit(reference), rx = gameRandomUnit(reference);
        shake.update(0.1f, random, offset);
        require(near(offset[2], (rz - 0.5f) * 1.0f) && near(offset[1], (ry - 0.5f) * 3.0f) && near(offset[0], (rx - 0.5f) * 5.0f),
                "offset = (r - 0.5) * byte * 0.1, drawn z, y, x");
        require(near(shake.secondsLeft(), 0.15f), "time decreases by the update time");

        shake.update(0.2f, random, offset);
        require(near(shake.secondsLeft(), -0.05f), "time is not clamped at zero");
        shake.update(0.1f, random, offset);
        require(offset[0] == 0 && offset[1] == 0 && offset[2] == 0, "no offset once the time ran out");

        ScreenShake damped;
        GameRandom a(77), b(77);
        damped.start(100, 100, 100, true, false, 1.0f);
        damped.update(0.5f, a, offset);
        const float z0 = (gameRandomUnit(b) - 0.5f) * 10.0f;
        require(near(offset[2], z0), "first update scale is time / damp time = 1");
        gameRandomUnit(b);
        gameRandomUnit(b);
        damped.update(0.25f, a, offset);
        const float z1 = (gameRandomUnit(b) - 0.5f) * 10.0f * 0.5f;
        require(near(offset[2], z1), "damped offset is scaled by the remaining time");

        ScreenShake locked;
        locked.start(10, 10, 10, false, true, 2.0f);
        locked.start(99, 99, 99, false, false, 5.0f);
        require(near(locked.secondsLeft(), 2.0f), "a running shake with the lock byte ignores a new start");
        ScreenShake unlocked;
        unlocked.start(10, 10, 10, false, false, 2.0f);
        unlocked.start(99, 99, 99, false, false, 5.0f);
        require(near(unlocked.secondsLeft(), 5.0f), "without the lock byte a new start replaces the shake");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}
