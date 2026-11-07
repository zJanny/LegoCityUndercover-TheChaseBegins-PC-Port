// The readers of BulletTime's state, read from artifacts/game/game.arm.elf (llvm-objdump, armv6k + vfp3) by 20261107-traffic-G (2026-11-07):
// the restore's mode byte dispatch 0x239410..0x239454, the slowing call's mode 0x2393c4..0x2393e8, the named-object lookup 0x23948c..0x2394cc and the
// emitter callback 0x2b6698. The record bytes and the products are made up and hand-computed from those rules.
#include "runtime/bullet_time.hpp"
#include "runtime/bullet_time_scale.hpp"

#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void modeByteSelectsTheCall() {
    require(bulletTimeModeCall(255) == BulletTimeModeCall::None, "-1 (cmn r0, #1 at 0x239414) calls nothing");
    require(bulletTimeModeCall(0) == BulletTimeModeCall::GameplayRecords, "0 calls 0x221054 (0x23941c)");
    require(bulletTimeModeCall(4) == BulletTimeModeCall::GameplayRecords, "4 calls 0x221054 (0x23942c..0x239434)");
    require(bulletTimeModeCall(3) == BulletTimeModeCall::RotFixedPreset, "3 calls 0x1fc060 (0x239424)");
    for (unsigned mode : {1u, 2u, 5u, 6u, 127u, 128u, 254u}) require(bulletTimeModeCall(std::uint8_t(mode)) == BulletTimeModeCall::None, "any other value calls nothing");
}

void slowingModeComesFromTheShotRecord() {
    require(bulletTimeSlowingMode(false, true, std::uint8_t(3)) == kBulletTimeNoMode, "flag clear: 255 whatever the record says (0x2393c4 cmp r7)");
    require(bulletTimeSlowingMode(true, false, std::uint8_t(3)) == kBulletTimeNoMode, "no owner: 255 (cmpne r5, #0)");
    require(bulletTimeSlowingMode(true, true, std::nullopt) == kBulletTimeNoMode, "no record from 0x20395c: 255 stays");
    require(bulletTimeSlowingMode(true, true, std::uint8_t(4)) == 4, "flag, owner and a record: the byte +0x18 of the record (0x2393e0)");
}

void systemKeepsTheModeForTheRestore() {
    BulletTimeSystem system;
    system.set(0.5f, 1.0f, true, std::uint8_t(3));
    require(system.mode() == 3 && system.cameraShotRequested(), "the slowing call stores the record's mode in +0x68");
    require(system.takeModeCall() == BulletTimeModeCall::None, "slowing makes no mode call");
    system.set(1.0f, 1.0f);
    require(system.takeModeCall() == BulletTimeModeCall::RotFixedPreset, "the restore calls 0x1fc060 for mode 3");
    require(system.takeModeCall() == BulletTimeModeCall::None, "taken once");
    require(system.takeRestoreObjectRequest() && !system.takeRestoreObjectRequest(), "the restore looks the named object up once");

    BulletTimeSystem noFlag;
    noFlag.set(0.5f, 1.0f, false);
    noFlag.set(1.0f, 1.0f);
    require(noFlag.takeModeCall() == BulletTimeModeCall::None, "flag clear: mode 255, no mode call (SCRIPT-S323 is narrowed to the flag branch)");

    BulletTimeSystem gameplay;
    gameplay.set(0.5f, 1.0f, true, std::uint8_t(0));
    gameplay.set(1.0f, 1.0f);
    require(gameplay.takeModeCall() == BulletTimeModeCall::GameplayRecords, "mode 0 calls 0x221054");
}

void namedObjectLookup() {
    require(std::strcmp(kBulletTimeRestoreObjectName, "5-11_AI_Boss_Dcam_Running") == 0, "string at 0x2394d4");
    require(!bulletTimeRestoreRunsNamedObject(false, 1), "no object by that name (0x2394a8): nothing");
    require(!bulletTimeRestoreRunsNamedObject(true, 0xfffe), "bit 0 of the halfword +0x10 clear (0x2394b0): nothing");
    require(bulletTimeRestoreRunsNamedObject(true, 1), "found with bit 0: 0x271ccc and 0x1dd3f8");
}

void emitterCallbackScalesTheCount() {
    ScaledCountRecord record{0, 100};
    applyTimeScaleToCount(record, 1.0f);
    require(record.count == 100 && record.flags == 0, "a scale of exactly 1.0 leaves the record alone (0x2b66a4..0x2b66a8)");
    applyTimeScaleToCount(record, 0.25f);
    require(record.count == 25 && record.flags == kScaledCountChangedBit, "100 x 0.25 = 25, bit 2 of the halfword +2 set");
    record = {1, 7};
    applyTimeScaleToCount(record, 0.5f);
    require(record.count == 3 && record.flags == 5, "7 x 0.5 = 3.5 truncates to 3, the other flag bits stay");
    record = {0, -7};
    applyTimeScaleToCount(record, 0.5f);
    require(record.count == -3, "vcvt.s32.f32 truncates towards zero");
    record = {0, 100};
    applyTimeScaleToCount(record, 0.01f);
    require(record.count == 1, "100 x 0.01f rounds to 1.0f in single precision, which stays 1");
    record = {0, 50};
    applyTimeScaleToCount(record, 0.01f);
    require(record.count == 0, "50 x 0.01 = 0.5 truncates to 0");
    record = {0, 1000000};
    applyTimeScaleToCount(record, 4e6f);
    require(record.count == std::numeric_limits<std::int32_t>::max(), "vcvt saturates");
    record = {0, 5};
    applyTimeScaleToCount(record, std::numeric_limits<float>::quiet_NaN());
    require(record.count == 0, "vcvt of NaN is 0");
}
}

int main() {
    try {
        modeByteSelectsTheCall();
        slowingModeComesFromTheShotRecord();
        systemKeepsTheModeForTheRestore();
        namedObjectLookup();
        emitterCallbackScalesTheCount();
    } catch (const std::exception& error) {
        std::cerr << "bullet-time-scale-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "bullet-time-scale-test: ok\n";
    return 0;
}
