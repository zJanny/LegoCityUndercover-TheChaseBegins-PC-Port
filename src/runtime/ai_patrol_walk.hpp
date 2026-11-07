#pragma once
// AI state 14, the walk to a patrol node or back to the spawn point, and the watch 0x1f5c3c shared with state 15 (docs/re/scripts-missions-1-4-spawners-conduct.md):
// the enter 0x2a11a0, the decision 0x2b68e8 around the path follower 0x1faf04, the angle test 0x15c1b0 of the watch. Pure steps on plain values;
// goon_conduct.cpp applies them. No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>

namespace chase::runtime {

constexpr std::uint32_t kAiPatrolRetryBase = 10;       // 0x2b6ac8: ctrl+0x84 = 0x235498(10) + 10 after a blocked step
constexpr std::uint32_t kAiPatrolRetrySpan = 10;       // 0x235498(10) = the next word of the game random modulo 10 (0x25c1b8)
constexpr float kAiWatchAngleDegrees = 45.0f;          // literal 0x1f5cc4 and 0x2b6b34, the last argument of 0x1f8d5c
constexpr float kAiPiSingle = 3.14159274f;             // literal 0x1f90c4
constexpr float kAiRadiansPerDegreeStep = 0.00555555569f;  // literal 0x1f90cc: degrees x pi x (1 / 180)
constexpr std::uint32_t kAiAngleTinyLengthBits = 0x34000000;  // 0x257ea0: a vector this short or shorter normalises to (0, 0, 1)

// STAND-IN SCRIPT-S333: the random word of the blocked step is drawn from the conduct's own generator with this seed; the original draws from
// the game's shared stream 0x25b2b8, which the port does not model across systems.
constexpr std::uint32_t kAiPatrolRetrySeedStandIn = 0x2a11a0;

// The value of the low byte the follower 0x1faf04 returns.
enum class AiFollowOutcome : std::uint8_t { Arrived, Blocked, Moving };
AiFollowOutcome aiFollowOutcome(int code);

enum class AiPatrolSource : std::uint8_t { None, PatrolObject, SpawnPoint };

struct AiPatrolEnterInput {
    std::uint32_t patrolObject = 0;     // ctrl+0xd4: the node the character walks to
    std::uint32_t pathParameter = 0;    // `AIPatrolPath` (0x24242c at 0x2a11cc), read only while ctrl+0xd4 is 0
    bool returnToSpawnPoint = false;    // ctrl+0xdc bit 5
    bool dontRun = false;               // `AIDontRun`, ctrl+0xdb bit 4
    bool walk = false;                  // ctrl+0xd9 bit 5
};
struct AiPatrolEnterStep {
    std::uint32_t patrolObject = 0;
    bool walk = false;
    bool patrolling = false;            // the log "Patrolling to %s" of 0x2a11ec
    AiPatrolSource source = AiPatrolSource::None;
};
// 0x2a11a0 up to the destination: without a node the character reads `AIPatrolPath` and walks (bit 5 of ctrl+0xd9 set), else with
// `ReturnToSpawnPoint` the bit takes `AIDontRun`; the destination is the node, else the spawn point ctrl+0xac when `ReturnToSpawnPoint`.
AiPatrolEnterStep aiPatrolEnter(const AiPatrolEnterInput& input);

struct AiPatrolArrival {
    bool idle = false;                  // AI state 0 (0x2b69b0..0x2b6a2c, nothing else)
    std::uint32_t patrolObject = 0;     // the new ctrl+0xd4 when AI state 15 follows
};
// 0x2b6998: arrived without a node and with `ReturnToSpawnPoint` the character becomes idle; else ctrl+0xd4 takes `next_Node` of the node
// (0 without a node) and AI state 15 follows.
AiPatrolArrival aiPatrolArrival(std::uint32_t patrolObject, bool returnToSpawnPoint, std::uint32_t nextNode);

// 0x2b6a38: ctrl+0x84 counts down while above 0; at 0 the character looks for a target (or becomes idle) and the count restarts at
// 10 + `random` modulo 10. True when the target search runs.
bool aiPatrolBlockedStep(std::uint32_t& countdown, std::uint32_t random);

// 0x15c1b0: the normalised direction from the viewer to the target, dotted with the viewer's forward vector (node +0x20), is above cos(angle).
// The angle in radians is 0x1f8d5c's last argument times pi times 1 / 180 (0x1f8d6c..0x1f8d94).
float aiWatchAngle(float degrees);
bool aiAngleTest(const std::array<float, 3>& viewer, const std::array<float, 3>& forward, const std::array<float, 3>& target, float angle);

}
