#pragma once
// The placed super build bricks: the pickup class of object type 0x84 with pickuptype 103 (the classes pickup_sbbrick2x2 / 2x4, the rainbow
// bricks). Update 0x1acc10, destroy 0x1d737c (the add to the u32 at 0x593430), docs/re/super-bricks.md. No SDL dependency.
//
// A brick in state 1 (idle) spins about +y and tests the distance between its node and the player's node every update; at less than 1.5 it
// becomes state 3 with the timer 1.0. State 3 counts the timer down by the frame time; at 0 or below the destroy adds `SuperBuildValue` to the
// counter, plays a sound of the UI bank and makes the save request 0x21cf38(0).
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr float kSuperBrickPickupDistance = 1.5f;     // 0x1acde8: the distance (0x2507dc) at or above the literal 0x3fc00000 does not pick up
constexpr float kSuperBrickCollectTime = 1.0f;        // 0x1acdfc: s18 = 1.0 (literal 0x1acf54) stored into +0x10
constexpr float kSuperBrickSpinPerSecond = -3.14159274f;  // 0x1aced0: the clock seconds times the literal 0xc0490fdb at 0x1acf58
constexpr float kSuperBrickClockTicksPerSecond = 3000.0f;  // the clock object 0x5165c8, +0xc
constexpr std::uint16_t kSuperBrickSoundSmall = 2;    // 0x1d7580: the sound index below the value 10 (ui/eq_newbrick_03)
constexpr std::uint16_t kSuperBrickSoundBig = 14;     // 0x1d7588: the sound index from the value 10 (lego_sfx/superbrick_hudhit_01)
constexpr std::uint16_t kSuperBrickBigValue = 10;

enum class SuperBrickState : std::uint8_t { Done = 0, Idle = 1, Collected = 3 };

struct SuperBrick {
    std::string instance;
    std::array<float, 3> position{};
    std::uint16_t value = 0;  // `SuperBuildValue`, the low 16 bits of its integer (record +0x16)
    SuperBrickState state = SuperBrickState::Idle;
    float timer = 0;  // record +0x10
};

struct SuperBrickCollected {
    std::size_t index = 0;
};

struct SuperBrickBanked {
    std::size_t index = 0;
    std::uint16_t value = 0;
    std::uint16_t sound = 0;  // the index handed to 0x235684 with the bank at *0x3757a8
    std::uint32_t total = 0;  // the counter after the add
};

struct SuperBrickStep {
    std::vector<SuperBrickCollected> collected;
    std::vector<SuperBrickBanked> banked;
};

// The spin matrix of 0x1aced0: 0x237c48 with the angles (0, -pi * seconds, 0) (row 0 (cos, 0, -sin), row 1 (0, 1, 0), row 2 (sin, 0, cos)), the node's
// translation kept. `clockTicks` is the clock object's value (3000 per second).
std::array<float, 16> superBrickMatrix(const std::array<float, 3>& position, std::uint32_t clockTicks);

// Alpha of a collected brick: 0x23f124(node, int(timer * 255)) while the timer runs (0x1ad094).
float superBrickAlpha(float timer);

class SuperBrickField {
public:
    void add(const std::string& instance, const std::array<float, 3>& position, std::uint16_t value);
    // One update. `enabled(i)` is false for a disabled entity (its update does not run). `playerPosition` is the translation of the player's node;
    // `blocked` is bit 4 of the player's entity word +0xc (the pickup test is skipped while it is set).
    template <typename Enabled>
    SuperBrickStep update(float dt, const std::array<float, 3>& playerPosition, bool blocked, Enabled enabled) {
        SuperBrickStep step;
        for (std::size_t i = 0; i < bricks_.size(); ++i)
            if (enabled(i)) updateOne(i, dt, playerPosition, blocked, step);
        return step;
    }
    std::uint32_t total() const { return total_; }
    const std::vector<SuperBrick>& bricks() const { return bricks_; }
    std::size_t count(SuperBrickState state) const;

private:
    void updateOne(std::size_t index, float dt, const std::array<float, 3>& playerPosition, bool blocked, SuperBrickStep& step);

    std::vector<SuperBrick> bricks_;
    std::uint32_t total_ = 0;  // u32 at 0x593418 + 0x18 = 0x593430
};

}
