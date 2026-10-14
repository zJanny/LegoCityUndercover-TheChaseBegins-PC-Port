#pragma once
// AI core and the AI natives of batch SCRIPT-34 (docs/re/ai-natives.md): the per-character AI state the natives write
// (component at entity+0x7c), the move orders of AIMove / AIMoveNoAvoid / AIFollowSimplePath and the AIMove trigger
// event raised when a move ends. The nav-mesh path finder and the locomotion states are not ported: a move is a
// straight walk (STAND-IN SCRIPT-S31). No SDL dependency.
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

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

class AiCore {
public:
    using PositionLookup = std::function<bool(const std::string&, float[3])>;
    using PathLookup = std::function<const std::vector<std::array<float, 3>>*(const std::string&)>;

    void setPositionLookup(PositionLookup lookup) { lookup_ = std::move(lookup); }
    void setPathLookup(PathLookup lookup) { pathLookup_ = std::move(lookup); }

    // Handler of `kind` for the characters named by lower-case names and the numbers already read from the script.
    void run(AiNativeKind kind, const std::vector<std::string>& names, const std::vector<float>& numbers);

    // Position of a character that something else moves (the player node, PositionObject).
    void setPosition(const std::string& lowerCaseName, const float position[3]);
    bool moving(const std::string& lowerCaseName) const;
    // True when the last update() advanced a move of that character (the host copies the new position).
    bool movedLastUpdate(const std::string& lowerCaseName) const;
    // Advances every move order; characters whose order ended in this call are returned once (the AIMove event).
    void update(float dt);
    std::vector<std::string> takeMovesEnded();

    const AiCharacter* character(const std::string& lowerCaseName) const;
    const std::vector<std::string>& unresolvedTargets() const { return unresolved_; }

private:
    AiCharacter& at(const std::string& lowerCaseName);
    bool locate(const std::string& lowerCaseName, std::array<float, 3>& out);
    void startMove(const std::string& character, std::vector<std::array<float, 3>> waypoints, bool run, bool noAvoid, bool path);
    void requestIdle(AiCharacter& character);
    void requestMillAbout(AiCharacter& character);

    std::map<std::string, AiCharacter> characters_;
    std::vector<std::string> ended_;
    std::vector<std::string> moved_;
    std::vector<std::string> unresolved_;
    PositionLookup lookup_;
    PathLookup pathLookup_;
};

}
