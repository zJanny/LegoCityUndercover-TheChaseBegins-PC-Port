#pragma once
// Studs (the collectible coins of a level): pickup test, the one-second collect animation and the stud
// counter, replicating the pickup system 0x180168 (update), 0x2277c4 (collect) and 0x28a128 (counter add).
// Evidence and measurements: docs/re/studs.md. No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

enum class StudKind : std::uint8_t { Silver, Gold, Blue, Purple };

// pickuptype of the class (object type 0x3d): the record byte +0x46 of the pickup pool.
constexpr std::array<std::uint32_t, 4> kStudValues = {10, 100, 1000, 10000};   // switch in 0x28a128
constexpr float kMeasuredStudPickupRadius = 1.5f;       // *0x3772b0, measured: collected at 1.49, not at 1.505
constexpr float kStudCollectSeconds = 1.0f;     // record timer +0x3c set by 0x2277c4 from table +0x60 (0x58efdc)
constexpr float kStudCentreHeight = 0.56f;      // constructor 0x17f8e0 adds it to initialposition y
constexpr float kStudHalfSize = 0.19f;          // quad half size of 0x17fc38, times *0x37728c (1.0)
constexpr float kStudSkipUnit = 0.23f;          // *0x37753c: distance covered per update that the skip counter assumes
constexpr int kStudSkipLimit = 30;   // FUN_0023ca4c (0x23ca4c) = 30; the pool in the oracle resets to 30 (trace skip1 of tools/oracle/hud-skip.txt)
constexpr std::uint64_t kStudCounterLimit = 4200000000ull;  // 0x28a128 clamps the stud counter here
constexpr float kMeasuredPlayerPickupPoint[3] = {0.0f, 1.153f, 0.08f};  // player entity +0x60, in the node's frame
// FUN_0023ca4c (0x23ca4c) returns 30 when the thread frame-rate object is absent; read at the gameplay start
// (0x376af8 +0x8 and +0x10 are zero). The flying stud divides its remaining distance by this times its remaining time.
constexpr float kStudFlightRate = 30.0f;
constexpr float kClockTicksPerSecond = 3000.0f;  // the clock object read by 0x1d2a78 (0x5165c8)

// Colour words of the table at 0x58efc0 (init 0x2ed0b4), the sprite tint: 0xAABBGGRR.
constexpr std::array<std::uint32_t, 4> kStudTint = {0xffeeeeee, 0xff24bcff, 0xffff4240, 0xd0ff42a0};
// Sound ids of tablessoundfx (u16 at +0x8a of the type table): coin1, coin1, coinblue, coinblue.
constexpr std::array<std::uint16_t, 4> kStudCollectSounds = {64, 64, 63, 63};

// Frame of the 4x4 sprite sheet pickups_commonstud: bits 6..9 of ticks * 0.32 (0x17fc38), 15 frames per second.
unsigned studSpriteFrame(std::uint32_t clockTicks);

// The point the pickup distance is measured to: the player's feet, raised and pushed forward by
// kMeasuredPlayerPickupPoint in the player's frame (heading in radians about +y, forward (sin, 0, cos)).
std::array<float, 3> studPickupPoint(float x, float y, float z, float heading);

// 0x24c1d0 as called by the flying stud (0x180168 state 4): the top-screen pixel (400x240) at `depth` in front of the camera,
// moved into the world by the camera pose. `verticalFov` is the projection's vertical field of view in radians; the x scale
// is the y scale divided by the screen aspect 400/240 (0x23458c, camera.md). yaw and pitch follow cameraAxes (scene.hpp).
constexpr float kTopScreenWidth = 400.0f, kTopScreenHeight = 240.0f;
std::array<float, 3> unprojectTopScreen(float screenX, float screenY, float depth, const float eye[3], float yaw, float pitch,
                                        float verticalFov);

struct StudSpawn {
    StudKind kind = StudKind::Silver;
    float position[3] = {0, 0, 0};   // initialposition
    unsigned group = 0;              // DisabledOnStartUp: nonzero hides the stud until a script enables its group
};

enum class StudState : std::uint8_t { Hidden, Idle, Collecting, Done };

struct Stud {
    StudKind kind = StudKind::Silver;
    StudState state = StudState::Idle;
    std::array<float, 3> centre{};
    unsigned group = 0;
    int skip = 0;                    // record +0x30: updates until the distance is tested again
    float timer = 0;                 // record +0x3c while collecting
};

// The counter after a stud's value was added (0x28a128 then the hook 0x29c090): what the HUD stud bar shows.
struct StudBanked {
    StudKind kind = StudKind::Silver;
    std::uint64_t total = 0;
};

struct StudCollected {
    std::size_t index = 0;
    StudKind kind = StudKind::Silver;
    std::uint16_t sound = 0;
};

class StudField {
public:
    explicit StudField(const std::vector<StudSpawn>& spawns);

    // One game update: the idle studs test their distance to `pickupPoint`, collecting studs run their timer
    // and add their value to the counter when it runs out. Returns the studs collected in this update.
    std::vector<StudCollected> update(float dt, const std::array<float, 3>& pickupPoint);

    // World position the collecting studs fly towards (the HUD's counter icon unprojected at depth 5.5 in front of the
    // camera, 0x180168 state 4). Without one they stay where they are.
    void setFlightTarget(const std::optional<std::array<float, 3>>& target) { flightTarget_ = target; }
    const std::optional<std::array<float, 3>>& flightTarget() const { return flightTarget_; }
    std::vector<StudBanked> takeBanked();

    std::uint64_t counter() const { return counter_; }
    // 0x28a128 for a stud of `kind` whose collect timer ran out (also the dropped studs of runtime/stud_drop.hpp): its value added, clamped at
    // kStudCounterLimit, and the new total handed to the HUD hook.
    void bank(StudKind kind);
    // AddStuds 0x18838c adds through 0x1584e0 to the counter object without the clamp of 0x28a128.
    void addToCounter(std::uint64_t amount) { counter_ += amount; }
    // The slot load 0x20a500 copies slot +0x5148 into the counter.
    void setCounter(std::uint64_t counter) { counter_ = counter; }
    const std::vector<Stud>& studs() const { return studs_; }
    std::vector<Stud>& studs() { return studs_; }
    std::size_t count(StudState state) const;

private:
    std::vector<Stud> studs_;
    std::uint64_t counter_ = 0;
    std::optional<std::array<float, 3>> flightTarget_;
    std::vector<StudBanked> banked_;
};

}
