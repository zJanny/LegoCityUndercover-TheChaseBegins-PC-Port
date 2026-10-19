#pragma once
// The detective tool (docs/re/abilities-detective.md): the detective spot (ObjectType 182, props/ability_detective), the reveal spot it shows
// (ObjectType 238, props/ability_detective_reveal_spot), the reveal prop that spot fires (ObjectType 74, ability_detective_reveal_dirtmound /
// _trapdoor ...), the footprints along the spot's DetectivePath and the player's detective states 0x117, 0x118 and 0x1a2. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/collision.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kDetectiveSpotType = 182;  // descriptor 0x16f134
constexpr std::uint32_t kRevealSpotType = 238;     // descriptor 0x1ad1b4
constexpr std::uint32_t kRevealPropType = 74;      // descriptor 0x1a97bc

// Player clips (tablesanimations sheet 1): 0x28c080 plays 0x113, 0x2830b4 plays 0x114, the X-spot use state 0x31ec68 plays CharacterUseAnimation (0x115).
constexpr int kAnimationDetectOut = 275;
constexpr int kAnimationDetectIn = 276;
constexpr int kAnimationDetectFind = 277;
// Role variant 6 of tablesanimations sheet 2 replaces the role while ext+0x130 bit 2 is set (0x219614 at 0x2196d0); 0x271 means none.
constexpr std::size_t kDetectiveRoleVariant = 6;

// Footprints (post-link 0x177a4c): ceil(path length x 1.6666666) of them (literal 0x3fd55555, ceil 0x253618), the path length summed over 10
// steps per segment (0x22a6c0 / 0x293540), side offset -0.45 for even and +0.45 for odd footprints (literals at 0x177d8c / 0x177d7c) along
// (tangent z, tangent y, tangent x) as the code builds it, stored as shorts of 4 per unit relative to the spot (0x177ca4, literal 0x177d90).
constexpr float kFootprintsPerUnit = 1.6666666f;
constexpr unsigned kPathLengthSteps = 10;
constexpr float kFootprintSide = 0.45f;
constexpr float kFootprintUnitsPerStep = 4.0f;
// Each footprint also stores the path's orientation at its position (0x270b70) as four signed bytes, x127 (literal 0x177d88); the draw
// (0x26024c) reads them back with the factor 0x3c010204 (1/127), makes the matrix with 0x248870 and puts the ray hit point into its translation.
constexpr float kFootprintRotationScale = 127.0f;
constexpr float kFootprintRotationInverse = 0.00787402f;
// Scan 0x1c7bec: a footprint in front of the player (dot(forward, player - footprint) < 0) and closer than 2.5 horizontally is revealed; the spot
// stays in detective mode while any footprint is closer than 8 (squared 6.25 and 64, literals 0x40c80000 / 0x42800000 compared as integers).
constexpr float kFootprintRevealDistanceSquared = 6.25f;
constexpr float kFootprintNearDistanceSquared = 64.0f;
// Footprint fade 0x1ba504: 8 per update from 255, freed when below 10; 16 pool entries per FootprintType (0x26024c) out of the 32 entities at
// 0x515cf0 and the 32-bit set at 0x515d70.
constexpr std::uint8_t kFootprintFadeStep = 8;
constexpr unsigned kFootprintPoolPerType = 16;
constexpr unsigned kFootprintPoolSize = 32;
// Sound of the change to state 2 of a detective spot (0x1c7e88, literal 0x1c8000), played at the spot.
constexpr std::uint16_t kSoundDetectiveRevealed = 0x2ee;

// Player detective states, in updates from the update of the request (oracle, docs/re/abilities-detective.md): 0x117 "DETECTIVE OUT ENTER"
// returns to state 1 after 37 updates (detect_out lasts 36), 0x118 "DETECTIVE IN ENTER" after 19 (detect_in lasts 18); the X-spot use state
// 0x1a2 raises its animation event (0x32e964, value above 0.5) after 43 updates and ends (0x32ec40) after 116.
// STAND-IN DETECT-S1: the use state's numbers are measured, not derived: its approach phase (0x2366bc, 0x207be4) and the event table were not read.
constexpr unsigned kDetectiveOutUpdates = 37;
constexpr unsigned kDetectiveInUpdates = 19;
constexpr unsigned kXSpotUseEventUpdate = 43;
constexpr unsigned kXSpotUseUpdates = 116;

// Reveal spot defaults (create 0x1c7a98): RevealTime 1.0. Reveal prop (create 0x1c4408, post-link 0x1b6228): RevealDelay read as an integer (default 1)
// times the frame rate 30 (0x23ca4c), RevealTime default 0.5 times 30, RevealType default 0, ATTR_SFXObjectSpawn / ATTR_SFXObjectDone.
constexpr float kRevealSpotDefaultTime = 1.0f;
constexpr float kRevealPropDefaultTime = 0.5f;
constexpr int kRevealPropDefaultDelay = 1;
constexpr int kRevealFramesPerSecond = 30;

struct DetectiveFootprintRecord {
    std::array<std::int16_t, 3> offset{};
    std::array<std::int8_t, 4> rotation{};
};
// Footprint offsets of a DetectivePath relative to the spot position and the quantised path orientation, as the post-link stores them.
std::vector<DetectiveFootprintRecord> detectiveFootprints(const assets::LevelPath& path, const float spot[3]);
// Path orientation 0x270b70: spherical interpolation (0x21a600, shortest way, linear above dot 0x3f7fff58) between the quaternion records of the
// two points around `position`; the identity (0, 0, 0, 1) when the path has none.
void detectivePathOrientation(const assets::LevelPath& path, float position, float out[4]);
// The ground of a footprint (0x26024c): the nearest hit of the ray test 0x21ab20 from 1 above to 1 below `point` (0x2501ac with +1.0 / -1.0), in the
// hit point's height; nothing when the ray hits nothing (the original then keeps whatever its stack held).
// STAND-IN DETECT-S9: the test is the camera's query (CameraResetProbe, surface mask 0x01000010); the footprint's own query words (stack at
// 0x2603f4..0x260444, a word 512) were not decoded. It reproduces the 10 hit points of the oracle trap E-1 to 1e-4.
std::optional<float> footprintGroundHeight(const CollisionWorld& world, const float point[3]);
// Matrix 0x248870 makes from the stored bytes, in the layout of the original (rotation in floats 0..2, 4..6, 8..10, translation in 12..14).
std::array<float, 16> detectiveFootprintMatrix(const std::array<std::int8_t, 4>& rotation, const float translation[3]);
// Path length 0x22a6c0 (cumulative table) with `steps` per segment; the last entry is the total.
std::vector<float> detectivePathLengths(const assets::LevelPath& path, unsigned steps);
// Point index of the arc length fraction `t` (0x22a4c0).
float detectivePathParameter(const assets::LevelPath& path, const std::vector<float>& lengths, float t);
// Unit tangent of the type 0 path at `position` (derivative of 0x1c17c0, normalised by 0x257ea0).
void detectivePathTangent(const assets::LevelPath& path, float position, float out[3]);

struct DetectiveFootprint {
    std::array<std::int16_t, 3> offset{};
    std::array<std::int8_t, 4> rotation{0, 0, 0, 127};
    int slot = -1;           // pool entry, -1 when not shown
    std::uint8_t alpha = 0;  // 255 when revealed, minus 8 per update
};

struct DetectiveBox {
    float centre[3] = {0, 0, 0};
    float half[3] = {0, 0, 0};
    bool present = false;
};

struct DetectiveSpot {
    std::string instance;  // lower case
    std::size_t objectIndex = 0;
    float position[3] = {0, 0, 0};
    DetectiveBox useZone;       // class bound UseBound (0x23e130)
    DetectiveBox questionZone;  // class bound QuestionBound (record +0x20)
    std::string revealSwitch;   // RevealSwitch (+0xc)
    std::string triggerObject;  // TriggerObject (+0x30)
    unsigned footprintType = 0;
    std::vector<DetectiveFootprint> footprints;
    int state = 0, requested = 0;
    bool user = false;          // +0x10: the player entered detective mode here
    bool questionShown = false; // QuestionParticles enabled
    bool usable = true;         // use registry flag (0x236624)
};

struct RevealSpot {
    std::string instance;
    std::size_t objectIndex = 0;
    float position[3] = {0, 0, 0};
    DetectiveBox useZone;
    float revealTime = kRevealSpotDefaultTime;
    std::string fadeObject, revealObject, triggerObject;
    int state = 0, requested = 0;
    float timer = 0;
};

struct RevealProp {
    std::string instance;
    std::size_t objectIndex = 0;
    std::string revealObject, triggerObject;
    int delayUpdates = kRevealPropDefaultDelay * kRevealFramesPerSecond;
    int flightUpdates = int(kRevealPropDefaultTime * float(kRevealFramesPerSecond));
    int revealType = 0;
    std::uint16_t soundSpawn = 0, soundDone = 0;
    int state = 0, requested = 0;
    int counter = 0;     // +0xb6
    int flightMode = 0;  // +0xae of the flight (0 idle, 1 fade, 2 ballistic, 3..5 bounce, 6 landed)
    int flightCounter = 0;
    int bounceUpdates = 6;  // int(30 x 0.2) at +0xa4 (0x1b6228)
};

enum class DetectivePlayerState { Normal, Out, In, Use };

struct DetectiveInput {
    float dt = 0;
    float player[3] = {0, 0, 0};
    float forward[3] = {0, 0, 1};
    bool actionPressed = false;  // the A press reaches 0x2011b0 (player states 1 to 3)
    bool playerFree = true;      // the player is in a state that runs 0x2011b0 and is not carrying
};

enum class DetectiveActionKind {
    Show,      // 0x242cf0 on an object
    Hide,      // 0x2428c0 on an object
    Activate,  // 0x244f40: enable, message 0xff, ObjectTriggered
    Sound,     // positional sound
};

struct DetectiveAction {
    DetectiveActionKind kind = DetectiveActionKind::Show;
    std::string instance;
    std::uint16_t sound = 0;
    float position[3] = {0, 0, 0};
};

class DetectiveSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    // One game update: the objects in reverse level order (the order measured for one reveal prop and one reveal spot), then the player.
    std::vector<DetectiveAction> update(const assets::PlacedObjectState& state, const DetectiveInput& input);
    // Message 0xff to a reveal prop (0x1d20e0), as 0x244f40 or a script TriggerObject delivers it.
    void triggerMessage(const std::string& lowerInstance);

    bool modeActive() const { return modeActive_; }  // ext+0x130 bit 2 (and bit 3)
    bool controlTaken() const { return playerState_ != DetectivePlayerState::Normal; }
    DetectivePlayerState playerState() const { return playerState_; }
    unsigned playerStateUpdates() const { return playerUpdates_; }
    int playerClip() const;
    const std::vector<DetectiveSpot>& spots() const { return spots_; }
    const std::vector<RevealSpot>& revealSpots() const { return revealSpots_; }
    const std::vector<RevealProp>& revealProps() const { return revealProps_; }
    std::vector<DetectiveSpot>& spots() { return spots_; }
    std::vector<RevealSpot>& revealSpots() { return revealSpots_; }
    std::vector<RevealProp>& revealProps() { return revealProps_; }
    static bool inBox(const DetectiveBox& box, const float origin[3], const float point[3]);

private:
    void updateSpot(DetectiveSpot& spot, const DetectiveInput& input, std::vector<DetectiveAction>& out);
    bool scan(DetectiveSpot& spot, const DetectiveInput& input);
    void revealFootprint(DetectiveSpot& spot, std::size_t index);
    void fadeFootprints(DetectiveSpot& spot);
    void updateRevealSpot(RevealSpot& spot, const DetectiveInput& input, std::vector<DetectiveAction>& out);
    void updateRevealProp(RevealProp& prop, std::vector<DetectiveAction>& out);
    void activate(const std::string& target, std::vector<DetectiveAction>& out);
    void updatePlayer(const assets::PlacedObjectState& state, const DetectiveInput& input);
    void enterPlayerState(DetectivePlayerState next, bool fromObject = false);

    std::vector<DetectiveSpot> spots_;
    std::vector<RevealSpot> revealSpots_;
    std::vector<RevealProp> revealProps_;
    std::vector<bool> footprintPool_ = std::vector<bool>(kFootprintPoolSize, false);
    DetectivePlayerState playerState_ = DetectivePlayerState::Normal;
    unsigned playerUpdates_ = 0;
    bool modeActive_ = false;
    bool enteredByObject_ = false;
    int useSpot_ = -1;
};

// Clip of role `role` (its default clip, tablesanimations sheet 2 column 1) while ext+0x130 bit 2 is set; `role` when the variant is 0x271.
class DetectiveRoles {
public:
    explicit DetectiveRoles(const assets::Table& animations);
    int clip(int role) const;
    std::vector<int> clips() const;

private:
    std::vector<std::array<int, 7>> roles_;
};

}
