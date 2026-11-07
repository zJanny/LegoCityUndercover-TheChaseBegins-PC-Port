#pragma once
// The readers of BulletTime's state (docs/re/scripts-missions-1-4-roof.md): the mode byte +0x68 of the system 0x239330 and the camera mode call
// the restore makes from it (0x221054, 0x1fc060), the restore's lookup of the object `5-11_AI_Boss_Dcam_Running`, and the global time scale
// 0x376d08 (0x376ce4 + 0x24, stored by 0x2a0744) that the emitter callback 0x2b6698 reads. No SDL dependency.
#include <cstdint>
#include <optional>

namespace chase::runtime {

constexpr std::uint8_t kBulletTimeNoMode = 255;
constexpr const char* kBulletTimeRestoreObjectName = "5-11_AI_Boss_Dcam_Running";

// 0x221054 requests the camera mode object `*(0x3770c8 + 0x10)`, 0x1fc060 the preset object `*0x372d74`; both store it through 0x226a30 into the
// camera manager's requested mode (0x376870 + 0x28).
enum class BulletTimeModeCall : std::uint8_t { None, GameplayRecords, RotFixedPreset };

// 0x239410..0x239454: the signed byte -1 (255) calls nothing, 0 and 4 call 0x221054, 3 calls 0x1fc060, every other value nothing.
BulletTimeModeCall bulletTimeModeCall(std::uint8_t mode);

// 0x2393c4..0x2393e8 for a scale below 1.0: the mode is 255; with the flag set and an owner, the byte +0x18 of the record that 0x20395c returns
// replaces it when there is a record.
std::uint8_t bulletTimeSlowingMode(bool flag, bool hasOwner, std::optional<std::uint8_t> shotRecordMode);

// 0x23948c..0x2394b4 after the restore's sound 99: the object found by name runs the two calls 0x271ccc and 0x1dd3f8 when bit 0 of its halfword
// +0x10 is set.
bool bulletTimeRestoreRunsNamedObject(bool found, std::uint16_t flags);

// The record the emitter callback 0x2b6698 gets: the halfword +2 collects change bits and the integer +0x18 is scaled.
struct ScaledCountRecord {
    std::uint16_t flags = 0;
    std::int32_t count = 0;
};
constexpr std::uint16_t kScaledCountChangedBit = 4;

// 0x2b6698: a time scale of exactly 1.0 leaves the record alone; otherwise count = (int)((float)count x scale) (vcvt.s32.f32: truncation towards zero,
// saturating, NaN 0) and bit 2 of the flags is set. Always returns true.
bool applyTimeScaleToCount(ScaledCountRecord& record, float timeScale);

}
