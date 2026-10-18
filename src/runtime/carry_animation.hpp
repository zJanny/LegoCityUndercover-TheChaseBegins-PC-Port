#pragma once
// The animations of carrying (docs/re/carrying-animation.md): which clip the player plays while a carry object is in the hands (the role record variants of 0x219614),
// the clips of the pickup and put-down states, the hand node the object hangs on and where the object sits relative to it. No SDL dependency.
#include "assets/assets.hpp"
#include <array>
#include <string>
#include <vector>

namespace chase::runtime {

// 0x29b170 (state 0xa7, pickup) plays 0x22c for a big object (record +0x9e bit 3 clear) and 0x9b for a small one; 0x283118 (state 0xa8, drop) and the
// target state 0xd6 (enter 0x2768bc) play 0x22e and 0x9d. Names in tables/tablesanimations.bxls sheet 1: supercarry_pickup, itemleft_pickup,
// supercarry_putdown, itemleft_putdown.
constexpr int kAnimationPickupBig = 0x22c;
constexpr int kAnimationPickupSmall = 0x9b;
constexpr int kAnimationPutdownBig = 0x22e;
constexpr int kAnimationPutdownSmall = 0x9d;

// The pickup hook 0x2ce4a4 hangs a small object (record +0x9e bit 3 set) on the joint "weaponLeft" and a big one on "weaponRight" (names at 0x2ce5e4, 0x2ce5e8).
constexpr const char* kCarryHandBig = "weaponright";
constexpr const char* kCarryHandSmall = "weaponleft";
constexpr const char* kCarryHandleNode = "carryhandle";

// The track whose value changes send the player message 39 with kind 0 (0x1d5ec0, also for track 0xc3fcea91); the player's handler hands kind 0 to the
// state hook as event type 0 (0x18f278 -> 0x21e1d4). The pickup hook 0x2ce4a4 attaches at its rise, the drop and put-down hooks release at its first event
// (oracle m11-8-donuts-in-hand and m11-9-delivery-goal, docs/re/carrying-animation.md).
constexpr std::uint32_t kCarryEventTrackHash = 0x98d31b7e;

// The role records of the animation table (tablesanimations sheet 2, 0x219614): one record per role with seven ids [default, v1..v6].
class CarryAnimations {
public:
    explicit CarryAnimations(const assets::Table& animations);
    // The clip 0x219614 picks for the role whose default clip is `id` while carrying: variant 5 for a big object (0x225754 = 2), variant 3 for a small one
    // (1); `id` itself when the role has no such variant (id 0x271 in the record) or `carryingMode` is 0. Class variants (ctrl+0x2cd bits) are not applied.
    int clip(int id, int carryingMode) const;
    // Name of clip `id` (sheet 1), empty when outside the table.
    std::string name(int id) const;
    // Every clip id that clip() returns for any role and any carrying mode.
    std::vector<int> carriedClips() const;

private:
    std::vector<std::array<int, 7>> roles_;
    std::vector<std::string> names_;
};

// Updates of the three states, counted from the update the state is entered (update 0). The attach and release updates come from the event track of
// the clips (pickup 16, put-down 8 for a big object; the drop plays the put-down clip); every state ends when its clip has played once (0x22f560 kind 6;
// oracle at dt 0.033: pickup slot replaced at update 49, place and drop end at update 27 = 13 frames at 15 per second, release at update 8). The defaults
// below are the values of the big-object clips.
struct CarryTiming {
    unsigned pickupAttach = 16;
    unsigned pickupEnd = 48;
    unsigned placeRelease = 8;
    unsigned placeEnd = 26;
    unsigned dropRelease = 8;
    unsigned dropEnd = 26;
};

// The timing a pair of clips gives: pickup attach from the event of `pickup`, place and drop release from the event of `putdown`, the ends from the clip
// lengths in updates. A clip without the event track leaves the default attach or release.
CarryTiming carryTiming(const assets::Animation& pickup, const assets::Animation& putdown);

// Update, counted from the start of the clip, at which `hash` first rises above 0.5 in a clip played once: key time x duration x 30 updates per second.
// The first update of a state is update 0. Negative when the clip has no such track.
int carryEventUpdate(const assets::Animation& animation, std::uint32_t hash);

// Where a carried object sits relative to the hand joint: the rigid inverse of the node "carryhandle" matrix (0x258aac, row vectors, translation last),
// so that the handle sits on the joint. The identity for an object without the node.
std::array<float, 16> carryHandOffset(const assets::Model& model);

}
