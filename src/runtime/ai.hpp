#pragma once
// AI core and the AI natives of batch SCRIPT-34 (docs/re/ai-natives.md): the per-character AI state the natives write
// (component at entity+0x7c), the move orders of AIMove / AIMoveNoAvoid / AIFollowSimplePath and the AIMove trigger
// event raised when a move ends. AIFollowSimplePath follows state 22 (ai_simple_path.hpp); the nav-mesh path finder
// behind AIMove and the locomotion are not ported: an AIMove is a straight walk (STAND-IN SCRIPT-S31). No SDL dependency.
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "runtime/ai_face_target.hpp"
#include "runtime/ai_go_to_target.hpp"
#include "runtime/ai_simple_path.hpp"
#include "runtime/ai_super_jump.hpp"
#include "runtime/nav_follow.hpp"

namespace chase::runtime {

enum class AiNativeKind {
    AIEnable,
    AIActive,
    AIMove,
    AIMoveNoAvoid,
    AIFaceTarget,
    AIIgnorePlayer,
    AINoState,
    CharacterInvulnerable,
    AISuperJump,
    AIResetState,
    AIFollowSimplePath,
    AIPursuit,
    AICantAttack,
    AIStalkPlayer,
    AIMillAbout,
    AIRunAway,
};

std::optional<AiNativeKind> aiNativeKind(const std::string& lowerCaseName);
std::vector<std::string> aiNativeNames();
// Leading script arguments of each native, an object or path argument counts as one.
unsigned aiNativeArguments(AiNativeKind kind);
// How many of the leading arguments are object or path names; the rest are numbers.
unsigned aiNativeObjects(AiNativeKind kind);
// Every AI native is a STAND-IN: the flags and requested states are transcribed, the effects of the state machine
// (path finding, locomotion, jump, pursuit, stalking, fleeing) are not (SCRIPT-S31).
bool aiNativeIsStandIn(AiNativeKind kind);

// Requested state byte of the AI component (+0x15). Idle: 0x2246c0 stores 0. NoState: 0x241304 (AIEnable, AINoState)
// stores 1. Combat: the sighting 0x21102c stores 2; Attack: 0x2080a0 stores 3; Move: the move front end 0x22d85c stores 4; StrikeWindUp: the decision
// of state 3 requests 31 (ai_pursuit.hpp runs these on the conduct's entities). FollowPath: the path state writes 5. MillAbout: 0x22471c stores 9. FaceTarget: 0x211110 stores 10.
// RunAway: 0x228c70 requests 25. Stalk: 0x282b88 requests 27. Takedown / Pursuit: AIPursuit passes 35 (takedown argument set) or 36 to 0x218b38. MoveTo: the state AIMove requests
// through 0x2414b4 (its number is not read; the value is a placeholder).
enum class AiState : std::uint8_t {
    Idle = 0,
    NoState = 1,
    Combat = 2,
    Attack = 3,
    Move = 4,
    FollowPath = 5,
    MillAbout = 9,
    FaceTarget = 10,
    GoToTarget = 11,
    Patrol = 14,
    ReturnToSpawn = 15,
    RunAway = 25,
    Stalk = 27,
    StrikeWindUp = 31,
    Takedown = 35,
    Pursuit = 36,
    MoveTo = 0xfe,
};

struct AiMoveOrder {
    std::vector<std::array<float, 3>> waypoints;
    bool run = false;
    bool noAvoid = false;
    bool path = false;
    std::optional<SimplePathFollow> follow;
    // Nav path of the original finder (nav_path.hpp) when the nav grids are set and the position lies in a single grid;
    // reset when the follower cannot continue, the order then walks straight (SCRIPT-S31).
    std::shared_ptr<NavPathRecord> nav;
    std::size_t navWorld = 0;
    bool blocked = false;  // AI state 14: the path request failed, the follower answers 1
    bool conduct = false;  // started by startPatrolWalk: the decision of its AI state runs the follower (followPatrolWalk), update() leaves it alone
    // Stand-in SCRIPT-S100: height of the character above the base of the grid it walks in, taken at the first step.
    std::optional<float> navHeightOffset;
};

// A finished move: the AIMove event is raised for `character` with the start argument of the raise site (0x23cee8's
// fourth argument: 255 at 0x2cbf10; component byte 0 at the other sites, whose value is not recovered and stays 0).
struct AiMoveEnd {
    std::string character;
    std::uint16_t argument = 0;
};

struct AiPursuit {
    std::string firstNode;
    bool takedown = false;
};

// AISuperJump: the setup of 0x213300 (plan) and the flight of locomotion states 72 / 73. The horizontal step is the code's
// (stepSuperJumpHorizontal). STAND-IN SCRIPT-S103: the height follows a parabola from the start through the apex `plan.climb`
// above the start to the target height; the original plays the jump-run animation with its root motion scaled to the climb
// (0x200cc4) and ends the state after 28 counts of ctrl+0x2cf or on landing (0x28e4cc).
struct AiSuperJump {
    std::array<float, 3> target{};
    float height = 0;
    SuperJumpPlan plan;
    std::array<float, 3> start{};
    float traveled = 0;
    float horizontalDistance = 0;
    // Set when another order replaced the flight (AIFaceTarget, AIResetState, ...): x and z stay where they are and the height goes on until the
    // host lands the character (AiCore::land). docs/re/ai-states.md, the girder guy's AIFaceTarget in the oracle.
    bool released = false;
};

// Values the natives wrote, unset until a script wrote them. Meaning of the original bits (component bytes):
//  enabled: AIEnable, bit 4 of +0xd8; homePosition: +0xb8, the node position at the AIEnable call;
//  invulnerable: CharacterInvulnerable, bit 6 of +0xd8;
//  ignorePlayer: AIIgnorePlayer, bit 0 of +0xd9;
//  canAttack: AICantAttack, bit 7 of +0xdb, set for an argument of 0;
//  millAbout: AIMillAbout, bit 7 of +0x315;
//  heading: AIFaceTarget, u16 at +10 (angle * 65536 / 2 pi).
struct AiCharacter {
    std::optional<bool> enabled;
    std::optional<std::array<float, 3>> homePosition;
    std::optional<bool> invulnerable;
    std::optional<bool> ignorePlayer;
    std::optional<bool> canAttack;
    std::optional<bool> millAbout;
    std::optional<std::uint16_t> heading;
    AiState state = AiState::Idle;
    std::optional<AiMoveOrder> move;
    std::string faceTarget;
    std::optional<AiPursuit> pursuit;
    std::string combatTarget;
    std::optional<float> stalkDistance;
    std::optional<AiSuperJump> superJump;
    bool runningAway = false;
    std::uint32_t stateResets = 0;
    std::optional<std::array<float, 3>> position;
};

// Walk speed measured at the police entry (oracle, docs/re/scripts.md: 0.11 per update of 1/30 s); run speed is the
// player's run speed per step times 30 steps per second (player_locomotion.hpp). STAND-IN: the AI states take the speeds
// from the character's locomotion parameters.
constexpr float kAiWalkSpeed = 0.11f * 30.0f;
constexpr float kAiRunSpeed = 0.23f * 30.0f;
constexpr float kAiUpdatesPerSecond = 30.0f;
// State 11 compares the horizontal distance with ctrl+0x8c, ATTR_RunSpeed (0x236ed4; default 0.23, the class value of other
// characters is not read, SCRIPT-S104). Bit 6 of +0x314 (a character below the target has arrived) is stood in by false.
constexpr float kAiGoToMinDistance = 0.23f;
// STAND-IN SCRIPT-S318: a released flight (AiSuperJump::released) lands when the ground probe finds a floor within this distance below the node;
// the original's fall lands on its own ground contact (docs/re/ai-states.md: 0.41 in the last update before the landing).
constexpr float kReleasedFlightReach = 0.4f;

// STAND-IN SCRIPT-S94: 0x204284 doubles the entity's float at +0x70 into the height tolerance of its lookups; 0.85 is the
// player's value (oracle read), other characters' values are not read.
constexpr float kAiNavHeightTolerance = 0.85f;
// Path record +0x74 and +0x78 are 0x202054 = 0x2b66d8(entity) | 1; the other bits come from the gate types registered in the
// table at global 0x376f10 (which entity may use which gate), not recovered: STAND-IN SCRIPT-S94, only bit 0 (always set).
constexpr std::uint32_t kAiNavGateMask = 1;
// STAND-IN SCRIPT-S101: the clock of 0x2355c0 that dates the temporary marks of the grids (older than 1 s or 2 s counts as
// old) is the game time in seconds, which is far above 2 at any level start; the nav clock here starts at 10 and runs with update().
constexpr float kAiNavClockStart = 10.0f;

class AiCore {
public:
    using PositionLookup = std::function<bool(const std::string&, float[3])>;
    using PathLookup = std::function<bool(const std::string&, SimplePathFollow&)>;

    void setPositionLookup(PositionLookup lookup) { lookup_ = std::move(lookup); }
    void setPathLookup(PathLookup lookup) { pathLookup_ = std::move(lookup); }
    // Asked for every character before update() advances it; false leaves it untouched (the control routine 0x29bf30 of an
    // active director camera, director_hold.hpp).
    using UpdateGate = std::function<bool(const std::string&)>;
    void setUpdateGate(UpdateGate gate) { updateGate_ = std::move(gate); }
    // The nav grids of the level, one world per section record (nav_grid.hpp). With them an AIMove runs the path finder
    // 0x204284 first like the front end 0x22d85c: result 1 ("I can't get there") leaves the character unchanged.
    void setNavigation(std::vector<NavWorld> worlds) {
        navigation_ = std::make_shared<std::vector<NavWorld>>(std::move(worlds));
        for (NavWorld& world : *navigation_) world.now = navClock_;
    }
    const std::vector<std::string>& cannotGetThere() const { return cannotGetThere_; }

    // Handler of `kind` for the characters named by lower-case names and the numbers already read from the script.
    void run(AiNativeKind kind, const std::vector<std::string>& names, const std::vector<float>& numbers);

    // 0x2246c0 (requested state 0) and 0x22471c (requested state 9 when the character mills about) for callers other than the natives, the
    // StartState entries of the AI spawner (ai_start_state.hpp).
    void startIdle(const std::string& lowerCaseName) { requestIdle(at(lowerCaseName)); }
    void startMillAbout(const std::string& lowerCaseName) { requestMillAbout(at(lowerCaseName)); }
    // 0x21102c for a character that holds a side slot: AI state 2 with ctrl+0xc4 = `target` (goon_conduct.hpp).
    void startCombat(const std::string& lowerCaseName, const std::string& target);
    // The requests of AI states 15 (0x2a9874) and 14 (0x2c5cac) of the idle conduct; a state other than 14 ends the walk of AI state 14.
    // The fight states 4 and 5 (ai_pursuit.hpp) keep a walk the conduct started.
    void startConductState(const std::string& lowerCaseName, AiState state) {
        AiCharacter& character = at(lowerCaseName);
        character.state = state;
        const bool walks = state == AiState::Patrol || ((state == AiState::Move || state == AiState::FollowPath) && character.move && character.move->conduct);
        if (!walks) character.move.reset();
    }
    // ctrl+0xc4 of a character whose fight decision chose another target without a state change.
    void setCombatTarget(const std::string& lowerCaseName, const std::string& target) { at(lowerCaseName).combatTarget = target; }
    // ctrl+6 and ctrl+10 written by a fight decision: the character at `position` turns to `heading` (the host copies it like a move).
    void face(const std::string& lowerCaseName, const std::array<float, 3>& position, std::uint16_t heading);
    // The AIMove event 0x23cee8 the decision of AI state 4 raises when a walk of the conduct arrives (0x2c630c).
    void endConductMove(const std::string& lowerCaseName) { ended_.push_back({lowerCaseName, 0}); }
    // The path follower of AI state 14 (goon_conduct.hpp): `startPatrolWalk` is the end of the enter 0x2a11a0 (the path request from `from` to
    // `destination`; false when the finder cannot get there), `followPatrolWalk` is 0x1faf04 with its application 0x1fae48 for one update and
    // returns the code (low byte: 0 arrived, 1 and 2 blocked, 3 and more moving). STAND-IN SCRIPT-S31: without nav grids or outside them the
    // character walks straight, arriving within the speed of one step.
    // `state` is the AI state of the walk: 14, or 4 / 5 for the moves of the fight states (0x22d85c).
    bool startPatrolWalk(const std::string& lowerCaseName, const std::array<float, 3>& from, const std::array<float, 3>& destination, bool walk,
                         AiState state = AiState::Patrol);
    int followPatrolWalk(const std::string& lowerCaseName, bool walk, float dt);

    // Position of a character that something else moves (the player node, PositionObject).
    void setPosition(const std::string& lowerCaseName, const float position[3]);
    bool moving(const std::string& lowerCaseName) const;
    // True when the last update() advanced a move of that character (the host copies the new position).
    bool movedLastUpdate(const std::string& lowerCaseName) const;
    // A released flight (AiSuperJump::released) ends: the character stands on the ground at `height`.
    void land(const std::string& lowerCaseName, float height);
    // Advances every move order; characters whose order ended in this call are returned once (the AIMove event).
    void update(float dt);
    std::vector<AiMoveEnd> takeMovesEnded();

    const AiCharacter* character(const std::string& lowerCaseName) const;
    const std::vector<std::string>& unresolvedTargets() const { return unresolved_; }

private:
    AiCharacter& at(const std::string& lowerCaseName);
    bool locate(const std::string& lowerCaseName, std::array<float, 3>& out);
    void startMove(const std::string& character, std::vector<std::array<float, 3>> waypoints, bool run, bool noAvoid, bool path);
    void startSimplePath(const std::string& character, SimplePathFollow follow, bool run);
    void advanceStraightMove(const std::string& name, AiCharacter& target, float dt);
    void advanceSimplePath(const std::string& name, AiCharacter& target, float dt);
    void advanceSuperJump(const std::string& name, AiCharacter& target, float dt);
    bool advanceReleasedFlight(AiCharacter& target, float dt);
    void advanceFaceTarget(const std::string& name, AiCharacter& target);
    void advanceNavMove(const std::string& name, AiCharacter& target, float dt);
    void stepTowards(AiCharacter& target, AiMoveOrder& order, const std::array<float, 3>& goal, float dt);
    bool startNavMove(const std::string& name, AiCharacter& target, const std::array<float, 3>& from, const std::array<float, 3>& to,
                      bool noAvoid);
    void requestIdle(AiCharacter& character);
    void requestMillAbout(AiCharacter& character);

    std::map<std::string, AiCharacter> characters_;
    std::vector<AiMoveEnd> ended_;
    std::vector<std::string> moved_;
    std::vector<std::string> unresolved_;
    std::vector<std::string> cannotGetThere_;
    std::shared_ptr<std::vector<NavWorld>> navigation_;
    float navClock_ = kAiNavClockStart;
    PositionLookup lookup_;
    PathLookup pathLookup_;
    UpdateGate updateGate_;
};

}
