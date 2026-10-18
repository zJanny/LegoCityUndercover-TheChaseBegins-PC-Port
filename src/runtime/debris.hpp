#pragma once
// The brick debris of the original (docs/re/vehicles-damage.md): dropped pickup records of type 6 in the reserved part of the pickup pool
// (records 20..43 of the 44 at the pool's start, 0x2ecee8), drawn by the twenty level objects Debris01..Debris20 of charbits.blvl. The burst
// is 0x21e2e4 with mode 3 (0x229fb8 for a vehicle contact, 0x20c938 for a destroyed object) and the spawner 0x19913c; the records move in
// 0x180168 (state 2 flies, state 1 rests) and 0x28a4b0 puts their debris object into place. Positions and velocities are per update, not per
// second (0x23d39c adds the velocity once per update). Not the particle system (runtime/particles.*). No SDL dependency.
#include "runtime/game_random.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace chase::runtime {

using DebrisVec3 = std::array<float, 3>;

// 0x21e2e4: mode 3 spawns from the records 0x14..0x2b of the reserved block (0x2c records, 0x2ecee8 allocates 0xd10 bytes).
inline constexpr unsigned kDebrisRecordFirst = 0x14;
inline constexpr unsigned kDebrisRecordCount = 0x2c - 0x14;
// 0x2ecf30: twenty debris objects Debris01..Debris20 (table 0x58f590, in use 0x58f630).
inline constexpr unsigned kDebrisObjectCount = 20;
// 0x19913c: the planes of the wall test (0x58f6d0, index 0x377294 & 3).
inline constexpr unsigned kDebrisPlaneCount = 4;
// 0x342d04 copies ten record indices into the descriptor; 0x19913c stops after the tenth.
inline constexpr unsigned kDebrisBurstLimit = 10;
// 0x229fb8 and the default of 0x20c938: six bricks.
inline constexpr unsigned kDebrisDefaultCount = 6;
// 0x20c938 default colours: DebrisColour 0xffffffff, DebrisColour2 0xffff00ff (r 255, g 0, b 255 means "no second colour", 0x199644..0x19970c).
inline constexpr std::uint32_t kDebrisDefaultColour = 0xffffffffu;
inline constexpr std::uint32_t kDebrisNoSecondColour = 0xffff00ffu;
// The pickup type of a brick (record +0x46) and its row of the type table 0x58ef7c (0x2ed0b4: +0x7c damping 0, +0x80 gravity 1.0, +0x84 bounce
// 0.3, +0x88 landing sound 0).
inline constexpr std::uint8_t kDebrisPickupType = 6;
inline constexpr float kDebrisTypeGravity = 1.0f;
inline constexpr float kDebrisTypeBounce = 0.3f;
inline constexpr float kDebrisTypeDamping = 0.0f;

enum class DebrisState : std::uint8_t { Free = 0, Resting = 1, Flying = 2 };

struct DebrisRecord {
    DebrisVec3 position{};          // +0x00
    DebrisVec3 velocity{};          // +0x0c, units per update
    std::uint32_t colour = 0;       // +0x18, bytes r, g, b, a
    std::uint8_t object = 0xff;     // +0x1c: debris object slot, 0xff none
    std::uint8_t flags22 = 0;       // +0x22: bit 0 random (spin mode 5), bit 1 super brick (0x2c8278), bit 2
    float ground = 0;               // +0x38: the height the brick rests at
    float life = 0;                 // +0x3c: seconds
    DebrisState state = DebrisState::Free;  // +0x44
    std::uint8_t type = 0;          // +0x46
    std::int8_t plane = -1;         // +0x48: the wall plane of the burst, -1 none
    std::uint8_t flags49 = 0;       // +0x49: bit 1 the slow burst (entity word +0xc bit 22)
};

// One of Debris01..Debris20: the object's node as 0x28a4b0 leaves it.
struct DebrisObject {
    bool enabled = false;                    // 0x242cf0 / 0x2428c0
    std::array<float, 16> matrix{};          // node matrix, row major with the translation in row 3 (0x19b498, then 0x28a4b0)
    std::uint32_t colour = 0xffffffffu;      // node colour of 0x23e110: r, g, b, a bytes
    std::uint8_t alpha = 255;                // 0x23f124
    float spinSpeed = 0;                     // 0x23afc0 speed of the "spin" clip: 0.8 + 0.4 r
    unsigned spinMode = 0;                   // 1, or 5 with record +0x22 bit 0
};

struct DebrisLineHit {
    DebrisVec3 point{};
    DebrisVec3 normal{};
};
// A line test from `from` to `to`: 0x21ab20 (nearest hit) for the ground, 0x22b108 (any hit) for the wall of a burst.
using DebrisLineTest = std::function<std::optional<DebrisLineHit>(const DebrisVec3& from, const DebrisVec3& to)>;

// The descriptor of 0x21e2e4 with mode 3 and the colour block of 0x229fb8 / 0x20c938.
struct DebrisBurst {
    DebrisVec3 position{};                         // 0x21e2e4 adds 0.5 to y
    std::optional<DebrisVec3> direction;           // 0x56d400 (zero) for both callers: no direction
    unsigned count = kDebrisDefaultCount;
    std::uint32_t colour = kDebrisDefaultColour;
    std::uint32_t secondColour = kDebrisNoSecondColour;
    float ratio = 0;                               // DebrisRatio; 0 picks the second colour with probability 1/2
    float spread = 1.0f;                           // s0 of 0x21e2e4 (1.0 for both callers)
    bool slow = false;                             // 0x37727e: entity word +0xc bit 22 (0x20c938)
};

// STAND-IN VEH-S104: 0x3772a8 and 0x3772ac as the oracle held them (5.0 and 10.0 in the states veh-smash-car and veh-smash-prop; 0 in the data
// image, the writer is not read).
inline constexpr float kDebrisFadeNearMeasured = 5.0f;
inline constexpr float kDebrisFadeFarMeasured = 10.0f;

struct DebrisSettings {
    float scale = 1.0f;          // 0x37728c (1.0 in the data image and in the oracle)
    float fadeNear = kDebrisFadeNearMeasured, fadeFar = kDebrisFadeFarMeasured;  // the alpha of a flying brick is (distance - near) / (far - near), clamped
    float rate = 30.0f;          // 0x23ca4c
};

class DebrisField {
public:
    explicit DebrisField(GameRandom& random) : random_(random) {}

    DebrisSettings settings;

    // 0x21e2e4 (mode 3) and 0x19913c. `wall` is the test of 0x22b108, `ground` the one of 0x21ab20.
    void burst(const DebrisBurst& burst, const DebrisLineTest& wall, const DebrisLineTest& ground);
    // 0x180168 for the brick records and 0x28a4b0 for their objects, once per game update; `frameDelta` is 0x23602c, `camera` the position of
    // the camera node (0x22e1f4).
    void update(float frameDelta, const DebrisVec3& camera);

    const std::array<DebrisRecord, kDebrisRecordCount>& records() const { return records_; }
    // Puts a recorded record (pool index kDebrisRecordFirst + index) into the field, with its debris object enabled.
    void setRecord(unsigned index, const DebrisRecord& record);
    const std::array<DebrisObject, kDebrisObjectCount>& objects() const { return objects_; }
    const std::array<std::array<float, 4>, kDebrisPlaneCount>& planes() const { return planes_; }

private:
    std::array<std::uint8_t, kDebrisBurstLimit> chooseRecords() const;
    void release(DebrisRecord& record);
    void place(DebrisRecord& record);

    GameRandom& random_;
    std::array<DebrisRecord, kDebrisRecordCount> records_{};
    std::array<DebrisObject, kDebrisObjectCount> objects_{};
    std::array<std::array<float, 4>, kDebrisPlaneCount> planes_{};
    unsigned planeIndex_ = 0;
    DebrisVec3 cameraPosition_{};
};

// The colour of a float3 class value (DebrisColour, DebrisColour2): each component converted to an integer and its low byte kept, alpha 0xff
// (0x20c938, 0x179e9c..0x179ed8).
std::uint32_t debrisColourOf(const std::array<float, 3>& value);
// 0x237c48: the rotation rows of the Euler angles (a, b, c) in radians.
std::array<float, 9> debrisRotation(float a, float b, float c);

}
