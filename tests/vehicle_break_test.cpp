// Test of the break of a vehicle and its wreck (src/runtime/vehicle_break.*, docs/re/vehicles-damage.md). Values from llvm-objdump / REA of
// artifacts/game/game.arm.elf (0x33b264..0x33b378, 0x2301b0..0x230230, 0x16cccc, 0x1765cc, 0x1d27f4, 0x22b224, 0x1aff24, 0x24257c, 0x307cac)
// and from the oracle trace of agent r18-B (state drv-police-car-seated, fixed dt 0.033, hearts poked to 1, A held, stick (100, 0) for 24
// updates, memtrace of record 0x08a884e0 +0x2..+0x4, +0x298, +0x30c..+0x313, +0x38..+0x77, +0x278, +0x320..+0x34f per update, rows 25..284).
#include "runtime/vehicle_break.hpp"
#include "runtime/vehicle_drive.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

const chase::assets::Matrix kIdentity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

void stuckTimerCountsToTheDestroy() {
    DrivenStuckTimer timer;
    const float step = kVehicleWorldStep;
    for (int i = 0; i < 90; ++i) stepDrivenStuckTimer(timer, -2.97f, 1.0f, 0.0f, 0.0f, step);
    expect(!timer.destroyRequest && near(timer.seconds, 3.0f, 1e-4f),
           "90 steps of 1/30 against the box reach 3.0000 without the request (r18-A trace: speed -2.97, timer 3.0000 at update 205)");
    stepDrivenStuckTimer(timer, -2.97f, 1.0f, 0.0f, 0.0f, step);
    expect(timer.destroyRequest && timer.seconds == 0.0f, "the 91st step passes 3.0, sets body+0x90 bit 0x40 and zeroes the timer (update 206)");
    const std::optional<DamageMessage> message = stuckDestroyMessage(timer, false);
    expect(message && message->hearts == 1000 && message->type == kVehicleDamageTypeScripted && message->kind == 0 && !timer.destroyRequest,
           "0x23019c sends message 0 with word +0xc = 1000 and type 6 and clears the bit (0x230200..0x230230)");
}

void stuckTimerRules() {
    DrivenStuckTimer timer;
    timer.seconds = 1.0f;
    stepDrivenStuckTimer(timer, 1.0f, 1.0f, 1.0f, 0.0f, 0.1f);
    expect(timer.seconds == 0.0f, "slow with accelerate and brake both pressed zeroes the timer (0x33b2c8..0x33b2e0)");
    timer.seconds = 1.0f;
    stepDrivenStuckTimer(timer, 1.0f, 1.0f, 0.0f, 1.5f, 0.1f);
    expect(near(timer.seconds, 0.975f, 1e-6f), "slow but turning at 1.0 or more decays by 0.25 x dt (0x33b2c0, 0x33b340)");
    timer.seconds = 1.0f;
    stepDrivenStuckTimer(timer, 4.0f, 0.0f, 1.0f, 0.0f, 0.1f);
    expect(near(timer.seconds, 0.975f, 1e-6f), "between 3 and 5 with one input decays");
    stepDrivenStuckTimer(timer, 4.0f, 0.0f, 0.0f, 0.0f, 0.1f);
    expect(timer.seconds == 0.0f, "no input zeroes the timer");
    timer.seconds = 1.0f;
    stepDrivenStuckTimer(timer, -5.5f, 1.0f, 0.0f, 0.0f, 0.1f);
    expect(timer.seconds == 0.0f, "above 5.0 the timer is zero (0x33b318)");
    timer.seconds = 2.0f;
    timer.destroyRequest = true;
    expect(!stuckDestroyMessage(timer, true) && timer.seconds == 0.0f && timer.destroyRequest,
           "a forced stop writes 0.0 to body+0x88 and sends nothing (0x2301d8)");
}

void launchVector() {
    GameRandom random(1234);
    GameRandom copy = random;
    const Vector3 launch = wheelLaunchVector({0.0f, 0.0f, 0.0f}, {0.0f, -3.0f, 4.0f}, random);
    const float rx = gameRandomUnit(copy), ry = gameRandomUnit(copy), rz = gameRandomUnit(copy);
    expect(near(launch[0], (rx - 0.5f) * 0.1f, 1e-6f), "x: direction 0 halved plus (r - 0.5) x 0.1 (first draw, 0x16cd74)");
    expect(near(launch[1], -0.6f + 1.0f + (ry - 0.5f) * 0.1f, 1e-6f), "y: direction -0.6 plus 1.0 plus the second draw (0x16cd88)");
    expect(near(launch[2], 0.8f * 0.5f + (rz - 0.5f) * 0.1f, 1e-6f), "z: direction 0.8 halved plus the third draw (0x16cda4)");
}

void wreckWheelFollowsTheOracle() {
    // Oracle rows (wheel 0 launch vector record +0x320): break row 112 (-0.119, 0.592, -0.460); bounces in rows 130, 139, 145, 149, 152, 155 on
    // flat ground; from row 156 the vector (-0.002, -0.022, -0.007) is below the squared length 0.001 and stays.
    const int bounceRows[] = {130, 139, 145, 149, 152, 155};
    struct Row {
        int row;
        Vector3 launch;
    };
    const Row rows[] = {{113, {-0.119f, 0.517f, -0.460f}}, {129, {-0.119f, -0.683f, -0.460f}}, {130, {-0.059f, 0.266f, -0.230f}},
                        {138, {-0.059f, -0.334f, -0.230f}}, {139, {-0.029f, 0.167f, -0.115f}}, {145, {-0.014f, 0.104f, -0.058f}},
                        {155, {-0.002f, 0.053f, -0.007f}}, {156, {-0.002f, -0.022f, -0.007f}}, {173, {-0.002f, -0.022f, -0.007f}}};
    Vector3 velocity = {-0.119f, 0.592f, -0.460f};
    chase::assets::Matrix world = kIdentity;
    world[13] = 10.0f;
    GameRandom random(7);
    std::size_t next = 0;
    for (int row = 113; row <= 173; ++row) {
        const bool bounce = std::find(std::begin(bounceRows), std::end(bounceRows), row) != std::end(bounceRows);
        const WheelPieceSweep sweep = [&](const Vector3& from, const Vector3&) -> std::optional<WheelPieceHit> {
            if (!bounce) return std::nullopt;
            return WheelPieceHit{{from[0], from[1] - 0.2f, from[2]}, {0.0f, 1.0f, 0.0f}};
        };
        if (wheelPieceMoving(velocity)) stepWheelPiece(world, {0.0f, 0.0f, 0.0f}, 0.3f, velocity, sweep, random);
        if (next < std::size(rows) && rows[next].row == row) {
            const Vector3& want = rows[next].launch;
            const bool ok = near(velocity[0], want[0], 0.0015f) && near(velocity[1], want[1], 0.0015f) && near(velocity[2], want[2], 0.0015f);
            if (!ok) std::printf("row %d: (%.4f, %.4f, %.4f)\n", row, velocity[0], velocity[1], velocity[2]);
            expect(ok, "the wheel's launch vector follows the oracle rows (fall 0.075, bounce x 0.5, rest without fall below 2.0 / 0.125)");
            ++next;
        }
    }
    expect(next == std::size(rows), "every oracle row was checked");
}

void wheelPieceMovesAndTurnsAboutItsCentre() {
    chase::assets::Matrix world = kIdentity;
    world[12] = 1.0f;
    Vector3 velocity = {0.0f, 1.0f, 0.0f};
    GameRandom random(3);
    const Vector3 centre = {0.0f, 0.5f, 0.0f};
    expect(!stepWheelPiece(world, centre, 0.3f, velocity, {}, random), "a free flight does not rest");
    const float worldCentre[3] = {centre[0] * world[0] + centre[1] * world[4] + centre[2] * world[8] + world[12],
                                  centre[0] * world[1] + centre[1] * world[5] + centre[2] * world[9] + world[13],
                                  centre[0] * world[2] + centre[1] * world[6] + centre[2] * world[10] + world[14]};
    expect(near(worldCentre[0], 1.0f, 1e-5f) && near(worldCentre[1], 1.5f, 1e-5f) && near(worldCentre[2], 0.0f, 1e-5f),
           "the bounds centre moved by the velocity and stays fixed under the spin T(-c) R T(c) (0x227790, 0x2588f4, 0x258c9c, 0x258a10)");
    expect(near(velocity[1], 1.0f - 0.075f, 1e-6f), "the fall 0x37716c = 0.075 per update (static initialiser 0x350708)");
    expect(near(world[8], std::sin(0.2f), 1e-5f), "the spin {0.1, 0.2, 0.14} as the rotation of 0x237c48 (m[8] = sin y)");
}

void wreckTimerAndReturn() {
    GameRandom random(5);
    VehicleWreck wreck = breakVehicle(kIdentity, {kIdentity, kIdentity, kIdentity, kIdentity, kIdentity}, random);
    expect(wreck.active && wreck.wheels == 4, "the first four wheels are launched, a fifth is hidden (0x16cdc4)");
    int updates = 0;
    while (!wreck.returnDue && updates < 100) {
        stepWreckTimer(wreck, 0.033f);
        ++updates;
    }
    expect(updates == 61 && near(wreck.stateSeconds, 2.013f, 1e-3f), "state 7 is requested in the 61st update of state 6 (oracle rows 113..173, timer 2.013)");
    VehicleRecord record;
    record.health = 5;
    record.hearts = 0;
    record.healthPoints = 3;
    record.debrisCooldown = 0.4f;
    record.destroyed = true;
    record.state = record.requestedState = kVehicleStateDestroyed;
    restoreVehicleRecord(record);
    expect(record.hearts == 5 && record.healthPoints == 100 && record.debrisCooldown == 0.0f && !record.destroyed && record.state == kVehicleStateDriven &&
               record.requestedState == kVehicleStateDriven,
           "the return: hearts 5, health points 100, dead bit clear, state 5 at once (oracle row 174: state 5/5, +0x30c 100, +0x30d 5, entity +0xc 0)");
}

void history() {
    chase::assets::Matrix placement = kIdentity;
    VehicleMatrixHistory ring(placement);
    for (int i = 1; i <= 6; ++i) {
        chase::assets::Matrix node = kIdentity;
        node[12] = float(i);
        ring.record(node);
    }
    expect(ring.returnMatrix()[12] == 0.0f, "six records still return the start value");
    chase::assets::Matrix node = kIdentity;
    node[12] = 7.0f;
    ring.record(node);
    expect(ring.returnMatrix()[12] == 1.0f, "record +0x38 is the oldest of the seven ring entries (0x307e10..0x307e2c)");
}

void returnPlace() {
    expect(vehicleReturnPlaceGood(4, 4, 0.99f, 0.0f, false), "upright on four wheels is a good place");
    expect(!vehicleReturnPlaceGood(3, 4, 1.0f, 0.0f, false), "a wheel off the ground is not (body+0xda0 < +0x2b4)");
    expect(!vehicleReturnPlaceGood(4, 4, 0.98f, 0.0f, false), "a tilt above 10 degrees is not (cos 0x3e32b8c3 = 0.98481)");
    expect(!vehicleReturnPlaceGood(4, 4, 1.0f, -4.5f, false), "falling faster than 4.0 is not");
    expect(!vehicleReturnPlaceGood(4, 4, 1.0f, 0.0f, true), "a dead vehicle is not");
}

}

int main() {
    stuckTimerCountsToTheDestroy();
    stuckTimerRules();
    launchVector();
    wreckWheelFollowsTheOracle();
    wheelPieceMovesAndTurnsAboutItsCentre();
    wreckTimerAndReturn();
    history();
    returnPlace();
    if (failures) return 1;
    std::printf("vehicle break: all checks passed\n");
    return 0;
}
