#pragma once
// Chase's attack on foot (docs/re/player-attack.md): the Y tap of 0x1bb690, the target search 0x219c08 with its score 0x21a1d8, the combat action
// states 0xa3 / 0xa4 (GOCSCombat_CombatActionState / GOCSCombat_CombatActionRepeatState: enter 0x307e7c, update 0x3085dc, exit 0x3084f4, animation
// events 0x319ffc and 0x3204dc), the hit test 0x1f7f28 and the messages the attack sends to its target through 0x244fc8. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

// 0x1bb690: a Y release after fewer held updates than this sets the tap edge ctrl+0xe bit 0x20; a longer hold sets nothing.
constexpr std::uint8_t kAttackTapUpdates = 10;
constexpr int kAttackStateAction = 0xa3;
constexpr int kAttackStateRepeat = 0xa4;
// 0x18d15c: variant 0 plays 0x7a + the combo index (combo_1, combo_2, combo_3 and row 125 "combo_1"); variant 3 (the running attack) plays 0x7e
// (0x7d against characters of type 0x43 / 0xcc, which are no candidates here).
constexpr int kAttackFirstComboClip = 0x7a;
constexpr int kAttackRunningClip = 0x7e;
constexpr std::uint32_t kAttackMessageHit = 0;        // 0x319ffc, record {0, player, 0, damage, 0, kind, 0}
constexpr std::uint32_t kAttackMessageNotice = 0x3e;  // 0x307e7c in the enter when the target's node is within the range, record {0, player, 0, ...}
constexpr std::int32_t kAttackDamage = 1;             // 0x319ffc +0xc (99 while byte +0x12e of the combat record has bit 3, not modelled)
constexpr std::uint32_t kAttackHitKind = 1;           // 0x319ffc +0x14: 1 for the variants 0 and 3
constexpr float kRunningAttackRangeScale = 1.5f;      // 0x2108c0
constexpr float kAttackSearchMaxRange = 5.0f;         // *0x372e5c: a longer range searches every room (not modelled; the player's range is 2.2)

// STAND-IN PLAYER-S5: values read from the heap in the oracle (m12-table-before, docs/re/player-attack.md); their writers are not read.
constexpr float kPlayerMeleeRangeStandIn = 2.2f;      // ctrl+0x80 of the player (0x21a1d0)
constexpr float kPlayerAttackAngleStandIn = 90.0f;    // *0x372e44, degrees, the player's search and hit angle
constexpr unsigned kPlayerComboLengthStandIn = 4;     // high nibble of the combat record byte +0xf

// STAND-IN PLAYER-S6: the clip tracks behind the event types of 0x319ffc are inferred from the measured updates: the hit comes in the update in
// which the clip passes the key of value 1 of track 0xb684fd50 (combo_1 frame 5.0 -> update 5, combo_2 4.0018 -> 5, combo_3 3.0055 -> 4); the
// table from track hashes to event types is not read.
constexpr std::uint32_t kAttackHitTrackStandIn = 0xb684fd50;     // event type 3 (value above 0.5): the hit
constexpr std::uint32_t kAttackComboTrackStandIn = 0xc99766ba;   // event type 7 (value above 0.5): the combo window bit 3 of record +0x10
// STAND-IN PLAYER-S7: the root step of 0x2c9770 for the combo clips comes from the sub-animation track 0xdcd633e2 (z of dcd633e0..e2), inferred
// from the measured step 0.07548 per update (combo_1 ends at 1.05684 after 14 updates); the joint curves of these clips (header 0xfa) are not decoded.
constexpr std::uint32_t kAttackRootTrackStandIn = 0xdcd633e2;

// 0x1bb690 for bit 0x20 of ctrl+0xc: true in the update in which a short Y press is released.
class AttackTap {
public:
    bool update(bool held);

private:
    std::uint8_t heldUpdates_ = 0;
};

// A clip of the attack: the clip frame reached at the end of update k is k * framesPerUpdate (the entering update is k = 1).
struct AttackClip {
    int id = 0;
    unsigned updates = 0;            // animationLength at its rate in 30 Hz updates; the finish is seen in the update after it
    float framesPerUpdate = 1.0f;
    std::vector<float> hitFrames;    // frames of the keys above 0.5 of the hit track
    std::vector<float> windowFrames; // frames of the keys above 0.5 of the combo window track
    std::vector<float> root;         // root track value at the end of update k, k = 0 .. updates
    bool passes(const std::vector<float>& frames, unsigned k) const;
};
AttackClip makeAttackClip(int id, const assets::Animation& animation);

// An entity as the attack code sees it: node matrix (0x250810), box in node space (entity+0x60 / +0x6c) and radius (entity+0x5c, the length
// of the half extents).
struct AttackBody {
    assets::Matrix node{};
    std::array<float, 3> centre{};
    std::array<float, 3> half{};
    float radius = 0;
};
AttackBody attackBody(const assets::Matrix& node, const std::array<float, 3>& centre, const std::array<float, 3>& half);
// The player's body: node from position and facing, the box of PlayerBox (entity+0x60 = (0, 1.15, 0.08), +0x6c = (0.6, 0.85, 0.52) read in the
// oracle, radius 1.16314).
AttackBody playerAttackBody(float x, float y, float z, std::int16_t facing);

struct AttackCandidate {
    AttackBody body;
    bool collision = true;           // entity+0x10 bit 0x200 (0x21a2d4 drops an entity without it)
};

// STAND-IN PLAYER-S9: the level objects the attack can find are the props of class leprops/leprop_wobblydestructible (handler type 119, which
// 0x21a2d4 accepts), with the box of their collision entry (also for a mesh entry); the other types 0x21a2d4 accepts (100, 155, 85, 214, 19) and
// characters are not candidates.
struct AttackTargetObject {
    std::string instance;
    bool collisionDefault = true;
    AttackBody body;
};
std::vector<AttackTargetObject> attackTargetObjects(const std::vector<assets::ObjectCollision>& placed);

// 0x21a1d8: len * (1.25 - cos) * 2 * weight for a candidate within the range and the angle, else FLT_MAX.
float attackScore(float range, float maxAngle, float weight, const std::array<float, 3>& from, const std::array<float, 3>& direction,
                  const std::array<float, 3>& to);
// 0x219c08 for the player (weight 0x1b7358 = 1.0 for props): the candidate with the lowest score, or none.
std::optional<std::size_t> findAttackTarget(float range, float maxAngle, const AttackBody& attacker, const std::array<float, 3>& direction,
                                            const std::vector<AttackCandidate>& candidates);
// 0x1f7f28: the boxes overlap vertically by more than 0.2, the distance of the box centres minus the target's radius is below the range and the
// horizontal direction to the target's node is within [minAngle, maxAngle] of the attacker's forward.
bool attackHitTest(float range, float maxAngle, float minAngle, const AttackBody& attacker, const AttackBody& target);
// 0x205fbc: the heading from the attacker's node to the target's box centre (node position without the collision flag), or none within the
// attacker's half width entity+0x6c.
std::optional<std::int16_t> attackTargetHeading(const AttackBody& attacker, const AttackCandidate& target);

struct AttackMessage {
    std::size_t target = 0;          // index into the candidates
    std::uint32_t id = 0;            // kAttackMessageHit or kAttackMessageNotice; the sender is the player
    std::int32_t damage = 0;         // record +0xc
    std::uint32_t kind = 0;          // record +0x14
};

struct AttackInput {
    bool tap = false;                // AttackTap edge of this update
    bool allowed = false;            // the player is in a ground locomotion state that runs 0x20b984 -> 0x1fdbd4
    bool running = false;            // ctrl+0x6c is 2 (run) and the stick moved the player (0x1fdbd4 param 4): the running attack
    StickIntent intent;
    float x = 0, y = 0, z = 0;       // player node position before the update
    const std::vector<AttackCandidate>* candidates = nullptr;
};

struct AttackStep {
    bool active = false;             // the player is in state 0xa3 / 0xa4 in this update
    bool started = false;            // a new attack state began in this update
    bool finished = false;           // the attack returned to locomotion in this update (the caller runs locomotion)
    bool runAfter = false;           // 0x3204dc requests the run state 2 instead of the idle state 1
    int state = 0;
    int clip = 0;
    float deltaX = 0, deltaZ = 0;    // root step of this update along the target heading
    std::vector<AttackMessage> messages;
};

// The clips of the attack, indexed by clip id.
struct AttackClips {
    std::vector<AttackClip> clips;
    const AttackClip* find(int id) const;
};

class PlayerAttack {
public:
    bool active() const { return state_ != 0; }
    int state() const { return state_; }
    std::optional<std::size_t> target() const { return target_; }
    // One update: from locomotion a tap enters 0xa3; in the attack the facing (motion.facing) turns toward the target heading (motion.lastIntent),
    // the root step follows it, the clip events send the hit and a tap in the combo window starts the next state.
    AttackStep update(const AttackInput& input, LocomotionState& motion, const AttackClips& clips);

private:
    void enter(const AttackInput& input, LocomotionState& motion, const AttackClips& clips, bool running, AttackStep& step);
    void advance(const AttackInput& input, LocomotionState& motion, AttackStep& step);

    int state_ = 0;
    int variant_ = 0;
    unsigned comboIndex_ = 0;
    unsigned elapsed_ = 0;           // updates of the clip so far (1 in the entering update)
    bool window_ = false;            // bit 3 of combat record +0x10
    bool pendingTap_ = false;        // a tap edge of the previous update (the attack states read it one update later)
    const AttackClip* clip_ = nullptr;
    std::optional<std::size_t> target_;
};

}
