#pragma once
// What an AI character in the idle state (AI state 0) does next (docs/re/scripts-missions-1-4-spawners-conduct.md): the decision routine 0x2a9874 of
// state 0 (word +0 of the state table *0x3771f0, called by the AI controller 0x200c30), its target search 0x1f8d5c over the two six-slot side
// lists 0x58deb0 (characters with entity +0xc bit 2, `AIBadGuy`) and 0x58dec8 (the others, slot 6 the player), the sight test 0x1f6958
// (`AIViewDistance`, `AIRangeBound`), the sighting 0x21102c (AI state 2 on the target), the engage 0x206d18 and the release 0x21decc of a
// side slot (ctrl+0xd8 bit 5). Every AI character runs the same rule; the parameters come from the AI set-up 0x2bc934 and the AI start
// 0x1c6e84. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/ai_patrol_walk.hpp"
#include "runtime/fall_arrest_engaged.hpp"
#include "runtime/game_random.hpp"
#include "runtime/goon_idle_return.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kAiSideSlots = kEngagedListSlots;                       // the loops of 0x206d18 and 0x21decc stop at 6
constexpr std::uint16_t kAiViewDistanceDefault = 18;          // 0x2451d0 default at 0x2bc974, stored as u16 at ctrl+0x7a
constexpr float kAiMeleeFireDistanceDefault = 2.2f;           // literal 0x400ccccd at 0x2bcd28 (ctrl+0x80)
constexpr float kAiFireDistanceDefault = 15.0f;               // literal 0x41700000 at 0x2bcd40 (ctrl+0x7c)
constexpr std::uint32_t kAiFriendlySightSineBits = 0x3f350481;  // 0x1f6ac0: the normalised y compared as integer bits (sin 45 degrees)
constexpr std::uint8_t kAiCombatState = 2;                    // 0x21102c stores 2 to ctrl+0x15
constexpr std::uint16_t kAiSightRefusedLocomotionState = 82;  // 0x211044: nothing in locomotion state 82
constexpr std::uint8_t kAiSeenBits = 2;                       // 0x21102c: ctrl+0xdc bits 3..4 = 2
constexpr float kAiRunSpeedDefault = 1.0f;                    // `AIRunSpeed` default, literal 0x2bcd00 (0x2bc94c)
constexpr float kAiRunSpeedScale = 0.23f;                     // *0x37450c (initialiser 0x343de4), the factor 0x2bc984 multiplies AIRunSpeed by
constexpr std::uint8_t kAiCharacterRowDefault = 192;          // 0x236d98: the default of `CharacterEnum`
constexpr std::uint8_t kAiCombatBehaviourDefault = 3;         // 0x2bce90: the default of `AICombatBehaviour`
constexpr std::uint8_t kAiFireDelayDefault = 60;              // 0x2bcab8: without `AIMeleeFireDelay` / `AIRangedFireDelay` 60 and a span of 30
constexpr std::uint8_t kAiFireDelaySpanDefault = 30;
constexpr std::uint8_t kAiTargetDelayDefault = 10;            // 0x2bcb44: the default of `AITargetDelay`
constexpr float kAiFriendlyHomeFactor = 8.0f;                 // literal 0x2a9d84: the friendly branch 0x2a9bc4 compares with 8 x ctrl+0x8c

// The parameters 0x2bc934 and 0x1c6e84 read for a placed character, and the entity bits of its create.
struct AiConductParams {
    std::uint16_t viewDistance = kAiViewDistanceDefault;      // `AIViewDistance` (integer reader 0x2451d0: 100.0 is 100)
    float meleeFireDistance = kAiMeleeFireDistanceDefault;    // `AIMeleeFireDistance`
    float fireDistance = kAiFireDistanceDefault;              // `AIFireDistance`
    std::string rangeBound;                                   // `AIRangeBound`, the bound 0x23e798 names (ctrl+0xcc)
    std::uint8_t targetMode = 0;                              // `AIDontTarget` & 7 into ctrl+0xda bits 0..2 (and 3..5)
    bool ignorePlayer = false;                                // `AIIgnorePlayer`, ctrl+0xd9 bit 0
    bool seesInvisible = false;                               // `AISeesInvisibleChars`, ctrl+0xdb bit 6
    bool friendly = false;                                    // `AIFriendly`, ctrl+0xdb bit 2 (0x1c6e84)
    bool closeSight = false;                                  // ctrl+0xdb bit 1 (0x21e798: a ranged weapon), the shortcut of the sight test 0x1f6aa0
    float runSpeed = kAiRunSpeedDefault * kAiRunSpeedScale;   // ctrl+0x8c: `AIRunSpeed` x *0x37450c (0x2bc988, after the character set-up's write)
    bool returnToSpawnPoint = true;                           // `ReturnToSpawnPoint`, ctrl+0xdc bit 5 (0x1c6e84)
    bool millAbout = false;                                   // `AIMillAbout`, ctrl+0x315 bit 7 (0x1c6e84)
    bool patrolPath = false;                                  // `AIPatrolPath` names an object: 0x1c6e84 requests AI state 14 instead of 0
    std::uint32_t patrolPathHash = 0;                         // the name hash of that object (0x24242c at 0x2a11cc)
    bool dontRun = false;                                     // `AIDontRun`, ctrl+0xdb bit 4: the walk back is a walk
    bool patrolAlertOnLos = false;                            // `AIPatrolAlertOnLOS`, ctrl+0xdb bit 5: the angle flag of the watch 0x1f5c3c
    AiIdleReturnParams idleReturn;                            // the sleep clips and `Wait_Time` (goon_idle_return.hpp)
    bool badGuy = false;                                      // `AIBadGuy`, entity +0xc bit 2
    bool boss = false;                                        // `Boss_IsBoss`, entity +0xc bit 14
    std::uint8_t characterRow = kAiCharacterRowDefault;       // `CharacterEnum`, ctrl+0x2cc: the row of tables/tablescharacters.bxls sheet 0
    std::uint8_t combatBehaviour = kAiCombatBehaviourDefault; // `AICombatBehaviour` & 7, ctrl+0xdc bits 0..2; applyAiCombatFacts adjusts it (0x2bceb0)
    bool melee = false;                                       // ctrl+0xdb bit 0: ability 19 and a melee weapon row (0x2bc998)
    bool strike = false;                                      // ability 54, asked by the decision of AI state 3 (0x2b83c0)
    bool charge = false;                                      // ability 63 (0x2b830c)
    bool dontChase = false;                                   // `AIDontChase`, ctrl+0xd8 bit 7
    bool lineOfSight = false;                                 // `AILineOfSight`, ctrl+0xd9 bit 4
    bool dontFaceTarget = false;                              // `AIDontFaceTarget`, ctrl+0xda bit 6
    std::uint8_t meleeDelay = kAiFireDelayDefault;            // `AIMeleeFireDelay` first value, ctrl+0x73
    std::uint8_t meleeDelaySpan = kAiFireDelaySpanDefault;    // second - first, 1 when equal, ctrl+0x74
    std::uint8_t rangedDelay = kAiFireDelayDefault;           // `AIRangedFireDelay`, ctrl+0x75
    std::uint8_t rangedDelaySpan = kAiFireDelaySpanDefault;   // ctrl+0x76
    std::uint8_t targetDelay = kAiTargetDelayDefault;         // `AITargetDelay`, ctrl+0x77
};
AiConductParams aiConductParamsOf(const assets::LevelObject& object, const assets::Level& cls);
// The request at the end of 0x1c6e84: AI state 14 with an `AIPatrolPath`, 9 for a friendly character that mills about, else 0.
std::uint8_t aiStartRequest(const AiConductParams& params);

// One AI character, or the player (`player`). The host fills the first block every update; the conduct keeps the second.
struct AiConductEntity {
    std::string name;
    bool player = false;
    AiConductParams params;
    std::optional<std::size_t> rangeBound;  // index of params.rangeBound in Level::bounds (0x23e798 finds none: no bound)

    std::array<float, 3> position{};        // node translation (node +0x30)
    std::array<float, 3> forward{0.0f, 0.0f, 1.0f};  // node +0x20, the row the angle test 0x15c1b0 reads
    std::optional<std::array<float, 3>> spawn;  // ctrl+0xac..0xb4, the spawn point
    bool present = true;                    // a node (+0x3c), entity +0x10 bit 0 and +0xc bit 4 clear
    bool hidden = false;                    // 0x23de70: no node or its hidden flags
    bool thinks = true;                     // the AI controller 0x200c30 runs this update (director hold, +0xc bit 4)
    std::uint8_t aiState = 0;               // ctrl+0x15 (the port takes requests at once, so +0x14 equals it)
    std::uint16_t locomotionState = 1;      // ctrl+0x6c
    bool invisible = false;                 // ctrl+0x316 & 0xc0 (0x1f6b18)

    bool engaged = false;                   // ctrl+0xd8 bit 5: the entity holds a slot of its side list
    std::string target;                     // ctrl+0xc4
    std::uint8_t aggroCounter = 0;          // ctrl+0x72
    std::uint32_t countdown = 0;            // ctrl+0x84, counted down by 0x2a9874
    bool cantGetThere = false;              // ctrl+0xd9 bit 2, set by 0x1f6c58 when the path search fails
    std::uint8_t seenBits = 0;              // ctrl+0xdc bits 3..4
    bool clipEnded = kAiClipEndedStandIn;   // 0x22f560 of the current clip answers 6
    std::uint32_t patrolObject = 0;         // ctrl+0xd4: the name hash of the patrol node the character walks to, written by AI state 14
    bool walk = false;                      // ctrl+0xd9 bit 5: the walk of AI state 14 is a walk, not a run
    bool avoidMove = false;                 // ctrl+0xd9 bit 3: the avoid argument of the last move 0x22d85c
    std::array<float, 3> destination{};     // ctrl+0xb8: where the last move leads, or where the character stood when its attack began
    std::optional<std::uint16_t> heading;   // ctrl+6 and ctrl+10 once a fight decision turned the character to its target
    bool moveWanted = false;                // ctrl+0xc bit 0, set by the fight decisions (the walk it asks for is not ported)
    std::uint8_t weaponBits = 0;            // ctrl+0x2cd: which weapon is out (0x2106d8)
    std::uint16_t pressBits = 0;            // ctrl+0xe bits 5..7 of this update: the attack presses of 0x2896d4
    bool conductWalk = false;               // host: the walk in progress was started by the conduct's walker, not by a script native
    bool landed = true;                     // ctrl+0x314 bit 6
    bool arrestTarget = false;              // host: locomotion state 323 or 324, or the arrest test 0x2107e0 passes (0x2c4780)
};

enum class AiConductEventKind : std::uint8_t {
    Engaged, Released, Sighted, MillAbout, SleepClip, ReturnToSpawn, Returned, PatrolWalk, CantGetThere, WalkEnded, Idle, PlayerCaught,
    FightState, RunTo, AttackPress, LostTarget, Taunt, OutsideBound, MoveEnded
};
struct AiConductEvent {
    AiConductEventKind kind = AiConductEventKind::Engaged;
    std::string character;
    std::string target;     // Sighted
    float distance = 0;     // Sighted: the distance 0x1f6958 measured
    bool badSide = false;   // Engaged, Released: list 0x58deb0
    std::size_t slot = 0;   // Engaged, Released
    bool combat = false;    // Sighted: the character holds a slot and AI state 2 was requested
    std::string clip;       // SleepClip: the clip name 0x2102b0 plays
    bool loop = false;      // SleepClip: the sleep clip (r2 = 1), not the wake clip
    AiPatrolSource source = AiPatrolSource::None;  // PatrolWalk: where the walk of AI state 14 leads
    bool patrolling = false;  // PatrolWalk: the character walks a patrol, not back to its spawn point
    bool toWait = false;      // WalkEnded: AI state 15 follows, else the character is idle
    std::uint8_t state = 0;   // FightState: the AI state requested (ai_pursuit.hpp), with `target`
    std::array<float, 3> destination{};  // RunTo
    std::uint16_t press = 0;  // AttackPress: the ctrl+0xe bit
};

// The path follower of AI state 14 on the host's side: `start` is 0x215988 and the request 0x1f9834(…, 1) of the enter (false: no path, the
// log "I can't get there"), `follow` is 0x1faf04 and its application 0x1fae48 (the low byte of the code: 0 arrived, 1 and 2 blocked, else
// moving).
struct AiConductWalker {
    std::function<bool(const AiConductEntity&, const std::array<float, 3>& destination)> start;
    std::function<int(const AiConductEntity&)> follow;
};
// A patrol node as the host knows it: its position and the object `next_Node` names (0 none).
struct AiPatrolNode {
    std::array<float, 3> position{};
    std::uint32_t next = 0;
};
using AiPatrolNodeLookup = std::function<std::optional<AiPatrolNode>(std::uint32_t hash)>;

// 0x22f650(point, bound, 0) of the host: the bound's switch +0xa and its shape around the centre.
using AiConductBoundTest = std::function<bool(std::size_t bound, const std::array<float, 3>& point)>;

class AiConduct {
public:
    // The level's AI characters and the player; slots and AI fields start clear. `table` is the one owner of the side lists and must outlive the
    // conduct; it is cleared.
    void reset(EngagedTable& table, std::vector<AiConductEntity> entities);
    std::vector<AiConductEntity>& entities() { return entities_; }
    const std::vector<AiConductEntity>& entities() const { return entities_; }
    AiConductEntity* find(const std::string& name);

    // One update: the state enters for entities whose AI state changed since the last update (0x289650 for 0 and 0x2a9870 for 1 release the
    // slot), then the decision of state 0 for every entity that thinks in state 0, in entity order.
    void update(const AiConductBoundTest& inside);
    std::vector<AiConductEvent> takeEvents();

    // 0x206d18 and 0x21decc.
    void engage(AiConductEntity& self);
    void release(AiConductEntity& self);
    // 0x1f6958(viewer, target, out, limit, second, 0): a negative limit takes AIViewDistance, a negative second the melee or fire distance; a viewer
    // with ctrl+0xdb bit 1 sees a target within the second distance and below 45 degrees whatever its bound.
    // `angle` (radians) is the pointer 0x1f6958 takes as its fourth argument: the angle test 0x15c1b0 must pass first.
    bool sees(const AiConductEntity& viewer, const AiConductEntity& target, float limit, float second, const AiConductBoundTest& inside,
              float* distance = nullptr, const float* angle = nullptr) const;
    // 0x1f8d5c(self, position, angle flag, limit, second, 45.0): the entity to attack, or null.
    const AiConductEntity* findTarget(const AiConductEntity& self, float limit, float second, const AiConductBoundTest& inside, bool angleFlag = false);
    // 0x2a9874 for one entity in state 0.
    void thinkIdle(AiConductEntity& self, const AiConductBoundTest& inside);
    // The request of AI state 15 at 0x2a9d40 with its enter 0x2bba40, and the decision 0x2c5cac of state 15.
    void requestReturn(AiConductEntity& self);
    void thinkReturn(AiConductEntity& self, const AiConductBoundTest& inside);
    // The request of AI state 14 at 0x2c5cf8 with its enter 0x2a11a0, and the decision 0x2b68e8 of state 14.
    void requestPatrol(AiConductEntity& self);
    void thinkPatrol(AiConductEntity& self, const AiConductBoundTest& inside);
    // The watch 0x1f5c3c of states 14 and 15: a target in sight raises PlayerCaught and starts the fight without a slot.
    void watch(AiConductEntity& self, const AiConductBoundTest& inside);
    void setWalker(AiConductWalker walker) { walker_ = std::move(walker); }
    void setPatrolNodes(AiPatrolNodeLookup lookup) { patrolNodes_ = std::move(lookup); }
    // 0x21102c(self, target); `held` when the caller set ctrl+0xd8 bit 5 itself (the watch 0x1f5c3c), so the engage does nothing.
    void sighted(AiConductEntity& self, const AiConductEntity& target, float distance, bool held = false);
    // What the fight decisions (ai_pursuit.hpp) need of the conduct: the entity ctrl+0xc4 names when it has a node (0x1f522c), the next word of
    // 0x25b2b8, the event list, the walker, the idle request 0x2246c0 with the enter of state 0, the mill-about request 0x22471c, the side
    // cool-downs 0x3771da (the others) / 0x3771dc (the bad guys) and the side lists.
    const AiConductEntity* targetOf(const AiConductEntity& self) const;
    const AiConductEntity* named(const std::string& name) const { return entityNamed(name); }
    std::uint32_t random() { return random_.next(); }
    void push(AiConductEvent event) { events_.push_back(std::move(event)); }
    const AiConductWalker& walker() const { return walker_; }
    void requestIdle(AiConductEntity& self);
    void requestMillAbout(AiConductEntity& self);
    std::uint16_t& sideCooldown(bool badGuy) { return sideCooldowns_[badGuy ? 1 : 0]; }
    // The clock 0x241098 the decision of state 0 tests.
    void setClock(std::uint32_t clock) { clock_ = clock; }

    const std::array<std::string, kAiSideSlots>& badSide() const { return table_->list(true); }
    const std::array<std::string, kAiSideSlots>& goodSide() const { return table_->list(false); }

private:
    AiConductEntity* entityNamed(const std::string& name);
    const AiConductEntity* entityNamed(const std::string& name) const;
    const AiConductEntity* player() const;
    const AiConductEntity* validTarget(const AiConductEntity& self) const;
    void enterReturn(AiConductEntity& self);
    void enterPatrol(AiConductEntity& self);

    std::vector<AiConductEntity> entities_;
    std::vector<std::uint8_t> lastState_;
    EngagedTable* table_ = nullptr;
    std::vector<AiConductEvent> events_;
    std::uint32_t clock_ = 1;
    AiConductWalker walker_;
    AiPatrolNodeLookup patrolNodes_;
    GameRandom random_{kAiPatrolRetrySeedStandIn};
    std::array<std::uint16_t, 2> sideCooldowns_{};
};

// STAND-IN SCRIPT-S326: the spawn point is the position of the first update the character is present (the original stores its node position at
// the AI start 0x1c6e84), 0x20bf3c, which can clear ReturnToSpawnPoint, answers 1, and a friendly character that goes back to its spawn point
// only takes a slot (the move 0x1f6c58 is not applied).
// STAND-IN SCRIPT-S326: the global byte tests the port does not track answer 0: 0x3771d6 (the boss branch of 0x1f8d5c) and 0x22b0b4
// (byte 0x5163e8 + 37, mill about for every idle character).
constexpr bool kAiConductBossSearchStandIn = false;
constexpr bool kAiConductGlobalMillAboutStandIn = false;

}
