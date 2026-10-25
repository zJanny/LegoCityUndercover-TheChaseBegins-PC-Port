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

// States 3 (magnet) and 4 (collected) belong to the pickup of a super brick (runtime/debris_pickup.hpp); DebrisField::update leaves them alone.
enum class DebrisState : std::uint8_t { Free = 0, Resting = 1, Flying = 2, Magnet = 3, Collected = 4 };

// The super brick block (0x2cdca4, 0x22a048 for a vehicle contact) and the SBDebris objects (0x2b3c48: SBDebris01..SBDebris20 of charbits.blvl,
// pickups of type 103 with SuperBuildValue 1).
inline constexpr std::uint8_t kSuperDebrisKind = 103;   // 'g': block +4, record +0x1e
inline constexpr unsigned kSuperDebrisObjectCount = 20;
inline constexpr std::uint8_t kSuperDebrisUnitValue = 1;      // *0x3743f0 in the data image and the oracle
inline constexpr std::uint8_t kSuperDebrisLargeValue = 0xff;  // *0x3743f1 in the data image and the oracle
inline constexpr std::uint8_t kSuperDebrisPropCount = 9;      // 0x2cdca4: block +6
inline constexpr std::uint8_t kSuperDebrisKindValue = 6;      // 0x2cdca4: the value of the kinds 0x55, 0x77, 0x8a, 0xd6 without SuperBuildValue
inline constexpr std::int32_t kSuperDebrisNoSpawnDir = 999;   // 0x2c8278: the default of SpawnDir
inline constexpr std::uint8_t kNoSuperDebrisObject = 0xff;

struct DebrisRecord {
    DebrisVec3 position{};          // +0x00
    DebrisVec3 velocity{};          // +0x0c, units per update
    std::uint32_t colour = 0;       // +0x18, bytes r, g, b, a
    std::uint8_t object = 0xff;     // +0x1c: debris object slot, 0xff none
    std::uint8_t phase = 0;         // +0x1d: 1 for a brick; a super brick's spin phase (0x1998dc)
    std::uint8_t kind = 0;          // +0x1e: kSuperDebrisKind for a super brick (0x2c8278)
    std::uint16_t superValue = 0;   // +0x20: the value of a super brick (0x2c8310, strh)
    std::uint8_t flags22 = 0;       // +0x22: bit 0 random (spin mode 5), bit 1 super brick (0x2c8278), bit 2 pickable (0x2c4f9c)
    std::int16_t skip = 0;          // +0x30: the pickup test's skip counter (0 at the spawn, 0x1997c0)
    std::uint8_t superObject = kNoSuperDebrisObject;  // the SBDebris object (0..19 for SBDebris01..20) in the slot table 0x58f630 entry of `object`
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

// 0x19913c: the wall planes (normal, -dot(point, normal)) of the bursts in the ring 0x58f6d0, next slot 0x377294 & 3; the bricks and the dropped
// studs (runtime/stud_drop.hpp) share it.
struct DebrisPlaneRing {
    std::array<std::array<float, 4>, kDebrisPlaneCount> planes{};
    unsigned next = 0;
};

// The rays of one 0x19913c call before its records: `start` = position + direction x 2.5, `end` = start - 10 in y, both moved to the wall hit of
// 0x22b108(position, start); `restHeight` = the ground test's hit y (else the reference y - 20) + 0.56 x scale, the reference y being the end's for
// the bricks (mode 3) and the start's otherwise; `plane` the ring slot of the wall hit, -1 without one.
struct DebrisBurstRays {
    DebrisVec3 start{}, end{};
    float restHeight = 0;
    std::int8_t plane = -1;
};
// `direction` is the burst direction times 0x37728c, zero without one.
DebrisBurstRays debrisBurstRays(const DebrisVec3& position, const DebrisVec3& direction, bool bricks, float scale, const DebrisLineTest& wall,
                                const DebrisLineTest& ground, DebrisPlaneRing& ring);
// The second ground test of 0x19913c from start and end moved 2 units along the horizontal velocity (0x257ea0): hit y + 0.56 x scale, else `fallback`.
float debrisAheadGround(const DebrisBurstRays& rays, const DebrisVec3& velocity, float scale, const DebrisLineTest& ground, float fallback);
// 0x180168 state 2 with a plane: when position + velocity lies behind it the velocity is reflected (u = -normalise(v), v = (2 (u . n) n - u) |v|).
void debrisReflectAtPlane(const DebrisVec3& position, DebrisVec3& velocity, const std::array<float, 4>& plane);

// 0x21e2e4 lines 0x21e3c0..0x21e598: the pool records of one mode by state (a stable counting sort), the records of states 1 to 4 then by pickup type
// and remaining life (+0x3c), both ascending; 0x342d04 keeps the first ten indices.
struct PickupSortKey {
    std::uint8_t state = 0, type = 0;
    float life = 0;
};
std::array<std::uint8_t, kDebrisBurstLimit> choosePickupRecords(const PickupSortKey* keys, unsigned count);

// The block of 0x2cdca4 / 0x179e4c {object, kind, value, count} that the colour block hands to 0x2c8278 at +0x14, with the parameters 0x2c8278
// reads from the block's object: SpawnCone (float, 1.0), SpawnLocal (int, 0), SpawnDir (int degrees, 999 none) and the object's node matrix
// (row major, translation in row 3).
struct DebrisSuperBlock {
    std::uint8_t kind = kSuperDebrisKind;
    std::uint8_t value = 0;
    std::uint8_t count = 0;
    float spawnCone = 1.0f;
    std::int32_t spawnLocal = 0;
    std::int32_t spawnDir = kSuperDebrisNoSpawnDir;
    std::array<float, 16> node{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

// 0x2cdca4 for a destroyed object while 0x3743f2 is set: `superBuildValue` the integer SuperBuildValue of the object (0 none), `kind` the entity
// kind byte +0x12. The value is SuperBuildValue's low byte, else 6 for the kinds 0x55, 0x77, 0x8a and 0xd6; none otherwise. Count 9.
std::optional<DebrisSuperBlock> superDebrisBlock(std::int32_t superBuildValue, std::uint8_t kind);

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
    std::optional<DebrisSuperBlock> super;         // colour block +0x14; 0x2c8278 is asked for every brick that found a debris slot
};

// STAND-IN VEH-S104: 0x3772a8 and 0x3772ac as the oracle held them (5.0 and 10.0 in the save state in a police van in front of moving cars and the save state in a police van in front of a lamp post; 0 in the data
// image, the writer is not read).
inline constexpr float kDebrisFadeNearMeasured = 5.0f;
inline constexpr float kDebrisFadeFarMeasured = 10.0f;

struct DebrisSettings {
    float scale = 1.0f;          // 0x37728c (1.0 in the data image and in the oracle)
    float fadeNear = kDebrisFadeNearMeasured, fadeFar = kDebrisFadeFarMeasured;  // the alpha of a flying brick is (distance - near) / (far - near), clamped
    float rate = 30.0f;          // 0x23ca4c
    std::uint8_t superUnit = kSuperDebrisUnitValue;    // *0x3743f0
    std::uint8_t superLarge = kSuperDebrisLargeValue;  // *0x3743f1
};

class DebrisField {
public:
    explicit DebrisField(GameRandom& random) : random_(random) {}
    // The wall planes shared with the dropped studs (runtime/stud_drop.hpp).
    DebrisField(GameRandom& random, DebrisPlaneRing& planes) : random_(random), sharedRing_(&planes) {}

    DebrisSettings settings;

    // 0x21e2e4 (mode 3) and 0x19913c. `wall` is the test of 0x22b108, `ground` the one of 0x21ab20. Returns the number of super bricks.
    unsigned burst(const DebrisBurst& burst, const DebrisLineTest& wall, const DebrisLineTest& ground);
    // 0x180168 for the brick records and 0x28a4b0 for their objects, once per game update; `frameDelta` is 0x23602c, `camera` the position of
    // the camera node (0x22e1f4).
    void update(float frameDelta, const DebrisVec3& camera);

    const std::array<DebrisRecord, kDebrisRecordCount>& records() const { return records_; }
    std::array<DebrisRecord, kDebrisRecordCount>& records() { return records_; }
    // Puts a recorded record (pool index kDebrisRecordFirst + index) into the field, with its debris object enabled.
    void setRecord(unsigned index, const DebrisRecord& record);
    const std::array<DebrisObject, kDebrisObjectCount>& objects() const { return objects_; }
    std::array<DebrisObject, kDebrisObjectCount>& objects() { return objects_; }
    // SBDebris01..SBDebris20; `superPresent` tells which of them the level has.
    const std::array<DebrisObject, kSuperDebrisObjectCount>& superObjects() const { return superObjects_; }
    std::array<DebrisObject, kSuperDebrisObjectCount>& superObjects() { return superObjects_; }
    bool superPresent(unsigned index) const { return superValues_.at(index).has_value(); }
    // The object the slot table 0x58f630 holds for the record's slot: its SBDebris object for a super brick, else Debris01..20; null without one.
    DebrisObject* objectOf(const DebrisRecord& record);
    // The free path of 0x180168 (0x180874..0x1808b4): state 0 and, for a brick with a slot, its object disabled and the slot entry cleared.
    void freeRecord(unsigned index);

    // 0x2b3c48: the SBDebris objects with their SuperBuildValue (none for a missing one); value superUnit goes to the table 0x515aa8, any other to
    // 0x515b48 (*0x3743f1 takes the first such value while it is 0). All are disabled.
    void loadSuperObjects(const std::array<std::optional<std::uint16_t>, kSuperDebrisObjectCount>& values);
    // 0x2c51e4 (and the tail of 0x2cdca4): the lists 0x515be8 / 0x515c3c of the disabled objects of the two tables, in table order, cursor 0.
    void listFreeSuperObjects();
    // 0x2c4f9c for every super brick record with a slot (0x180c04..0x180c24, after the record's step): states 1 and 3 set +0x22 bit 2, spin the
    // object about y by phase / 255 x 2 pi - pi x `clockSeconds` and mix its node colour from the record colour to `rainbow` (*0x3743f8) by
    // min((1 - rate x life / 255) x 10 / 3, 1); state 4 sets the record colour.
    void animateSuper(float clockSeconds, std::uint32_t rainbow);
    const std::array<std::array<float, 4>, kDebrisPlaneCount>& planes() const { return sharedRing_ ? sharedRing_->planes : ownRing_.planes; }

private:
    struct SuperList {
        std::array<std::uint8_t, kSuperDebrisObjectCount> entries{};
        std::uint8_t count = 0, next = 0;
    };
    void release(DebrisRecord& record);
    void place(DebrisRecord& record);
    bool slotFree(unsigned slot) const;
    bool takeSuper(DebrisSuperBlock& block, DebrisRecord& record);

    GameRandom& random_;
    std::array<DebrisRecord, kDebrisRecordCount> records_{};
    std::array<DebrisObject, kDebrisObjectCount> objects_{};
    std::array<std::uint8_t, kDebrisObjectCount> slotSuper_ = filledSlots();
    std::array<DebrisObject, kSuperDebrisObjectCount> superObjects_{};
    std::array<std::optional<std::uint16_t>, kSuperDebrisObjectCount> superValues_{};
    std::array<std::uint8_t, kSuperDebrisObjectCount> unitTable_{}, largeTable_{};
    std::uint8_t unitCount_ = 0, largeCount_ = 0;
    SuperList unitList_, largeList_;
    static std::array<std::uint8_t, kDebrisObjectCount> filledSlots() {
        std::array<std::uint8_t, kDebrisObjectCount> slots{};
        slots.fill(kNoSuperDebrisObject);
        return slots;
    }
    DebrisPlaneRing& ring() { return sharedRing_ ? *sharedRing_ : ownRing_; }

    DebrisPlaneRing ownRing_;
    DebrisPlaneRing* sharedRing_ = nullptr;
    DebrisVec3 cameraPosition_{};
};

// The colour of a float3 class value (DebrisColour, DebrisColour2): each component converted to an integer and its low byte kept, alpha 0xff
// (0x20c938, 0x179e9c..0x179ed8).
std::uint32_t debrisColourOf(const std::array<float, 3>& value);
// 0x237c48: the rotation rows of the Euler angles (a, b, c) in radians.
std::array<float, 9> debrisRotation(float a, float b, float c);

}
