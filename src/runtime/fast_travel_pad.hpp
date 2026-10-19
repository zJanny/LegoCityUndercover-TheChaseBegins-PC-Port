#pragma once
// The fast travel pad (class props/prop_fasttravelzone, kind 227) and the player's fast travel states 0x180 "Fast Travel Enter" and 0x181 "Fast
// Travel Active": A in the pad's use zone lands the holder's Helicopter child and moves the player onto the pad. Rules and evidence:
// docs/re/fast-travel.md. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/animation_slots.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kFastTravelPadClass = "props/prop_fasttravelzone";
constexpr std::uint8_t kFastTravelPadKind = 227;
constexpr const char* kFastTravelHelicopterChild = "Helicopter";  // post-link 0x1bac80: 0x2450f0(holder, "Helicopter")

constexpr std::uint16_t kPlayerStateFastTravelEnter = 0x180;
constexpr std::uint16_t kPlayerStateFastTravelActive = 0x181;

// 0x32c2ec asks 0x21941c for animation 582 (tablesanimations sheet 1, "typing_stood"), blend 0.1, speed 1.0.
constexpr int kAnimationTypingStood = 582;

// 0x2cd91c: the approach factor ctrl+0x24c grows by dt x 1.5 and the facing turns by at most 0x800 per update.
constexpr float kFastTravelApproachRate = 1.5f;
constexpr int kFastTravelTurnStep = 0x800;
constexpr float kFastTravelHeadingScale = 10430.378f;

enum class FastTravelClip { None, Land, Idle, Takeoff };

struct FastTravelPad {
    std::string instance;    // lower case; "holder.child" for one nested in a class file
    std::string holder;      // lower case holder instance (0x239678), empty for a placed pad
    std::string helicopter;  // lower case "holder.helicopter", empty without one
    std::string helicopterMesh;
    assets::Matrix world{};            // node world matrix
    assets::Matrix helicopterWorld{};  // the Helicopter's node world matrix
    float zoneCentre[3] = {0, 0, 0};   // class bound UseBound, node frame
    float zoneHalf[3] = {0, 0, 0};
    bool hasZone = false;
    bool busy = false;  // class data +0x64 (update 0x1c8244): the Helicopter plays "takeoff"
};

// The Helicopter's animation player: the slot list of its entity and the three clips post-link 0x1bac80 loads (0x23e6e4: "land", "idle",
// "takeoff" of models/<model>/).
struct FastTravelHelicopter {
    std::array<AnimationClipInfo, 3> clips{};  // land, idle, takeoff
    AnimationSlots slots;
    bool shown = false;  // post-link hides it (0x2428c0), the enter 0x32c2ec shows it (0x242cf0)
};

struct FastTravelInput {
    float dt = 0;
    float player[3] = {0, 0, 0};
    std::int16_t facing = 0;  // ctrl+8
    // STAND-IN LEVOBJ-S96: the gate of the use registry walk (0x1fee94) is the port's "player free" flag.
    bool playerFree = false;
    bool actionPressed = false;  // the action flag (byte +5) of the use query (LEVOBJ-S41: read as A)
};

// What the player's state does to the player in this update.
struct FastTravelPlayer {
    bool moved = false;  // the approach set the position
    float position[3] = {0, 0, 0};
    bool turned = false;
    std::int16_t facing = 0;
    bool resetRunTimer = false;    // the enter sets ctrl+0x24c (the run timer) to 0
    std::optional<std::int16_t> lastIntent;  // ctrl+0xa, the pad's heading
};

enum class FastTravelEventKind {
    Entered,         // state 0x180 entered: player clip typing_stood, Helicopter shown and "land" started
    Active,          // state 0x181 entered
    HelicopterIdle,  // the landing ended: "idle" loops, 0x28b930 answers 1 (the map buttons may open the fast travel map)
    HelicopterHidden,
};

struct FastTravelEvent {
    FastTravelEventKind kind = FastTravelEventKind::Entered;
    std::size_t pad = 0;
};

// The Helicopter's skeleton and the clips "land", "idle" and "takeoff" of its model folder (models/<mesh folder>/<name>.bfnanm).
struct FastTravelHelicopterModel {
    std::string modelPath;
    assets::Skeleton skeleton;
    std::array<assets::Animation, 3> animations;
    std::array<AnimationClipInfo, 3> clips{};
};
std::optional<FastTravelHelicopterModel> loadFastTravelHelicopter(assets::FuseArchive& archive, const std::string& mesh);
// Skin matrices of the Helicopter at the frame of its current slot (the last frame once a play-once clip ended); empty without a slot.
std::vector<assets::Matrix> fastTravelHelicopterPose(const FastTravelHelicopterModel& model, const FastTravelHelicopter& helicopter, std::int64_t clock);

class FastTravelPads {
public:
    // Every props/prop_fasttravelzone placed in the level or nested in the class file of a placed object.
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    void add(FastTravelPad pad, FastTravelHelicopter helicopter = {});
    // Clip lengths and rates of the Helicopter of pad `index` (land, idle, takeoff).
    void setHelicopterClips(std::size_t index, const std::array<AnimationClipInfo, 3>& clips);

    // One update: the pad updates 0x1c8244, the state the player requested in the previous update (0x2414b4 stores it, the next state step enters
    // it), the current state's update, then the use query (message 3, 0x1d79a8) of the pad whose zone holds the player.
    std::vector<FastTravelEvent> update(const assets::PlacedObjectState& state, const FastTravelInput& input, FastTravelPlayer& player);

    std::uint16_t playerState() const { return state_; }
    bool takesPlayerControl() const { return state_ != 0; }
    int playerClip() const { return state_ != 0 ? kAnimationTypingStood : 0; }
    // 0x28b930: 0 in state 0x180, the Active flag (+0x14 of the state object 0x5157e4) in 0x181, 1 otherwise.
    bool mapReady() const;

    const std::vector<FastTravelPad>& pads() const { return pads_; }
    const FastTravelHelicopter& helicopter(std::size_t index) const { return helicopters_[index]; }
    FastTravelClip currentClip(std::size_t index) const;
    std::int64_t clock() const { return clock_; }
    std::optional<std::size_t> activePad() const { return pad_; }

    static bool inZone(const FastTravelPad& pad, const float player[3]);
    // atan2 of the forward row (x, z) x 10430.378, truncated (0x24765c, 0x2473ec).
    static std::int16_t heading(const assets::Matrix& node);

private:
    void enter(std::uint16_t next, FastTravelPlayer& player, std::vector<FastTravelEvent>& events);
    void play(std::size_t index, FastTravelClip clip, unsigned mode);
    bool current(std::size_t index, FastTravelClip clip, bool finished) const;

    std::vector<FastTravelPad> pads_;
    std::vector<FastTravelHelicopter> helicopters_;
    std::int64_t clock_ = 0;
    std::uint16_t state_ = 0;
    std::uint16_t requested_ = 0;
    std::optional<std::size_t> pad_;  // 0x374210
    bool approaching_ = false;        // 0x374328
    float approach_ = 0;              // ctrl+0x24c
    std::int16_t target_ = 0;         // ctrl+0xa
    bool idleStarted_ = false;        // Active +0x14
};

}
