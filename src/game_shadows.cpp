#include "game_shadows.hpp"

namespace game_shadows {
namespace runtime = chase::runtime;
using chase::render::Mat4;

void LevelShadows::load(const std::vector<chase::assets::LightObject>& lights, const std::optional<float>& levelRadius) {
    light_ = runtime::shadowLight(lights, levelRadius);
    if (!light_) return;
    lightMatrix_ = chase::assets::placementMatrix(lights[light_->lightIndex].placement);
    radius_ = light_->radius;
    pass_.emplace();
}

void LevelShadows::markLevel(chase::render::Scene& scene, std::size_t roomInstances, const chase::assets::Level& level,
                             const chase::assets::ClassLookup& classOf, const std::vector<std::size_t>& placedSceneIndex,
                             const std::vector<chase::assets::Placement>& placedObjects, const std::vector<chase::assets::CharacterObject>& characters,
                             const std::vector<std::optional<chase::render::CharacterInstance>>& placedCharacters,
                             const chase::render::CharacterInstance* player, const std::string& playerClass) const {
    for (std::size_t i = 0; i < roomInstances; ++i) scene.setInstanceShadow(i, false, true);
    for (std::size_t i = 0; i < placedObjects.size() && i < placedSceneIndex.size(); ++i)
        if (const chase::assets::Level* cls = classOf(placedObjects[i].className))
            markObject(scene, placedSceneIndex[i], level.objects[placedObjects[i].objectIndex], *cls);
    for (std::size_t i = 0; i < characters.size() && i < placedCharacters.size(); ++i)
        if (placedCharacters[i])
            if (const chase::assets::Level* cls = classOf(characters[i].placement.className))
                markCharacter(scene, *placedCharacters[i], level.objects[characters[i].placement.objectIndex], *cls);
    if (player)
        if (const chase::assets::Level* cls = classOf(playerClass)) markCharacter(scene, *player, chase::assets::LevelObject{}, *cls);
}

void LevelShadows::markObject(chase::render::Scene& scene, std::size_t instance, const chase::assets::LevelObject& object,
                              const chase::assets::Level& cls) const {
    const runtime::ShadowFlags flags = runtime::shadowFlags(object, cls);
    scene.setInstanceShadow(instance, flags.caster, flags.receiver);
}

void LevelShadows::markCharacter(chase::render::Scene& scene, const chase::render::CharacterInstance& character,
                                 const chase::assets::LevelObject& object, const chase::assets::Level& cls) const {
    markObject(scene, character.bodyInstance, object, cls);
    markObject(scene, character.headInstance, object, cls);
    if (character.hatInstance) markObject(scene, *character.hatInstance, object, cls);
}

void LevelShadows::render(chase::render::Scene& scene, const Mat4& viewToWorld, std::optional<float> playerY, bool shown) {
    drawn_ = false;
    if (!light_ || !shown) return;
    runtime::ShadowMatrix camera = viewToWorld;
    for (const std::size_t i : {0u, 1u, 2u, 8u, 9u, 10u}) camera[i] = -camera[i];
    if (playerY)
        if (const auto focus = runtime::shadowFocus(camera, runtime::kShadowRayUp, runtime::kShadowRayForward, *playerY, radius_)) focus_ = focus;
    if (!focus_) return;
    const runtime::ShadowCamera shadowCamera = runtime::shadowCamera(lightMatrix_, *focus_, camera, radius_);
    pass_->render(scene, runtime::shadowViewProjection(shadowCamera));
    drawn_ = true;
}

void LevelShadows::apply(const chase::render::Scene& scene, const Mat4& viewProjection) const {
    if (!drawn_) return;
    pass_->apply(scene, viewProjection, runtime::shadowConstant(light_->colour, light_->opacity));
}

}
