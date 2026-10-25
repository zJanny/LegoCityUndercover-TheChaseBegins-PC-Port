#pragma once
// The placed super build bricks: the pickup class of object type 0x84 with pickuptype 103 (the classes pickup_sbbrick2x2 / 2x4, the rainbow
// bricks). Update 0x1acc10, destroy 0x1d737c (the add to the u32 at 0x593430), docs/re/super-bricks.md. No SDL dependency.
//
// A brick in state 1 (idle) spins about +y and tests the distance between its node and the player's node every update; at less than 1.5 it
// becomes state 3 with the timer 1.0. State 3 counts the timer down by the frame time; at 0 or below the destroy adds `SuperBuildValue` to the
// counter, plays a sound of the UI bank and makes the save request 0x21cf38(0).
#include "runtime/studs.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr float kSuperBrickPickupDistance = 1.5f;     // 0x1acde8: the distance (0x2507dc) at or above the literal 0x3fc00000 does not pick up
constexpr float kSuperBrickCollectTime = 1.0f;        // 0x1acdfc: s18 = 1.0 (literal 0x1acf54) stored into +0x10
constexpr float kSuperBrickSpinPerSecond = -3.14159274f;  // 0x1aced0: the clock seconds times the literal 0xc0490fdb at 0x1acf58
constexpr float kSuperBrickClockTicksPerSecond = 3000.0f;  // the clock object 0x5165c8, +0xc
constexpr float kSuperBrickFlightDepth = 6.0f;        // 0x1acfc0: the flight target is 6.0 in front of the camera, on its axis (x 0 / xScale, y 6 x -0.1041667 x (cam +0xd0 x 240 x 0.5) / yScale with cam +0xd0 = 0)
constexpr std::uint16_t kSuperBrickSoundSmall = 2;    // 0x1d7580: the sound index below the value 10 (ui/eq_newbrick_03)
constexpr std::uint16_t kSuperBrickSoundBig = 14;     // 0x1d7588: the sound index from the value 10 (lego_sfx/superbrick_hudhit_01)
constexpr std::uint16_t kSuperBrickBigValue = 10;

// The rainbow tint of the idle bricks (*0x3743f8, written by 0x2600dc once per update): the phase *0x3743f4 (0..1, +0.5 per second, 1.0 is subtracted
// above 1.0) times 14 picks two of the 14 entries of the colour table 0x515c90 (index modulo 14) and the colour is their linear mix
// (0x17e414), packed to bytes by truncation (0x203738). The table bytes (R, G, B) are read from the oracle's memory.
constexpr float kSuperBrickTintPerSecond = 0.5f;
constexpr unsigned kSuperBrickTintEntries = 14;
constexpr std::array<std::array<std::uint8_t, 3>, kSuperBrickTintEntries> kSuperBrickTintTable = {{
    {0x8a, 0x2f, 0xa1}, {0x00, 0x45, 0x67}, {0x00, 0x58, 0xc6}, {0x4e, 0xb4, 0xe3}, {0x00, 0x7e, 0x33}, {0x92, 0xcd, 0x06}, {0xff, 0xd3, 0x0b},
    {0xff, 0x5c, 0x00}, {0xd3, 0x00, 0x00}, {0x96, 0x00, 0x00}, {0x66, 0x33, 0x00}, {0xd3, 0xb9, 0x84}, {0xff, 0xff, 0xff}, {0xa7, 0xa7, 0x9f}}};

class SuperBrickTint {
public:
    // 0x2600dc, called once per update at the end of the pickup update 0x180168 (the hook [0x58ef04 + 0x10]). The phase *0x3743f4 is static data that
    // starts at 0 and no level load resets; the run carries it (`setPhase`).
    void update(float dt);
    void setPhase(float phase) { phase_ = phase; }
    // The bytes of *0x3743f8 (R, G, B) as of the last update.
    const std::array<std::uint8_t, 3>& bytes() const { return bytes_; }
    std::array<float, 3> colour() const { return {float(bytes_[0]) / 255.0f, float(bytes_[1]) / 255.0f, float(bytes_[2]) / 255.0f}; }
    float phase() const { return phase_; }

private:
    float phase_ = 0;
    std::array<std::uint8_t, 3> bytes_ = {255, 255, 255};
};

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
    // `blocked` is bit 4 of the player's entity word +0xc (the pickup test is skipped while it is set). `flightTarget` is the world point 6.0 in
    // front of the camera (0x1acfc0..0x1ad02c); a collected brick moves towards it by 0x1ad04c..0x1ad07c, (target - position) / (kStudFlightRate x timer) like a stud. STAND-IN STUD-S12: the camera shift
    // *0x58ef40 that the original adds to the position before the step (0x1ad028..0x1ad03c) is left out.
    template <typename Enabled>
    SuperBrickStep update(float dt, const std::array<float, 3>& playerPosition, bool blocked, Enabled enabled,
                          const std::optional<std::array<float, 3>>& flightTarget = std::nullopt) {
        SuperBrickStep step;
        for (std::size_t i = 0; i < bricks_.size(); ++i)
            if (enabled(i)) updateOne(i, dt, playerPosition, blocked, flightTarget, step);
        return step;
    }
    std::uint32_t total() const { return total_; }
    // A finished super build-it (0x26d550 state 3) lowers the count by its value (runtime/super_buildit.hpp).
    void spend(std::uint32_t value) { total_ -= value; }
    // The slot load 0x20a500 copies slot +0x5190 into the count.
    void setTotal(std::uint32_t total) { total_ = total; }
    const std::vector<SuperBrick>& bricks() const { return bricks_; }
    std::size_t count(SuperBrickState state) const;

private:
    void updateOne(std::size_t index, float dt, const std::array<float, 3>& playerPosition, bool blocked,
                   const std::optional<std::array<float, 3>>& flightTarget, SuperBrickStep& step);

    std::vector<SuperBrick> bricks_;
    std::uint32_t total_ = 0;  // u32 at 0x593418 + 0x18 = 0x593430
};

}
