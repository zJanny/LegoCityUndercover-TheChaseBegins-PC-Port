// Stud pickup rules (chase_runtime) on made-up studs. Every value pinned here comes from the original game:
// the provenance is next to it (docs/re/studs.md).
#include "runtime/studs.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

StudSpawn spawnAt(StudKind kind, float x, float y, float z, unsigned group = 0) {
    StudSpawn spawn;
    spawn.kind = kind;
    spawn.position[0] = x;
    spawn.position[1] = y;
    spawn.position[2] = z;
    spawn.group = group;
    return spawn;
}

void spriteFrames() {
    // 0x17fc90: bits 6..9 of ticks * 0x51eb851f >> 32; the clock runs at 3000 ticks per second (oracle, object 0x5165c8).
    require(studSpriteFrame(0) == 0, "frame 0 at tick 0");
    require(studSpriteFrame(199) == 0 && studSpriteFrame(200) == 1, "frame 1 starts at tick 200 (1/15 s)");
    require(studSpriteFrame(3000) == 15, "tick 3000 (one second) shows frame 15");
    require(studSpriteFrame(3200) == 0, "16 frames repeat every 3200 ticks");
}

void pickupPoint() {
    // player entity +0x60 = (0, 1.153, 0.08), read in the oracle at the gameplay start
    const std::array<float, 3> ahead = studPickupPoint(10, 20, 30, 0);
    require(std::fabs(ahead[0] - 10) < 1e-6f && std::fabs(ahead[1] - 21.153f) < 1e-5f && std::fabs(ahead[2] - 30.08f) < 1e-5f,
            "pickup point 1.153 above the feet and 0.08 ahead");
    const std::array<float, 3> east = studPickupPoint(10, 20, 30, float(M_PI) / 2);
    require(std::fabs(east[0] - 10.08f) < 1e-5f && std::fabs(east[2] - 30) < 1e-5f, "heading turns the forward offset");
}

void hiddenAndCentre() {
    StudField field({spawnAt(StudKind::Gold, 1, 2, 3), spawnAt(StudKind::Silver, 4, 5, 6, 3)});
    require(field.studs()[0].state == StudState::Idle, "no group: visible at the start");
    require(field.studs()[1].state == StudState::Hidden, "DisabledOnStartUp 3: hidden until a script enables its group");
    require(std::fabs(field.studs()[0].centre[1] - 2.56f) < 1e-6f, "the sprite centre is 0.56 above initialposition");
    const std::array<float, 3> on = {4, 5.56f, 6};
    field.update(0.033f, on);
    require(field.studs()[1].state == StudState::Hidden, "a hidden stud is not collected");
}

void radius() {
    // recorded in the oracle (rad_t2, rad_t3): collected at distance 1.49, not at 1.505
    StudField near({spawnAt(StudKind::Silver, 0, 0, 0)}), far({spawnAt(StudKind::Silver, 0, 0, 0)});
    const float centreY = kStudCentreHeight;
    require(near.update(0.033f, {1.49f, centreY, 0}).size() == 1, "collected at 1.49");
    require(far.update(0.033f, {1.505f, centreY, 0}).empty(), "not collected at 1.505");
    StudField vertical({spawnAt(StudKind::Silver, 0, 0, 0)});
    require(vertical.update(0.033f, {0, centreY + 1.0f, 1.0f}).size() == 1, "the distance is three-dimensional (1.414 is inside)");
    StudField diagonal({spawnAt(StudKind::Silver, 0, 0, 0)});
    require(diagonal.update(0.033f, {0, centreY + 1.1f, 1.1f}).empty(), "the distance is three-dimensional (1.556 is outside)");
}

void collectAnimationAndValues() {
    const StudKind kinds[4] = {StudKind::Silver, StudKind::Gold, StudKind::Blue, StudKind::Purple};
    const std::uint64_t values[4] = {10, 100, 1000, 10000};   // 0x28a128
    const std::uint16_t sounds[4] = {64, 64, 63, 63};         // tablessoundfx coin1 / coinblue, u16 at +0x8a of the type table
    for (int k = 0; k < 4; ++k) {
        StudField field({spawnAt(kinds[k], 0, 0, 0)});
        const std::array<float, 3> on = {0, kStudCentreHeight, 0};
        const std::vector<StudCollected> first = field.update(0.07f, on);
        require(first.size() == 1 && first[0].kind == kinds[k] && first[0].sound == sounds[k], "collect event with its sound");
        require(field.studs()[0].state == StudState::Collecting && field.counter() == 0, "the counter does not change on contact");
        // oracle (rad_long): collected in update 42, state 4 for 14 further updates at dt 0.07, counter added in update 57
        for (int update = 0; update < 14; ++update) {
            field.update(0.07f, on);
            require(field.studs()[0].state == StudState::Collecting && field.counter() == 0, "collecting for one second");
        }
        field.update(0.07f, on);
        require(field.studs()[0].state == StudState::Done && field.counter() == values[k], "value added when the animation ends");
    }
}

void counterLimit() {
    StudField field({spawnAt(StudKind::Purple, 0, 0, 0)});
    const std::array<float, 3> on = {0, kStudCentreHeight, 0};
    for (int update = 0; update < 40; ++update) field.update(0.07f, on);
    require(field.counter() == 10000, "one purple stud");
    require(kStudCounterLimit == 4200000000ull, "0x28a128 clamps at 4200000000");
}

void skipCounter() {
    // 0x180168: a stud far away is not tested again for (distance - 1.5) / 0.23 updates (at most 17)
    StudField field({spawnAt(StudKind::Gold, 0, 0, 0)});
    const std::array<float, 3> faraway = {3.5f, kStudCentreHeight, 0}, on = {0, kStudCentreHeight, 0};
    field.update(0.033f, faraway);
    require(field.studs()[0].skip == int((2.0f) / kStudSkipUnit), "skip counter from the distance");
    int updates = 0;
    while (field.update(0.033f, on).empty()) {
        ++updates;
        require(updates < 20, "tested again after the skip counter ran out");
    }
    require(updates == int(2.0f / kStudSkipUnit) - 1, "the player on the stud is noticed when the skip counter reaches zero");
    StudField distant({spawnAt(StudKind::Gold, 0, 0, 0)});
    distant.update(0.033f, {100, kStudCentreHeight, 0});
    require(distant.studs()[0].skip == kStudSkipLimit, "skip counter is limited");
}

void bankedTotals() {
    // 0x28a128 adds the value, then the hook 0x29c090 hands the new counter to the HUD stud bar (0x22ddf4).
    StudField field({spawnAt(StudKind::Silver, 0, 0, 0), spawnAt(StudKind::Blue, 0, 0, 0)});
    const std::array<float, 3> on = {0, kStudCentreHeight, 0};
    field.update(0.07f, on);
    require(field.takeBanked().empty(), "nothing is banked on contact");
    for (int update = 0; update < 15; ++update) field.update(0.07f, on);
    const std::vector<StudBanked> banked = field.takeBanked();
    require(banked.size() == 2 && banked[0].kind == StudKind::Silver && banked[0].total == 10 && banked[1].kind == StudKind::Blue &&
                banked[1].total == 1010,
            "the studs bank one after the other with the running total");
    require(field.takeBanked().empty(), "banked events are handed out once");
}

void flight() {
    // Oracle trace hud-traces/bar1 (tools/oracle/hud-bar.txt): the stud record at 861.136 17.352 1060.531 flew in 15 updates
    // of dt 0.07; from update 19 on the camera had settled and the implied target was constant. Records 19..24 (position, timer
    // after the update): the step is (target - position) / (30 * timer), measured as 0x180168 state 4.
    const std::array<float, 3> target = {864.6103f, 20.8361f, 1064.3806f};
    const float timers[6] = {0.37f, 0.3f, 0.23f, 0.16f, 0.09f, 0.02f};
    const float expected[6][3] = {{862.515f, 18.754f, 1062.039f}, {862.748f, 18.985f, 1062.299f}, {863.018f, 19.253f, 1062.600f},
                                  {863.349f, 19.583f, 1062.971f}, {863.816f, 20.047f, 1063.493f}, {865.140f, 21.362f, 1064.972f}};
    StudField field({spawnAt(StudKind::Silver, 0, 0, 0)});
    field.setFlightTarget(target);
    Stud& stud = field.studs()[0];
    stud.state = StudState::Collecting;
    stud.centre = {expected[0][0], expected[0][1], expected[0][2]};
    stud.timer = timers[0];
    for (int row = 0; row < 6; ++row) {
        if (row > 0) field.update(0.07f, {1000, 1000, 1000});
        for (int axis = 0; axis < 3; ++axis)
            require(std::fabs(stud.centre[std::size_t(axis)] - expected[row][axis]) < 2e-3f, "recorded flight position");
        require(std::fabs(stud.timer - timers[row]) < 1e-4f, "recorded timer");
    }
    require(field.update(0.07f, {1000, 1000, 1000}).empty() && field.studs()[0].state == StudState::Done, "ends when the timer runs out");
    StudField still({spawnAt(StudKind::Silver, 5, 6, 7)});
    still.studs()[0].state = StudState::Collecting;
    still.studs()[0].timer = 1.0f;
    still.update(0.07f, {1000, 1000, 1000});
    require(still.studs()[0].centre[0] == 5 && std::fabs(still.studs()[0].centre[1] - 6.56f) < 1e-5f, "without a target the stud stays where it is");
}

void unprojection() {
    // Oracle (tools/oracle/hud-camera.txt): camera eye K+0x34 (867.3656, 20.09896, 1069.3754) and look point K+0x94 + K+0xf4
    // (862.5931, 18.39880, 1060.5876); the sink (203, -14.5, depth 5.5) from 0x23ca00 was the point (864.61, 20.836, 1064.381)
    // that the flying stud converged to in the recorded trace.
    const float eye[3] = {867.3656f, 20.09896f, 1069.37537f};
    const float look[3] = {862.59314f, 18.39880f, 1060.58765f};
    const float dx = look[0] - eye[0], dy = look[1] - eye[1], dz = look[2] - eye[2];
    const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float yaw = std::atan2(dx, dz), pitch = std::asin(dy / length);
    const float verticalFov = 2.0f * std::atan(0.75f * std::tan(40.0f * 3.1415927f / 180 * 0.5f));
    const std::array<float, 3> world = unprojectTopScreen(203.0f, -14.5f, 5.5f, eye, yaw, pitch, verticalFov);
    require(std::fabs(world[0] - 864.61f) < 2e-3f && std::fabs(world[1] - 20.836f) < 2e-3f && std::fabs(world[2] - 1064.381f) < 2e-3f,
            "the counter sink unprojects to the recorded flight target");
    const std::array<float, 3> centre = unprojectTopScreen(200.0f, 120.0f, 5.5f, eye, yaw, pitch, verticalFov);
    require(std::fabs(centre[0] - (eye[0] + 5.5f * dx / length)) < 1e-3f && std::fabs(centre[1] - (eye[1] + 5.5f * dy / length)) < 1e-3f,
            "the screen centre is straight ahead");
}
}

int main() {
    try {
        spriteFrames();
        pickupPoint();
        hiddenAndCentre();
        radius();
        collectAnimationAndValues();
        counterLimit();
        skipCounter();
        bankedTotals();
        flight();
        unprojection();
    } catch (const std::exception& error) {
        std::cerr << "studs-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "studs-test: ok\n";
    return 0;
}
