#pragma once
// The animations of carrying (docs/re/carrying.md): which clip the player plays while a carry object is in the hands (the role record variants of 0x219614),
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

// STAND-IN LEVOBJ-S72: the track hash whose first rise is the event the pickup hook (0x2ce4a4, event type 0, value above 0.5) and the target put-down hook
// react to. Inferred from the clips (its key falls on update 16 of supercarry_pickup and on update 8 of supercarry_putdown, the measured release); the
// table that maps track hashes to event types was not read.
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

// Updates of the three states, counted from the update the state is entered (update 0). The place values are measured in the oracle (state 0xd6
// entered at update 0; release 8 updates later, back to state 1 at update 17); the drop release is 4 updates (state 0xa8). The attach and release
// updates the game takes from the clip events (carry_animation.hpp: pickup 16, place 8 for a big object); the defaults below are the values of the
// pickup before the clips were read. The pickup ends with its clip (the animation slot finishes: 0x22f560, docs/re/carrying.md; oracle m11-7-box-prompt:
// 24 frames at 15 per second, the slot is replaced at update 49 of dt 0.033); carryTiming sets it from the clip. STAND-IN LEVOBJ-S30: the ends of the place
// and drop states copy the measured place end (17; the clip lasts 26 updates), and the drop release is the one measurement.
struct CarryTiming {
    unsigned pickupAttach = 8;
    unsigned pickupEnd = 17;
    unsigned placeRelease = 8;
    unsigned placeEnd = 17;
    unsigned dropRelease = 4;
    unsigned dropEnd = 17;
};

// The timing a pair of clips gives: pickup attach from the event of `pickup`, place release from the event of `putdown`; the rest keeps the defaults
// (pickup end: the clip length in updates). A clip without the event track leaves the default attach.
CarryTiming carryTiming(const assets::Animation& pickup, const assets::Animation& putdown);

// Update, counted from the start of the clip, at which `hash` first rises above 0.5 in a clip played once: key time x duration x 30 updates per second.
// The first update of a state is update 0. Negative when the clip has no such track.
int carryEventUpdate(const assets::Animation& animation, std::uint32_t hash);

// Where a carried object sits relative to the hand joint: the rigid inverse of the node "carryhandle" matrix (0x258aac, row vectors, translation last),
// so that the handle sits on the joint. The identity for an object without the node.
std::array<float, 16> carryHandOffset(const assets::Model& model);

}
