#pragma once
// Dropped studs (docs/re/studs.md, entry 2026-10-19 "Dropped studs ported"): the modes 0..2 of the pickup burst 0x21e2e4 with the spawner 0x19913c
// on the records 0..19 of the pickup pool (0x2ecee8), their update in 0x180168 (pickup test, flight, bounce, landing sound, life, collect flight) and
// their draw rule in 0x17fc38. The bricks of mode 3 are runtime/debris.hpp; both share the wall planes and the record choice. Positions and velocities
// are per update. No SDL dependency.
#include "runtime/debris.hpp"
#include "runtime/game_random.hpp"
#include "runtime/studs.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

// 0x21e2e4: modes 0..2 take the records 0..0x13 (r1 = 0, r2 = 20).
inline constexpr unsigned kStudDropRecordCount = 0x14;
// The pickup type of a dropped heart (mode 2) and its stud values: 0x19913c mode 0 takes 100 for blue, 10 for gold, 1 for silver from the count.
inline constexpr std::uint8_t kStudDropHeartType = 4;
inline constexpr unsigned kStudDropBlueUnits = 100, kStudDropGoldUnits = 10;
// The skip counter (+0x30) every caller of this port passes as the fourth stack word (0x2149ac, 0x18eb94, 0x18ec60): 30 updates before the first test.
inline constexpr std::int16_t kStudDropDefaultSkip = 30;
// 0x180618: the landing sound waits 0x23ca4c() x 2 = 60 updates (halfword 0x377282, counted down at the start of 0x180168); type table +0x88 of the
// studs is 65 (pickups/coinsland, read in the oracle, studs.md 2026-10-13T17:09:41); its frequency scale is 0.95 + 0.1 r (0x22f770).
inline constexpr std::uint16_t kStudLandingSound = 65;
inline constexpr unsigned kStudLandingSoundWait = 60;
inline constexpr float kStudLandingFrequencyBase = 0.95f, kStudLandingFrequencyRange = 0.1f;
// 0x2277c4: the collect sound gets the volume 0.7 (0x219178, literal 0x227abc).
inline constexpr float kStudCollectVolume = 0.7f;
// 0x17fc38: a resting dropped stud (+0x45 = 0) is drawn while its life is at least 2.125 s, below that on bit 2 of int(30 x life).
inline constexpr float kStudDropBlinkLife = 2.125f;

// The mode byte r0 of 0x21e2e4: 0 counts the studs out of the count, 1 becomes 2 when the next random word is a multiple of three and 0 otherwise
// (0x21e6ac), 2 drops a heart first and goes on as mode 0 (0x199990).
enum class StudDropMode : std::uint8_t { Count = 0, Random = 1, Heart = 2 };

// The arguments of 0x21e2e4(s0 spread, s1, r0 mode, r1 count, r2 position, r3 direction, stack 0, velocity flag, 0, skip, collect flag, s16).
struct StudDropRequest {
    StudDropMode mode = StudDropMode::Random;
    unsigned count = 0;
    DebrisVec3 position{};                 // 0x21e2e4 adds 0.5 to y
    DebrisVec3 direction{};                // zero (0x56d400) means none
    float spread = 1.0f;                   // s0: SpawnCone for 0x2149ac, 1.0 for the character
    bool directionIsVelocity = false;      // stack word 1: 0x19913c copies r3 as the velocity instead of drawing it
    std::int16_t skip = kStudDropDefaultSkip;  // stack word 3: the record's skip counter +0x30
    bool collectAtOnce = false;            // bit 0 of the byte stack word 4 points to: record +0x49 bit 2, collected at the next test
    bool slow = false;                     // 0x37727e: entity word +0xc bit 22 (0x2149ac), cleared after every 0x21e2e4
};

enum class StudDropState : std::uint8_t { Free = 0, Resting = 1, Flying = 2, Collecting = 4 };

struct StudDropRecord {
    DebrisVec3 position{};        // +0x00
    DebrisVec3 velocity{};        // +0x0c
    std::int16_t skip = 0;        // +0x30
    float ground = 0;             // +0x38
    float life = 0;               // +0x3c: seconds; the collect timer in state 4
    StudDropState state = StudDropState::Free;  // +0x44
    std::uint8_t type = 0;        // +0x46: 0..3 stud, 4 heart
    std::int8_t plane = -1;       // +0x48
    std::uint8_t flags49 = 0;     // +0x49: bit 1 slow, bit 2 collect at once
};

struct StudDropSound {
    std::uint16_t id = 0;
    DebrisVec3 position{};
    float frequencyScale = 1.0f;  // 0x22f770
    float volume = 1.0f;          // 0x219178
};

// What one update did: the collect sounds (0x2277c4, the type's +0x8a) and the landing sounds (0x180618).
struct StudDropUpdate {
    std::vector<StudDropSound> sounds;
    unsigned collected = 0;
};

class StudDropField {
public:
    StudDropField(GameRandom& random, DebrisPlaneRing& planes) : random_(random), planes_(planes) {}

    float scale = 1.0f;  // 0x37728c

    // 0x21e2e4 modes 0..2 and 0x19913c. Returns the mode 0x19913c ran (mode 1 resolved). `wall` is the test of 0x22b108, `ground` the one of 0x21ab20.
    StudDropMode drop(const StudDropRequest& request, const DebrisLineTest& wall, const DebrisLineTest& ground);
    // 0x180168 for the records 0..19 once per game update; `frameDelta` is 0x23602c; collected studs bank into `counter` (0x28a128) and fly towards
    // `flightTarget` (the HUD sink 0, as StudField). STAND-IN STUD-S6: hearts are not picked up.
    StudDropUpdate update(float frameDelta, const std::array<float, 3>& pickupPoint, const std::optional<std::array<float, 3>>& flightTarget,
                          StudField& counter);
    // 0x17fc38: whether the record is drawn (states 1, 2 and 4; a resting record blinks below kStudDropBlinkLife).
    bool drawn(unsigned index) const;

    const std::array<StudDropRecord, kStudDropRecordCount>& records() const { return records_; }
    void setRecord(unsigned index, const StudDropRecord& record) { records_.at(index) = record; }
    unsigned landingSoundWait() const { return landingWait_; }

private:
    GameRandom& random_;
    DebrisPlaneRing& planes_;
    std::array<StudDropRecord, kStudDropRecordCount> records_{};
    unsigned landingWait_ = 0;
};

// 0x21e2e4 lines 0x21e340..0x21e388 and 0x19913c lines 0x199184..0x1991cc: a direction with x or z not 0 becomes the angle byte
// int((atan2(x, z) + pi) x 64 / pi) and back (sin(a), 0, cos(a)) x scale with a = byte x pi / 64 - pi; none for x = z = 0.
std::optional<DebrisVec3> studDropDirection(const DebrisVec3& direction, float scale);

// 0x2149ac(entity, SpawnLoc node, count, forward flag, mode flag): the request it hands to 0x21e2e4 (none for count 0). `spawnDir` is the int
// `SpawnDir` (999 none), `spawnLocal` the int `SpawnLocal`, `spawnCone` the float `SpawnCone` (1.0); `node` the matrix of the SpawnLoc child (else
// the entity's), row major with the translation in row 3. Without SpawnDir the direction is the node's row 2 when `forward`, else none. STAND-IN
// STUD-S6: the hook [0x377278 + 0x28] is taken as unset.
struct StudSpawnParameters {
    unsigned count = 0;
    bool forward = false;
    bool modeCount = false;   // stack word 0 of 0x2149ac: mode 0, else mode 1
    float spawnCone = 1.0f;
    std::int32_t spawnDir = 999;
    std::int32_t spawnLocal = 0;
    bool slow = false;
    std::array<float, 16> node{};
};
std::optional<StudDropRequest> studSpawnRequest(const StudSpawnParameters& parameters);

}
