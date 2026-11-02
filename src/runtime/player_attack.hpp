#pragma once
// Chase's attack on foot (docs/re/player-attack.md): the Y tap of 0x1bb690, the target search 0x219c08 with its score 0x21a1d8, the combat action
// states 0xa3 / 0xa4 (GOCSCombat_CombatActionState / GOCSCombat_CombatActionRepeatState: enter 0x307e7c, update 0x3085dc, exit 0x3084f4, animation
// events 0x319ffc and 0x3204dc), the hit test 0x1f7f28 and the messages the attack sends to its target through 0x244fc8. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/attack_targets.hpp"
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

// STAND-IN PLAYER-S5: values read from the heap in the oracle (the save state in front of the table of mission 1-2, docs/re/player-attack.md); their writers are not read.
constexpr float kPlayerMeleeRangeStandIn = 2.2f;      // ctrl+0x80 of the player (0x21a1d0)
constexpr float kPlayerAttackAngleStandIn = 90.0f;    // *0x372e44, degrees, the player's search and hit angle
constexpr unsigned kPlayerComboLengthStandIn = 4;     // high nibble of the combat record byte +0xf

// STAND-IN PLAYER-S6: the clip tracks behind the event types of 0x319ffc are inferred from the measured updates: the hit comes in the update in
// which the clip passes the key of value 1 of track 0xb684fd50 (combo_1 frame 5.0 -> update 5, combo_2 4.0018 -> 5, combo_3 3.0055 -> 4); the
// table from track hashes to event types is not read.
constexpr std::uint32_t kAttackHitTrackStandIn = 0xb684fd50;     // event type 3 (value above 0.5): the hit
constexpr std::uint32_t kAttackComboTrackStandIn = 0xc99766ba;   // event type 7 (value above 0.5): the combo window bit 3 of record +0x10
// The root step of 0x2c9770 comes from the extra joint curve of the clip (0x2327b4 -> 0x270778: joint table bit 28, curve index word & 0x1fff; joint 21 of the
// combo clips, z rising 0.07549 per frame). STAND-IN PLAYER-S10: the value between two frames is interpolated linearly (the combo clips run one frame per update).

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
    AttackEntity entity;             // kind, flags and controller as 0x21a2d4 and 0x1b7358 read them (a wobbly prop by default)
    std::string name;                // lower-case instance name
};

// The arguments of 0x219c08 that differ between its callers in the attack.
struct AttackSearch {
    float range = 0;
    float angle = 0;
    unsigned flags = kAttackSearchFlags;   // 0x21a2d4 flags
    bool allAround = true;                 // fourth argument: a bad guy is searched all around (every call of the player's attack sets it)
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
// 0x219c08 over the near list (a range up to 5.0): among the candidates that pass 0x21a2d4, the one with the lowest score of 0x21a1d8 with the
// weight 0x1b7358, the candidate's half width and depth beyond the attacker's added to the range and the angle of attackSearchAngle.
// STAND-IN PLAYER-S13: the near list 0x23d4c0 (the player's list 0x593284, maintained elsewhere) is taken to hold every candidate.
std::optional<std::size_t> findAttackTarget(const AttackSearch& search, const AttackEntity& attackerEntity, const AttackBody& attacker,
                                            const std::array<float, 3>& direction, const std::vector<AttackCandidate>& candidates);
// The same for the player with the flags of the Y tap.
std::optional<std::size_t> findAttackTarget(float range, float maxAngle, const AttackBody& attacker, const std::array<float, 3>& direction,
                                            const std::vector<AttackCandidate>& candidates);

// The player's entity: a character (kind 27) with collision, not a bad guy, on the ground.
AttackEntity playerAttackEntity();

// What 0x1fdbd4 does with a Y tap of the player.
enum class PlayerAttackMove {
    Punch,       // 0x214f40 variant 0 (3 running), with the melee target or none
    FightMove,   // 0x1d3980: 0x214f40 variant 4 against the grab target, which is asked for state 0xe3
    Struggle,    // 0x19bfc0: states 0xfe (player) and 0xff (target), for a target with ability 0x32 or 0x35 or a boss
};
struct PlayerAttackChoice {
    PlayerAttackMove move = PlayerAttackMove::Punch;
    std::optional<std::size_t> target;
    bool running = false;
};
// The facts of the original's tables the choice needs.
struct PlayerAttackTables {
    std::uint8_t weaponRow = 13;     // character table byte +0x17 of the player (row 9 police: 13)
    AbilitySet fightMoveAbilities{}; // the ability bytes +0xc of the eight fight moves of *0x372e28
    std::vector<FightMove> moves;    // the eight fight moves
};
// 0x1fdbd4 for a Y tap of the player in ground locomotion: the grab search (3.0, 45 degrees) keeps a character that passes 0x28265c and 0x23d610;
// otherwise the melee search (ctrl+0x80, 1.5 x running) drops a character unless the weapon row is 16.
// STAND-IN PLAYER-S8: the line of sight 0x219990 of 0x28265c is taken as clear. The special target *(0x3746f4 + 8) (0x219b80) is not modelled.
PlayerAttackChoice choosePlayerAttack(const AttackBody& attacker, const AttackEntity& player, const std::array<float, 3>& direction, bool running,
                                      const std::vector<AttackCandidate>& candidates, const PlayerAttackTables& tables);
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

// 0x1d3980 and 0x19bfc0: the fight move or grab struggle that the tap started against a character.
struct FightStart {
    std::size_t target = 0;
    PlayerAttackMove move = PlayerAttackMove::FightMove;
    int index = -1;                  // the fight move (combat record byte +0xb), -1 for a struggle
};
constexpr int kFightMoveVariant = 4;  // 0x1d3980: 0x214f40(player, ctrl, 4, 0)

struct AttackInput {
    bool tap = false;                // AttackTap edge of this update
    bool allowed = false;            // the player is in a ground locomotion state that runs 0x20b984 -> 0x1fdbd4
    bool running = false;            // ctrl+0x6c is 2 (run) and the stick moved the player (0x1fdbd4 param 4): the running attack
    StickIntent intent;
    float x = 0, y = 0, z = 0;       // player node position before the update
    const std::vector<AttackCandidate>* candidates = nullptr;
    AttackEntity player = playerAttackEntity();
    const PlayerAttackTables* tables = nullptr;  // null: the defaults (weapon row 13, no fight moves)
    GameRandom* random = nullptr;    // 0x25b2b8 for the fight move choice
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
    std::optional<FightStart> fight;  // the entering update chose a fight move or a struggle
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
    int variant() const { return variant_; }
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
    int lastFightMove_ = -1;         // combat record byte +0xb (0xff before the first fight move)
};

}
