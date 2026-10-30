#pragma once
// The fence vault (kind 189, props/parkour_fencevault) and the player state 326 "FENCE VAULT", docs/re/player-traversal-fence-vault.md: the B press of a
// player in a ground locomotion state asks the zone list 0x58ea1c (0x20b984 -> 0x1fee94); a fence whose UseBound holds the player takes the press
// (handler 0x19de38), sends him through USE LERP (state 20) to a point in front of the fence line and into state 326 (enter 0x2769dc, update 0x29aadc),
// whose clip rise is scaled to the fence's origin; state 5 follows. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/lever_use.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kFenceVaultObjectKind = 189;
constexpr unsigned kPlayerStateFenceVault = 326;
constexpr unsigned kPlayerStateFenceVaultLerp = kPlayerStateUseLerp;  // pushed by 0x2366bc(player, fence, 326, 0)
constexpr int kAnimationVault = 286;                 // tablesanimations sheet 1 "vault": VaultType 1 (0x2769dc)
constexpr int kAnimationVaultFence = 287;            // "vault_fence": every other VaultType
constexpr float kFenceVaultOffset = 1.55f;           // 0x19dfdc: distance of the use point from the fence line
constexpr float kFenceVaultOffsetType1 = 1.0f;       // 0x19dfd8: the same for VaultType 1
constexpr float kFenceVaultLerpSeconds = kUseLerpSeconds;
constexpr float kFenceVaultLerpSecondsFacing = 0.1f; // 0x19dfec: ctrl+0x290 when the player faces the fence

struct FenceVaultObject {
    std::string instance;                 // lower case
    std::array<float, 16> matrix{};       // node rows right, up, forward, translation (assets::placementMatrix)
    int vaultType = 0;                    // VaultType (record +0x18)
    int faceTowards = 0;                  // FaceTowards, low 3 bits of the zone byte (0x21d77c)
    bool hasUseBound = false;
    float useCentre[3] = {0, 0, 0}, useHalf[3] = {0, 0, 0};  // class bound UseBound
    const float* position() const { return &matrix[12]; }
    const float* right() const { return &matrix[0]; }
    const float* up() const { return &matrix[4]; }
    const float* forward() const { return &matrix[8]; }
};

std::vector<FenceVaultObject> loadFenceVaults(const assets::Level& level, const assets::ClassLookup& classOf);

// 0x221570 for a fence's zone: the FaceTowards test (modes 1..5) and the player's position in the fence's frame inside the UseBound. A fence without
// UseBound has no zone here (0x21d77c would build one from the model bounds; every shipped fence class has the bound).
bool fenceVaultZoneHolds(const FenceVaultObject& fence, const float player[3], std::int16_t facing);

// The B press gate of 0x20b984 (0x1fee94 with pressed 0): the first fence, in level order, whose zone holds and that is not disabled; -1 when none. The
// handler answers 1 to every message 3, so that fence takes the press even when it does not vault (a player who is no character or carries a big object).
int fenceVaultOffer(const std::vector<FenceVaultObject>& fences, const float player[3], std::int16_t facing,
                    const std::function<bool(const FenceVaultObject&)>& disabled);

// The use point the handler 0x19de38 writes over that of 0x207be4: the player's foot on the fence line, moved by the offset to the player's side, the
// heading across the fence (ctrl+0x280, ctrl+0x296) and the lerp time (ctrl+0x290).
struct FenceVaultApproach {
    std::array<float, 3> position{};
    std::int16_t heading = 0;
    float seconds = kFenceVaultLerpSeconds;
};
FenceVaultApproach fenceVaultApproach(const FenceVaultObject& fence, const float player[3], std::int16_t facing);

// The clip of state 326 (0x2769dc): kAnimationVault for VaultType 1, else kAnimationVaultFence.
int fenceVaultClip(const FenceVaultObject& fence);

// 0x2769dc: the root-motion scale in y, (fence y - player y) / (max y - min y) of the clip's root track (0x2147e4); 0 for a track without height.
float fenceVaultRiseScale(const FenceVaultObject& fence, float playerY, const assets::RootMotion& motion);

struct FenceVaultClipData {
    const assets::RootMotion* motion = nullptr;
    float framesPerSecond = 15.0f;
    float lastFrame = 0.0f;  // length - 1: 0x29aadc ends when the clip frame reaches the frame count of 0x23caa4 minus 1
};

struct FenceVaultStep {
    std::array<float, 3> position{};
    std::int16_t facing = 0;
    int animation = 0;                  // 0 in USE LERP (its own clip is not ported), the vault clip in state 326
    std::optional<unsigned> entered;    // 20 in the update of the press, 326 in the update the vault starts
    bool ended = false;                 // state 5 starts: the player falls with `fallSpeed`
    float fallSpeed = 0.0f;             // ctrl+0x2b8 = the height lost in the last update
};

// USE LERP and state 326 as one sequence. start() runs in the update of the B press and takes the first lerp step (0x27fecc): the facing turns towards
// the heading by at most 3200, the position moves by dt / seconds of the way; once the lerp is at 1 the next update sets the heading and the update after
// it starts state 326 (the order of the lever's USE LERP, docs/re/switches.md). The k-th update of state 326 (k = 0 at its start) moves to clip frame
// k dt x rate of the root track turned by the heading, its height scaled; the update that reaches the clip's last frame but one ends it, and state 5
// takes its first fall step of the last update's height loss in that update (RPC 20261030-qolstores-F-1). TRAV-S38: the moves have no collision
// (0x215804, 0x1fa7e0), the clip frame is the sum of dt x rate per update, the end's fall step stands for the ground probe 0x20b48c (not read).
class FenceVaulter {
public:
    FenceVaultStep start(const std::vector<FenceVaultObject>& fences, std::size_t fence, const float position[3], std::int16_t facing, float dt);
    FenceVaultStep update(const std::vector<FenceVaultObject>& fences, const FenceVaultClipData& clip, float dt);
    bool active() const { return state_ != 0 || pending_ != 0; }
    std::size_t fence() const { return fence_; }
    unsigned state() const { return state_; }

private:
    FenceVaultStep step() const;
    void stepLerp(float dt);
    FenceVaultApproach approach_;
    std::size_t fence_ = 0;
    unsigned state_ = 0;
    unsigned pending_ = 0;
    float lerp_ = 0.0f;
    float frame_ = 0.0f;
    float rise_ = 1.0f;
    std::array<float, 3> start_{};
    std::array<float, 3> position_{};
    std::int16_t facing_ = 0;
    int animation_ = 0;
};

}
