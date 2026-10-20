#pragma once
// The pedestrian character (kind 197, docs/re/pedestrians.md): create 0x1913f8, update slot 0xc 0x19188c = the steady
// callback 0x277c24 through 0x2424d0 (only while state == requested), then the state step 0x1d7e88; the walk of state 2
// is 0x2b9138 and its avoidance test 0x2edf08. What the update reads from the rest of the game comes in through
// PedestrianWalkWorld. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_manager.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// Component +0x14 .. +0x34 of the create, in that order: the clip names it loads from models/char_minifig/<AnimationClass>/
// (the dodge clips *_01 from CLASS_Generic/ when the class has no .bfnanm of that name).
enum class PedestrianClip : std::uint8_t { Walk, DodgeRight01, DodgeLeft01, DodgeRight02, DodgeLeft02, Bump01, Bump02, Splat, SplatUp };
const char* pedestrianClipName(PedestrianClip clip);

constexpr float kPedestrianTurnRate = 5.0f;              // 0x2b945c
constexpr float kPedestrianEntryLookAhead = 0.1f;        // 0x1d81cc / 0x1d81d0
constexpr float kPedestrianPathStep = 0.3f;              // 0x2b96b8 / 0x2b96bc
constexpr float kPedestrianReverseStep = 0.75f;          // 0x2b96a8 / 0x2b96ac
constexpr float kPedestrianSidestep = 1.3f;              // 0x2b96b0
constexpr std::uint8_t kPedestrianAvoidUpdates = 30;     // 0x2b959c
constexpr std::uint32_t kPedestrianHoldBase = 10;        // 0x1d7f68
constexpr std::uint32_t kPedestrianHoldRange = 50;       // 0x1d7f48
constexpr std::uint32_t kPedestrianHoldVisible = 10;     // 0x1d8274
constexpr float kPedestrianSplatSeconds = 3.0f;          // 0x1d81c0
constexpr float kPedestrianAvoidDistance = 2.0f;         // 0x2ee05c
constexpr float kPedestrianAvoidLateral = 1.0f;          // 0x2ee064
constexpr float kPedestrianTouchDistanceSquared = 1.5f;  // 0x3fc00000 in 0x1fd8c8
constexpr float kPedestrianPushStep = 0.1f;              // 0x1fd9c4
constexpr float kPedestrianTiltLimit = 0.34906587f;      // 0x2b9470 (20 degrees)
constexpr float kPedestrianBumpRayEndY = -1.0f;          // 0x2b9478
constexpr float kPedestrianProbeAbove = 0.25f;           // *0x374494, set by the static initialiser 0x347904
constexpr float kPedestrianProbeBelow = 0.25f;           // *0x374498, set by the static initialiser 0x347904
constexpr float kPedestrianWallNearScale = 1.5f;         // 0x205b54
constexpr float kPedestrianBoxScale = 0.5f;              // 0x242eac

// The 100-byte component at entity +0x7c (create 0x1913f8) and the node matrix (rows 0 .. 2, position).
struct PedestrianBody {
    std::uint16_t state = 0;       // +2
    std::uint16_t requested = 0;   // +4
    std::uint32_t hold = 0;        // +0x10, read by the manager's removal
    PedestrianVec3 target{};       // +0x38
    float lateral = 0;             // +0x48
    float pathParameter = 0;       // +0x4c
    float speed = 0;               // +0x54
    float splatTimer = 0;          // +0x58
    bool forward = false;          // +0x5c
    std::uint8_t avoidDelay = 0;   // +0x5d
    std::uint8_t dodgeClip = 0;    // +0x5e: index into the four dodge clips
    std::uint8_t dodgeSide = 0;    // +0x5f
    std::uint8_t bumpClip = 0;     // +0x60
    std::uint8_t voice = 4;        // +0x61: 4 with the parameter IsMale (default 1), else 8
    bool walkStarted = false;      // +0x62
    std::array<PedestrianVec3, 4> node{};
};

// 0x1913f8: state and requested state 0, +0x61 from IsMale.
PedestrianBody pedestrianCreate(bool isMale, const std::array<PedestrianVec3, 4>& node);

// The path object of +0x44: its point count (hword +0xa) and slot 0 of its type's callbacks (table 0x357e24, 20 bytes per type),
// called as (path +8, position out, tangent out, 1, parameter).
struct PedestrianPath {
    std::uint16_t pointCount = 0;
    std::function<void(float parameter, PedestrianVec3& position, PedestrianVec3& tangent)> sample;
};
// The sampler of the path's type: type 0 (0x1c17c0) is the spline position of sampleDirectorPath with the unit tangent of
// detectivePathTangent, type 1 (0x1b33fc) is pedestrianLinearPathSample; any other type throws. The node transform of path +0x18
// is not applied: only the natives EffectAttachParticlesToPath and EffectLinkPathToObject set it, and no script calls them.
PedestrianPath pedestrianLevelPath(const assets::LevelPath& path);
// 0x1b33fc: an open path clamps the parameter to 0 .. count - 1, a closed one wraps both indices (signed remainder 0x24e500
// plus count). d = (point[i + 1] - point[i]) x fraction; tangent = unit(d) ((0, 0, 1) at a fraction of 0), position = d +
// point[i]. The point after the last of an open path is outside the block; at that index the fraction is 0 and d is 0.
void pedestrianLinearPathSample(const assets::LevelPath& path, float parameter, PedestrianVec3& position, PedestrianVec3& tangent);

// The entity box of the create 0x1913f8 (0x242df4 on the node bounds +0xa0 centre / +0xac half extents, which 0x242eb0 =
// assets::modelBounds fills): +0x60 = (min + max) x 0.5, +0x6c = (max - min) x 0.5, +0x5c = the length of +0x6c (0x2536a8),
// the arrival radius of the walk and the reach of the wall test.
struct PedestrianEntityBox {
    PedestrianVec3 centre{};
    PedestrianVec3 half{};
    float radius = 0;
};
PedestrianEntityBox pedestrianEntityBox(const PedestrianVec3& boundsCentre, const PedestrianVec3& boundsHalf);

// 0x2058cc: the ray from (x, y + *0x374494, z) to (x, y - *0x374498, z); a hit sets y to the hit's height. The ray 0x21ab20 with
// its filter (record 0x362298, mask 16) is the caller's. STAND-IN PED-S4: the room set on a hit (0x221064, 0x23e110) is not ported.
void pedestrianGroundProbe(PedestrianVec3& position,
                           const std::function<std::optional<float>(const PedestrianVec3& from, const PedestrianVec3& to)>& ray);

// 0x24ef70: the squared distance (dx dx + dy dy) + dz dz.
float pedestrianDistanceSquared(const PedestrianVec3& a, const PedestrianVec3& b);

// The node draw test 0x2930e0(node, distance squared): true unless the node word's type (bits 0 .. 4) is the mesh node type
// *0x37674c, its bit 9 and bit 4 of +0xde are set, its fade tier (pedestrianFadeTier) is 0 .. 2 and the fade-in distance of that
// tier (0x529e30 +0x3ac + 4 tier) squared is at most the distance squared.
bool pedestrianNodeDrawn(const PedestrianNode& node, std::uint32_t meshNodeType, const DistanceFadeTable& fade, float distanceSquared);

// The inputs of 0x2930e0 in the state step: the pedestrian's node, *0x37674c, the fade table and the point 0x52a1b4
// (0x529e30 +0x384) the distance is taken from.
struct PedestrianDrawTest {
    PedestrianNode node;
    std::uint32_t meshNodeType = 0;
    DistanceFadeTable fade;
    PedestrianVec3 fadePoint{};
};

// The player's vehicle as 0x278b04 reads it: its node's world rows and translation and its entity box half extents x (+0x6c)
// and z (+0x74).
struct PedestrianVehicleBox {
    std::array<PedestrianVec3, 4> node{};
    float halfX = 0;
    float halfZ = 0;
};
// 0x278b04(vehicle, entity): the pedestrian's position in the vehicle's frame (0x2536c0) has |x| < +0x6c and |z| < +0x74.
bool pedestrianUnderVehicle(const PedestrianVehicleBox& vehicle, const PedestrianVec3& position);

// 0x23afc0(entity, clip, mode, first 0, end 0xffff, speed 1.0, blend 0.0); `rootMotion` = 0x239568 on the new slot.
struct PedestrianAnimationRequest {
    PedestrianClip clip = PedestrianClip::Walk;
    int mode = 0;
    std::uint16_t first = 0;
    std::uint16_t end = 0xffff;
    float speed = 1.0f;
    float blend = 0.0f;
    bool rootMotion = false;
};

// STAND-IN PED-S4: the blocked result, the ground probe and the ray, and the clip finished test come from the caller.
struct PedestrianWalkWorld {
    float dt = 0;  // 0x23602c
    const PedestrianPath* path = nullptr;
    float arrivalRadius = 0;  // entity +0x5c: pedestrianEntityBox(...).radius
    PedestrianVec3 playerPosition{};
    bool playerInVehicle = false;     // 0x24255c of the player
    std::uint16_t playerState = 0;    // player component hword +0x6c
    std::vector<PedestrianVec3> otherPedestrians;  // the other live slots of the manager, in slot order (0x2edf08)
    bool* groundProbeTaken = nullptr;  // byte 0x375d5c, cleared by the manager update
    bool visible = false;  // 0x2930e0 of the node, used without `drawTest`
    bool hidden = false;   // 0x23de70, used without `drawTest`
    bool splatRefresh = false;  // 0x278b04(player vehicle, entity) in state 6, used without `playerVehicleBox`
    // 0x1ffcd4 over the list at 0x5863b4 + 0x305c (0x2b9340 .. 0x2b93a4, pedestrianBlocked): the first non-zero result (3 sidestep
    // at the target, 4 turn back), else 0. It can write the body's requested state, dodge side and walk byte and slide the velocity,
    // which the walk then adds to the position (0x2b93c0).
    std::function<int(PedestrianBody& body, const PedestrianVec3& position, PedestrianVec3& velocity, float dt)> blocked;
    // 0x2058cc: the ground probe of the node position (pedestrianGroundProbe with the caller's ray).
    std::function<void(PedestrianVec3& position)> groundProbe;
    // 0x21ab20 from `from` to `to`: the height of the hit.
    std::function<std::optional<float>(const PedestrianVec3& from, const PedestrianVec3& to)> groundRay;
    // 0x245f84(clip) == 6.
    std::function<bool(PedestrianClip clip)> clipFinished;
    // When set, the state step computes `visible` (0x2930e0 at the node position after the update) and `hidden` (0x23de70 =
    // pedestrianNodeHidden) from it.
    std::optional<PedestrianDrawTest> drawTest;
    // The player's vehicle (0x24255c of the player); when set, the state 6 refresh is 0x278b04 on it instead of `splatRefresh`.
    std::optional<PedestrianVehicleBox> playerVehicleBox;
    // The translation of the root delta 0x2327b4 of the current slot (0x233900 of entity +0x40) in the root motion 0x277afc of
    // states 3, 4 and 7; empty when it returns 0. Its yaw is not used: the turned matrix 0x277afc builds is not stored.
    // STAND-IN PED-S8: supplied by the caller.
    std::optional<PedestrianVec3> rootTranslation;
    // 0x21a73c(entity, move, near list, count, 0, 0x80000, 0, 0) of the wall test 0x2059ec: the move after the wall slide. The
    // near list is the caller's: the entities of 0x23d4c0 within `nearRadius` (entity +0x5c x 1.5) of the position, kept when
    // their hword +0x10 has bit 9 or their +0x78 is set and (both +0x5c) x 1.5 is above the distance to their box centre.
    // STAND-IN PED-S8: without it the move is not changed.
    std::function<PedestrianVec3(const PedestrianVec3& position, const PedestrianVec3& move, float nearRadius)> wallSlide;
};

struct PedestrianWalkUpdate {
    std::vector<PedestrianAnimationRequest> animations;
    bool walked = false;
    bool groundProbed = false;
};

class PedestrianWalk {
public:
    // 0x19188c: the steady callback 0x277c24 when state == requested, then the state step 0x1d7e88.
    static PedestrianWalkUpdate update(PedestrianBody& body, const PedestrianWalkWorld& world, GameRandom& random);
    // 0x2b9138, the steady update of state 2.
    static void walk(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out);
    // 0x2edf08: 0 clear, 1 or 2 when the player or another pedestrian is within 2.0 (xz) and within 1.0 to the side.
    static int avoidance(const PedestrianBody& body, const PedestrianWalkWorld& world);
    // 0x277afc, the steady update of states 3, 4 and 7: the move is the root translation through the node rows (the branch of
    // a clip with bit 0 of its word +8, which the create sets on every clip of these states), then the wall test and the probe.
    static void rootMotion(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out);
    // 0x277c94, the steady update of state 5: the contact 0x1fd8c8 with the vector 0x56d400 (zero), the wall test and the probe;
    // on a touch the node moves by the vector once more.
    static void bumped(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out);
    // 0x1fd8c8(entity, position, velocity, wall, probe): false at once when the player has a vehicle and `probe` is clear;
    // otherwise the touch (xz distance squared below 1.5) with the push 0.1 for player states 2 and 3, the wall test 0x2059ec
    // on a touch with `wall`, and the ground probe with `probe`. Returns the touch.
    static bool contact(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianVec3& velocity, bool wall, bool probe,
                        PedestrianWalkUpdate& out);
    // 0x2059ec(entity, move): the node moves by the wall slide of (move x, 0, move z) in x and z; move becomes (slid x,
    // move y + slid y, slid z).
    static void wallTest(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianVec3& move);

private:
    static void enter(PedestrianBody& body, const PedestrianWalkWorld& world, GameRandom& random, PedestrianWalkUpdate& out);
    static void step(PedestrianBody& body, const PedestrianWalkWorld& world);
};

// 0x22859c(matrix, from, to, 0): row 2 = unit(to - from), row 0 from row 2 (|y| <= 0.98: (z, 0, -x), else (0, -z, y)), normalised,
// row 1 = row 2 x row 0; the position stays.
void pedestrianLookAt(std::array<PedestrianVec3, 4>& node, const PedestrianVec3& from, const PedestrianVec3& to);
// First angle of 0x247660 (the matrix to angles of the pair with the smaller length).
float pedestrianTiltAngle(const std::array<PedestrianVec3, 4>& node);

}
