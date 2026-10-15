// Hop of the unbuilt bricks (src/runtime/buildit_wobble.cpp). The start values and the expected rows are the brick records (+0 amplitude,
// +4 speed, +8 phase, +0xc offset) of 1-1_BI_DonutVanParts read in the oracle after every update with the player standing in the use zone, state
// m11-4-van-parts, 40 updates idle (command `memtrace 40 brick 0x08e114e0:0x558 ...`, docs/re/buildit.md, entry "Hop of the unbuilt bricks").
#include "runtime/buildit_wobble.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
using namespace chase::runtime;

int failures = 0;

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool near(float a, float b, float tolerance = 2e-5f) { return std::fabs(a - b) <= tolerance; }

const UnitRandom noRandom = [] { return 0.0f; };

void plainHop() {
    BuilditWobble wobble({{0.361096203f, 20.749054f, 67.8738403f, 0.264624804f}});
    const std::vector<bool> may{true};
    const float phase[] = {88.6228943f, 109.371948f, 130.121002f, 150.870056f};
    const float offset[] = {0.334503919f, 0.360991895f, 0.340652794f, 0.276124865f};
    for (int i = 0; i < 4; ++i) {
        wobble.update(true, may, noRandom);
        require(near(wobble.bricks()[0].phase, phase[i], 1e-3f), "phase after update " + std::to_string(i + 1) + " (oracle brick 0)");
        require(near(wobble.offset(0), offset[i]), "offset after update " + std::to_string(i + 1) + " (oracle brick 0)");
    }
}

void crossing() {
    BuilditWobble wobble({{0.43334505f, 24.4050598f, 113.292389f, 0.433263361f}});
    const std::vector<bool> may{true};
    std::vector<float> draws{0.25f, 61718.0f / 65536.0f, 1126.0f / 65536.0f};
    std::size_t next = 0;
    const UnitRandom scripted = [&] { return next < draws.size() ? draws[next++] : 0.0f; };
    wobble.update(true, may, scripted);
    wobble.update(true, may, scripted);
    require(next == 0, "no draw before the phase passes 180");
    const std::vector<WobbleSound> sounds = wobble.update(true, may, scripted);
    require(next == 3, "the crossing draws the pitch, the amplitude and the speed in this order");
    require(sounds.size() == 1 && near(sounds[0].pitch, 1.1f - 0.25f * 0.2f), "the first crossing makes the sound with pitch 1.1 - 0.2 r");
    const BrickWobble& brick = wobble.bricks()[0];
    require(near(brick.phase, 6.50756836f, 1e-3f), "phase falls by 180 (oracle row 3)");
    require(brick.amplitude == 0.482522607f, "amplitude 0.2 + 0.3 r (oracle row 3, r = 61718 / 65536)");
    require(brick.speed == 15.171814f, "speed 15 + 10 r (oracle row 3, r = 1126 / 65536)");
    require(near(wobble.offset(0), 0.133173347f), "the offset of the crossing update uses the old amplitude");
}

void soundsLimited() {
    std::vector<BrickWobble> bricks(5, BrickWobble{0.3f, 20.0f, 179.0f, 0});
    BuilditWobble wobble(bricks);
    const std::vector<bool> may(5, true);
    const std::vector<WobbleSound> sounds = wobble.update(true, may, noRandom);
    require(sounds.size() == 3 && sounds[0].brick == 0 && sounds[2].brick == 2, "only the first three crossings of an update make the sound");
}

void conditionFalse() {
    BuilditWobble wobble({{0.36f, 20.0f, 67.0f, 0.3f}, {0.24f, 18.0f, 120.0f, 0.2f}});
    const std::vector<bool> may{true, true};
    wobble.update(false, may, noRandom);
    require(wobble.offset(0) == 0.3f && wobble.offset(1) == 0.2f && wobble.bricks()[0].phase == 67.0f, "never hopped: nothing changes (oracle: phase frozen when the player is away)");
    wobble.update(true, may, noRandom);
    const float phase = wobble.bricks()[0].phase;
    wobble.update(false, may, noRandom);
    require(wobble.offset(0) == 0.0f && wobble.offset(1) == 0.0f && wobble.bricks()[0].phase == phase, "the bricks that hopped go back to offset 0 and keep their phase (oracle: sin 0.0, phase frozen)");
}

void busyBricksStay() {
    BuilditWobble wobble({{0.36f, 20.0f, 67.0f, 0.3f}, {0.24f, 18.0f, 120.0f, 0.2f}});
    wobble.update(true, {true, false}, noRandom);
    require(wobble.offset(1) == 0.0f && wobble.bricks()[1].phase == 120.0f, "a brick that is building or built does not hop");
}

void startValues() {
    float draws[] = {0.5f, 0.5f, 0.5f, 1.0f / 65536.0f, 0, 0};
    std::size_t next = 0;
    const BuilditWobble wobble(2, [&] { return draws[next++]; });
    require(wobble.bricks()[0].phase == 90.0f && near(wobble.bricks()[0].amplitude, 0.35f) && wobble.bricks()[0].speed == 20.0f, "start values: phase 180 r, amplitude 0.2 + 0.3 r, speed 15 + 10 r");
    require(near(wobble.bricks()[1].phase, 180.0f / 65536.0f, 1e-6f) && wobble.bricks()[1].amplitude == 0.2f && wobble.bricks()[1].speed == 15.0f, "the second brick draws its own three values");
}
}

int main() {
    plainHop();
    crossing();
    soundsLimited();
    conditionFalse();
    busyBricksStay();
    startValues();
    if (failures) return EXIT_FAILURE;
    std::cout << "buildit-wobble-test: ok\n";
    return EXIT_SUCCESS;
}
