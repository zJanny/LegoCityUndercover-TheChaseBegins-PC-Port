// Tests of the pickup of super debris bricks (src/runtime/debris_pickup.*) and the HUD pending add (src/runtime/hud_pending_add.*),
// docs/re/super-bricks.md entry 2026-10-20 "The pickup of super debris bricks". The values come from llvm-objdump of artifacts/game/game.arm.elf:
// the record update 0x180168 (pickup test 0x180318..0x1804f4, magnet state 3 0x1808cc..0x180a88, state 4 0x180a8c..0x180be8), the collect
// 0x2277c4, the hook 0x2c4f78, the counter add 0x28a128 and its hook 0x29c090, the placement 0x2a2cc0, the HUD update 0x2a6068..0x2a60c4; and
// the type row 6 read in the oracle (debris.md §2026-10-18T14:46:04: +0x68 0.1, +0x8a 1).
#include "runtime/debris_pickup.hpp"
#include "runtime/hud_pending_add.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

constexpr float kDt = 0.033f;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

DebrisRecord superRecord(float x, std::uint16_t value) {
    DebrisRecord record;
    record.position = {x, 0.0f, 0.0f};
    record.colour = 0xff102030u;
    record.object = 3;
    record.kind = kSuperDebrisKind;
    record.superValue = value;
    record.type = kDebrisPickupType;
    record.flags22 = kDebrisFlagSuper | kDebrisFlagPickable;
    record.life = 8.0f;
    record.state = DebrisState::Resting;
    return record;
}

DebrisPickupInput onFoot() {
    DebrisPickupInput input;
    input.frameDelta = kDt;
    input.pickupPoint = {0.0f, 0.0f, 0.0f};
    input.tint = 0xff0b30ffu;
    return input;
}

void testNeedsThePickableBit() {
    DebrisRecord record = superRecord(1.0f, 1);
    record.flags22 = kDebrisFlagSuper;
    std::uint32_t count = 96;
    const DebrisPickupStep step = updateDebrisPickup(&record, 1, onFoot(), count);
    require(record.state == DebrisState::Resting && record.skip == 0 && step.sounds.empty(), "a record without +0x22 bit 2 is not tested");
    record.flags22 = kDebrisFlagSuper | kDebrisFlagPickable;
    DebrisPickupInput blocked = onFoot();
    blocked.blocked = true;
    updateDebrisPickup(&record, 1, blocked, count);
    require(record.state == DebrisState::Resting && record.skip == 0, "the player's word +0xc bit 4 skips the test");
}

void testPickupWithinTheRadius() {
    DebrisRecord record = superRecord(1.49f, 1);
    std::uint32_t count = 96;
    const DebrisPickupStep step = updateDebrisPickup(&record, 1, onFoot(), count);
    require(record.state == DebrisState::Collected, "a pickable record within 1.5 is collected");
    require(near(record.life, kDebrisCollectTime - kDt), "the collect timer loses the frame time in the update of the collect (0x18083c)");
    require(step.sounds.size() == 1 && step.sounds[0].id == 1 && near(step.sounds[0].volume, 0.7f), "the collect sound is row 6 +0x8a at volume 0.7");
    require(step.shown == 1 && record.colour == 0xff0b30ffu, "0x2c4f78 takes the tint and shows the HUD display");
    require(step.placements.size() == 1 && step.placements[0].record == 0 && near(step.placements[0].scale, 0.5f + 0.5f * record.life),
            "a collected record places its object with the scale 0.5 + 0.5 timer");
    require(count == 96, "the count changes only when the timer ran out");
}

void testSkipCounter() {
    DebrisRecord record = superRecord(3.0f, 1);
    std::uint32_t count = 0;
    updateDebrisPickup(&record, 1, onFoot(), count);
    require(record.skip == 6 && record.state == DebrisState::Resting, "outside the radius the skip is int((3.0 - 1.5) / 0.23) = 6");
    record.position = {0.5f, 0.0f, 0.0f};
    for (int i = 0; i < 5; ++i) updateDebrisPickup(&record, 1, onFoot(), count);
    require(record.skip == 1 && record.state == DebrisState::Resting, "the test waits for the skip counter");
    updateDebrisPickup(&record, 1, onFoot(), count);
    require(record.state == DebrisState::Collected, "the counter reaching 0 tests again");
    DebrisRecord far = superRecord(100.0f, 1);
    updateDebrisPickup(&far, 1, onFoot(), count);
    require(far.skip == 30, "the skip is limited to 0x23ca4c() = 30");
}

void testMagnetFliesToTheTarget() {
    DebrisRecord record = superRecord(4.0f, 1);
    DebrisPickupInput input = onFoot();
    input.magnet = true;
    input.magnetTarget = DebrisVec3{0.0f, 1.0f, 0.0f};
    std::uint32_t count = 0;
    updateDebrisPickup(&record, 1, input, count);
    require(record.state == DebrisState::Magnet && near(record.life, kDebrisMagnetLife - kDt), "the magnet (radius FLT_MAX) starts state 3");
    updateDebrisPickup(&record, 1, input, count);
    require(near(record.position[0], 3.6f) && near(record.position[1], 0.1f), "state 3 moves 0.1 of the way to the target per update");
    unsigned updates = 0;
    DebrisPickupStep step;
    while (record.state == DebrisState::Magnet && updates < 100) {
        step = updateDebrisPickup(&record, 1, input, count);
        ++updates;
    }
    require(record.state == DebrisState::Collected && step.shown == 1, "the magnet collects the record");
    require(updates == 23, "collected in the update after the 23rd step: 4.123 x 0.9^23 < 1.5 x 0.25 < 4.123 x 0.9^22");
}

void testMagnetLifeRunsOut() {
    DebrisRecord record = superRecord(10.0f, 1);
    DebrisPickupInput input = onFoot();
    input.magnet = true;
    std::uint32_t count = 0;
    updateDebrisPickup(&record, 1, input, count);
    unsigned updates = 0;
    while (record.state == DebrisState::Magnet && updates < 100) {
        updateDebrisPickup(&record, 1, input, count);
        ++updates;
    }
    require(record.state == DebrisState::Collected && near(record.position[0], 10.0f), "without a target node state 3 waits for its life");
    require(updates == 30, "life 1.0 minus 0.033 in the first update and 30 more updates");
}

void testBankAddsTheValue() {
    DebrisRecord records[2] = {superRecord(1.0f, 1), superRecord(1.0f, 10)};
    DebrisPickupInput input = onFoot();
    input.flightTarget = DebrisVec3{0.0f, 5.0f, 0.0f};
    std::uint32_t count = 96;
    updateDebrisPickup(records, 2, input, count);
    const float yBefore = records[0].position[1];
    updateDebrisPickup(records, 2, input, count);
    require(near(records[0].position[1], yBefore + (5.0f - yBefore) / (30.0f * records[0].life)), "state 4 flies to sink 2 by 1 / (30 timer)");
    DebrisPickupStep step;
    unsigned updates = 1;
    while (records[0].state == DebrisState::Collected) {
        step = updateDebrisPickup(records, 2, input, count);
        ++updates;
    }
    require(updates == 30, "the collect timer 1.0 runs out in the 30th update after the collect (dt 0.033)");
    require(count == 107 && step.banked.size() == 2 && step.banked[0].total == 97 && step.banked[1].total == 107, "0x29c090 adds +0x20 to 0x593430");
    require(step.banked[0].sound == 2 && step.banked[1].sound == 14, "UI sound 14 for a value of 10 or more, else 2");
    require(step.studUnits == 20, "0x28a128 adds 10 to the stud counter for each type 6 record");
    require(step.freedRecords.size() == 2 && records[0].state == DebrisState::Free, "the record is freed with its object");
}

void testOracleMagnetCycle() {
    DebrisRecord record = superRecord(14.2f, 1);
    DebrisPickupInput input = onFoot();
    input.magnet = true;
    input.magnetTarget = DebrisVec3{0.0f, 0.0f, 0.0f};
    input.flightTarget = DebrisVec3{0.0f, 10.0f, 0.0f};
    std::uint32_t count = 0x66;
    unsigned magnetUpdates = 0, collectedUpdates = 0;
    std::uint64_t studs = 0;
    updateDebrisPickup(&record, 1, input, count);
    require(record.state == DebrisState::Magnet, "state 3 one update after the landing (B-1)");
    for (unsigned update = 0; update < 100 && record.state != DebrisState::Free; ++update) {
        const DebrisPickupStep step = updateDebrisPickup(&record, 1, input, count);
        studs += step.studUnits;
        if (record.state == DebrisState::Magnet) ++magnetUpdates;
        else if (record.state == DebrisState::Collected) ++collectedUpdates;
        input.magnetTarget = DebrisVec3{record.position[0] + 2.0f, 0.0f, 0.0f};
    }
    require(magnetUpdates + 1 == 30 && collectedUpdates == 31, "30 updates in state 3 and 31 in state 4 (B-1, dt 0.033)");
    require(count == 0x67 && studs == 10, "+1 to 0x593430 and +10 studs in the update of the free (B-1)");
}

void testPendingAdd() {
    HudPendingAdd hud;
    require(!applyHudPendingAdd(hud, 2000, 1000), "nothing pending");
    hud.pending = 5;
    require(!applyHudPendingAdd(hud, 1035, 1000), "not until the update count is above stamp + 35");
    const std::optional<std::uint32_t> applied = applyHudPendingAdd(hud, 1036, 1000);
    require(applied && *applied == 5 && hud.pending == 0, "applied and cleared at stamp + 36");
}

}

int main() {
    try {
        testNeedsThePickableBit();
        testPickupWithinTheRadius();
        testSkipCounter();
        testMagnetFliesToTheTarget();
        testMagnetLifeRunsOut();
        testBankAddsTheValue();
        testOracleMagnetCycle();
        testPendingAdd();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "debris pickup: all tests passed\n";
    return 0;
}
