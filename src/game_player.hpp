#pragma once
#include "assets/assets.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "runtime/ai.hpp"
#include "runtime/buildit_player.hpp"
#include "runtime/carry_animation.hpp"
#include "runtime/scanner.hpp"
#include "runtime/zipline.hpp"
#include "runtime/climb_bar.hpp"
#include "runtime/pole.hpp"
#include "runtime/collision.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/player_vertical.hpp"
#include "runtime/sound_events.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace game_player {

// The class whose CharacterEnum 9 row ("police") is the one character the completion count excludes,
// taken as the starting character (inference). The idle animation is picked by file name only.
inline constexpr const char* kPlayerCharacter = "levels/chars/char_police.blvl";
inline constexpr std::uint16_t kPlayerCharacterId = 9;
inline constexpr const char* kPlayerBaseCharacter = "levels/chars/char_player.blvl";
inline constexpr const char* kPlayerAnimationDirectory = "models/char_minifig/class_police/";
inline constexpr const char* kPlayerAnimationSuffix = "_minifig.bfnanm";
inline constexpr const char* kCarryAnimationDirectory = "models/char_minifig/class_generic/";
// ctrl+0x6c of the player (read in the oracle, locomotion traces).
inline constexpr std::uint16_t kPlayerEntityType = 3;

// The player character: locomotion state, skeleton, role animations and the scene instances it moves.
struct PlayerRig {
    chase::runtime::LocomotionState motion;
    chase::runtime::VerticalState vertical;
    chase::runtime::JumpProfile jumpStanding, jumpMoving;
    bool jumpMovingAnimation = false;
    chase::render::CharacterRig character;
    chase::render::CharacterInstance instance;
    std::map<int, chase::assets::Animation> animations;  // 0 = idle, runtime::kAnimationWalk / Run / Sprint
    std::optional<chase::runtime::CarryAnimations> carryAnimations;  // the clips of the carried variants and of the pickup and put-down states are in `animations`
    mutable std::vector<chase::assets::Matrix> skin;                  // skin matrices of the last pose
    int animation = 0;
    double frame = 0;
    float startX = 0, startZ = 0;
    chase::runtime::FootstepDetector footsteps;
    chase::runtime::CharacterSounds characterSounds;
    std::vector<chase::runtime::SoundCall> sounds;  // the sound calls of the update, filled by updatePlayer and animate

    std::array<float, 3> position() const { return {motion.x, vertical.y, motion.z}; }
    chase::render::Mat4 matrix() const;
    void animate(chase::render::Scene& scene, float dt, int role, float scale);
    void pose(chase::render::Scene& scene) const;
    void poseClip(chase::render::Scene& scene, const chase::assets::Animation& clip, double clipFrame) const;
    void setVisible(chase::render::Scene& scene, bool visible) const;
    // World transform of a model hung on the joint `lowerJointName` in the last pose; nothing without the joint.
    std::optional<chase::render::Mat4> jointWorld(const std::string& lowerJointName) const;
    // The clip to play for `role` while carrying (`carryingMode` 0, 1, 2 as CarrySet::carryingMode) or in a pickup / put-down state (`stateClip`, 0 when none).
    int carriedRole(int role, int carryingMode, int stateClip) const;
};

struct PlayerAnimation {
    int role = 0;
    float scale = 1.0f;
};

std::int16_t headingFromDegrees(float degrees);

chase::assets::CharacterAppearance playerAppearance(chase::assets::FuseArchive& archive);

std::optional<PlayerRig> placePlayer(chase::assets::FuseArchive& archive, chase::render::Scene& scene, const chase::assets::Level& level);

// A scripted move (AIMove on the player, STAND-IN SCRIPT-S31): the AI position and heading replace the locomotion result.
void applyAiPosition(PlayerRig& player, const chase::runtime::AiCharacter& ai);
PlayerAnimation aiMoveAnimation(const chase::runtime::AiCharacter& ai);

PlayerAnimation updatePlayer(PlayerRig& player, const chase::runtime::CollisionWorld& collision, const chase::runtime::StickIntent& intent,
                             bool jumpPressed, float dt);

}
