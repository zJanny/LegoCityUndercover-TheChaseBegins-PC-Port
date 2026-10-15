#pragma once
// Minifig characters in a Scene: the skinned body with the head (and hat) model on its joint, shared by
// the player and the characters of a level. Appearance rules: docs/re/player.md and docs/re/objects.md.
#include "render/scene.hpp"
#include <optional>
#include <string>

namespace chase::render {

struct CharacterRig {
    assets::Skeleton skeleton;
    std::optional<std::size_t> headJoint, hatJoint;
};

struct CharacterInstance {
    std::string bodyKey;
    std::size_t bodyInstance = 0, headInstance = 0;
    std::optional<std::size_t> hatInstance;
};

CharacterRig loadCharacterRig(assets::FuseArchive& archive, const std::string& bodyModelPath);
assets::Animation loadAnimation(assets::FuseArchive& archive, const std::string& path);

// Adds the body, head and optional hat model. `base` names the placeholder textures the shared models
// carry (the char_player class), `look` the ones that replace them. The skinned body is shared by every
// instance with the same look and `poseOwner`, so characters that are posed differently need different owners.
CharacterInstance addCharacter(Scene& scene, const assets::CharacterAppearance& base, const assets::CharacterAppearance& look,
                               const std::string& hat, const std::string& poseOwner, const Mat4& world);

// The base look for CharacterAppearance (the placeholder textures of the shared models): class char_player.
assets::CharacterAppearance readBaseAppearance(assets::FuseArchive& archive);

// The animation file `name` ("idle", "idle2") of an animation class folder; the generic class when the folder has none.
std::string levelCharacterAnimationPath(assets::FuseArchive& archive, const std::string& animationClass, const char* name);

// Draws the characters of a level standing in place: each at its placement, in frame 0 of the idle
// animation of its animation class, each with its own skinned body so that it can be posed on its own. placeLevelCharacters returns the scene instances per character (empty for the ones left out), addLevelCharacters how many were added.
std::vector<std::optional<CharacterInstance>> placeLevelCharacters(Scene& scene, assets::FuseArchive& archive,
                                                                   const std::vector<assets::CharacterObject>& characters);
std::size_t addLevelCharacters(Scene& scene, assets::FuseArchive& archive, const std::vector<assets::CharacterObject>& characters);

// Skins the body model of the instance's key with `animation` at `frame`; returns the skin matrices.
std::vector<assets::Matrix> poseBody(Scene& scene, const CharacterRig& rig, const CharacterInstance& instance,
                                     const assets::Animation& animation, double frame);
// Places the body at `world` and moves the head and hat to their joints.
void placeCharacter(Scene& scene, const CharacterRig& rig, const CharacterInstance& instance,
                    const std::vector<assets::Matrix>& skin, const Mat4& world);

}
