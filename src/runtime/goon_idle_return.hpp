#pragma once
// The parts of the idle decision 0x2a9874 around the sleep and the return to the spawn point (docs/re/scripts-missions-1-4-spawners-conduct.md):
// the gate 0x241098, the sleep branch (`AISleepAnim`, `AISleepAnimWake`, ctrl+0xdc bits 3..4), the request of AI state 15 at the end of the
// decision and state 15 itself (enter 0x2bba40, decision 0x2c5cac). Pure steps on plain values; goon_conduct.cpp applies them. No SDL dependency.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr std::uint8_t kAiReturnToSpawnState = 15;     // 0x2a9d4c: the request of 0x2a9874
constexpr std::uint8_t kAiReturnedState = 14;          // 0x2c5cf8: the request of 0x2c5cac when the wait ends
constexpr std::int32_t kAiReturnWaitDefault = 15;      // 0x2bba5c, and the default of `Wait_Time` at 0x2bba68
constexpr float kAiReturnDistance = 2.0f;              // literal 0x2a9d88, compared with the 0x22b62c distance
constexpr std::uint8_t kAiSleepNotYet = 0;             // ctrl+0xdc bits 3..4
constexpr std::uint8_t kAiSleepWaking = 1;
constexpr std::uint8_t kAiSleepAwake = 2;
constexpr std::uint8_t kAiSleepAsleep = 3;
constexpr std::uint16_t kAiSleepLocomotionLimit = 1;   // 0x2a992c: a locomotion state above 1 wakes the character
constexpr float kAiSleepClipBlend = 0.1f;              // literal 0x2a9ba8, s0 of 0x2102b0
constexpr float kAiSleepClipSpeed = 1.0f;              // literal 0x2a9ba4, s1 of 0x2102b0

// STAND-IN SCRIPT-S329: the port plays no clip on AI characters, so a sleep or wake clip counts as ended in the next update.
constexpr bool kAiClipEndedStandIn = true;

// The parameters 0x2a9874 and 0x2bba40 read by name.
struct AiIdleReturnParams {
    std::string sleepAnim;      // `AISleepAnim` (0x241624 at 0x2a98b4)
    std::string sleepAnimWake;  // `AISleepAnimWake` (0x241624 at 0x2a99b0)
    std::int32_t waitTime = kAiReturnWaitDefault;  // `Wait_Time` (0x2451d0 at 0x2bba74), read only when ctrl+0xd4 is set
};

// 0x241098 answers the clock of the running process; the decision ends when it is 0.
bool aiIdleGate(std::uint32_t clock);

// The clip name 0x2a99d4 takes from a sleep parameter: the text after its last '/'.
std::string aiSleepClipName(const std::string& path);

enum class AiSleepClip : std::uint8_t { None, Sleep, Wake };
struct AiSleepInput {
    std::uint8_t bits = kAiSleepNotYet;  // ctrl+0xdc bits 3..4
    std::uint16_t locomotionState = 1;   // ctrl+0x6c
    bool friendly = false;               // ctrl+0xdb bit 2
    bool hasWakeClip = false;            // `AISleepAnimWake` set and not empty
    bool clipEnded = false;              // 0x22f560 of the current clip (0x233900) answers 6
};
struct AiSleepStep {
    bool awake = true;                   // r5: the target search and the 0x1fbad0 tail run
    std::uint8_t bits = kAiSleepNotYet;
    AiSleepClip clip = AiSleepClip::None;  // 0x2102b0: the sleep clip looped (r2 = 1) or the wake clip once (r2 = 0)
};
// 0x2a98b4..0x2a9acc for a character whose `AISleepAnim` is set and not empty. `seesPlayer` is the sight test 0x1f6958(entity, *0x3772b8, 0, 0,
// -1.0, -1.0); it is asked only in the first state of a character that is not friendly and stands (locomotion state 0 or 1).
AiSleepStep aiSleepStep(const AiSleepInput& input, const std::function<bool()>& seesPlayer);

// 0x2a9cf4..0x2a9d38: an engaged, shown character asks for AI state 15 with ctrl+0xd4 set, or with `ReturnToSpawnPoint` when it stands at
// least 2.0 from its spawn point ctrl+0xac (0x22b62c, the distance in x and z).
bool aiReturnRequested(bool waitObject, bool returnToSpawnPoint, const std::array<float, 3>& position, const std::optional<std::array<float, 3>>& spawn);
// The enter 0x2bba40 after its release 0x21decc: ctrl+0x84 = `Wait_Time` with ctrl+0xd4 set, else 15.
std::uint32_t aiReturnCountdown(bool waitObject, const AiIdleReturnParams& params);
// The first part of 0x2c5cac: with ctrl+0xd4 or `ReturnToSpawnPoint` ctrl+0x84 counts down (signed); true when it is then at or below 0 and
// AI state 14 is requested.
bool aiReturnWaitStep(bool waitObject, bool returnToSpawnPoint, std::uint32_t& countdown);

}
