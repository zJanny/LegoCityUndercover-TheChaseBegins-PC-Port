#pragma once
// The shadows in chase-game (runtime/shadow.hpp, render/shadow.hpp, docs/re/rendering-shadows.md): the instance flags from
// Shadow_Caster / Shadow_Receiver, the focus and camera of the shadow light each frame, the caster pass before the scene and the
// receiving draw after it.
#include "assets/assets.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "render/shadow.hpp"
#include "runtime/shadow.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace game_shadows {

// STAND-IN RENDER-S68: every level room model receives (0x1c3098 sets the receiver bit on the root node of a room's scene record when
// its +0xc byte is 0; that byte is not read), and the shader programs that have the Shadows uniform are taken to be those of every
// material (the program table 0x595410 is not read).
class LevelShadows {
public:
    void load(const std::vector<chase::assets::LightObject>& lights, const std::optional<float>& levelRadius);
    bool active() const { return light_.has_value(); }
    // Marks the first `roomInstances` instances (the level's room models) as receivers, the placed objects, the level characters and
    // the player (an instance of class `playerClass`) by their classes.
    void markLevel(chase::render::Scene& scene, std::size_t roomInstances, const chase::assets::Level& level, const chase::assets::ClassLookup& classOf,
                   const std::vector<std::size_t>& placedSceneIndex, const std::vector<chase::assets::Placement>& placedObjects,
                   const std::vector<chase::assets::CharacterObject>& characters,
                   const std::vector<std::optional<chase::render::CharacterInstance>>& placedCharacters,
                   const chase::render::CharacterInstance* player, const std::string& playerClass) const;
    void markObject(chase::render::Scene& scene, std::size_t instance, const chase::assets::LevelObject& object, const chase::assets::Level& cls) const;
    void markCharacter(chase::render::Scene& scene, const chase::render::CharacterInstance& character, const chase::assets::LevelObject& object,
                       const chase::assets::Level& cls) const;
    // The frame pass: `viewToWorld` is the inverse of the GL view matrix of the frame, `playerY` the player's height. Nothing is drawn
    // and apply() does nothing while `shown` is false (the scanner view, 0x51641e).
    void render(chase::render::Scene& scene, const chase::render::Mat4& viewToWorld, std::optional<float> playerY, bool shown);
    void apply(const chase::render::Scene& scene, const chase::render::Mat4& viewProjection) const;

private:
    std::optional<chase::runtime::ShadowLight> light_;
    chase::runtime::ShadowMatrix lightMatrix_{};
    std::optional<std::array<float, 3>> focus_;
    float radius_ = chase::runtime::kShadowRadiusDefault;
    bool drawn_ = false;
    mutable std::optional<chase::render::ShadowPass> pass_;
};

}
