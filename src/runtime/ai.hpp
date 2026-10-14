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

#include "runtime/ai_simple_path.hpp"
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
// stores 1. FollowPath: the path state writes 5. MillAbout: 0x22471c stores 9. FaceTarget: 0x211110 stores 10.
// Takedown / Pursuit: AIPursuit passes 35 (takedown argument set) or 36 to 0x218b38. MoveTo: the state AIMove requests
// through 0x2414b4 (its number is not read; the value is a placeholder).
enum class AiState : std::uint8_t {
    Idle = 0,
    NoState = 1,
    FollowPath = 5,
    MillAbout = 9,
    FaceTarget = 10,
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

struct AiSuperJump {
    std::array<float, 3> target{};
    float height = 0;
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
    // The nav grids of the level, one world per section record (nav_grid.hpp). With them an AIMove runs the path finder
    // 0x204284 first like the front end 0x22d85c: result 1 ("I can't get there") leaves the character unchanged.
    void setNavigation(std::vector<NavWorld> worlds) {
        navigation_ = std::make_shared<std::vector<NavWorld>>(std::move(worlds));
        for (NavWorld& world : *navigation_) world.now = navClock_;
    }
    const std::vector<std::string>& cannotGetThere() const { return cannotGetThere_; }

    // Handler of `kind` for the characters named by lower-case names and the numbers already read from the script.
    void run(AiNativeKind kind, const std::vector<std::string>& names, const std::vector<float>& numbers);

    // Position of a character that something else moves (the player node, PositionObject).
    void setPosition(const std::string& lowerCaseName, const float position[3]);
    bool moving(const std::string& lowerCaseName) const;
    // True when the last update() advanced a move of that character (the host copies the new position).
    bool movedLastUpdate(const std::string& lowerCaseName) const;
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
    void advanceNavMove(const std::string& name, AiCharacter& target, float dt);
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
};

}
