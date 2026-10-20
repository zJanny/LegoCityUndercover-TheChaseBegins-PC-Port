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
// Type 0 (0x1c17c0): the spline position of sampleDirectorPath and the unit tangent of detectivePathTangent.
// STAND-IN PED-S3: the transform by the path's node (0x254250 when path +0x18 is set) is not applied, and type 1 (0x1b33fc) is
// sampled as type 0.
PedestrianPath pedestrianLevelPath(const assets::LevelPath& path);

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

struct PedestrianWalkWorld {
    float dt = 0;  // 0x23602c
    const PedestrianPath* path = nullptr;
    float arrivalRadius = 0;  // entity +0x5c = length of entity +0x6c (0x242df4 at the create)
    PedestrianVec3 playerPosition{};
    bool playerInVehicle = false;     // 0x24255c of the player
    std::uint16_t playerState = 0;    // player component hword +0x6c
    std::vector<PedestrianVec3> otherPedestrians;  // the other live slots of the manager, in slot order (0x2edf08)
    bool* groundProbeTaken = nullptr;  // byte 0x375d5c, cleared by the manager update
    bool visible = false;  // 0x2930e0 of the node
    bool hidden = false;   // 0x23de70
    bool splatRefresh = false;  // 0x278b04(player vehicle, entity) in state 6
    // 0x1ffcd4 over the list at 0x5863b4 + 0x305c: the first non-zero result (1, 2 sidestep, 3 sidestep at the target, 4 turn back).
    std::function<int(const PedestrianVec3& position, const PedestrianVec3& velocity, float dt)> blocked;
    // 0x2058cc: the ground probe of the node position.
    std::function<void(PedestrianVec3& position)> groundProbe;
    // 0x21ab20 from `from` to `to`: the height of the hit.
    std::function<std::optional<float>(const PedestrianVec3& from, const PedestrianVec3& to)> groundRay;
    // 0x245f84(clip) == 6.
    std::function<bool(PedestrianClip clip)> clipFinished;
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
