#include "game_object_visibility.hpp"

namespace game_object_visibility {
namespace render = chase::render;

namespace {
void setVisible(render::Scene& scene, const render::CharacterInstance& instance, bool visible) {
    scene.setInstanceVisible(instance.bodyInstance, visible);
    scene.setInstanceVisible(instance.headInstance, visible);
    if (instance.hatInstance) scene.setInstanceVisible(*instance.hatInstance, visible);
}
}

void CharacterVisibility::apply(render::Scene& scene, const std::vector<chase::assets::CharacterObject>& characters,
                                const std::vector<std::optional<render::CharacterInstance>>& placed, const chase::assets::PlacedObjectState& state) {
    const std::vector<bool> drawn = chase::runtime::drawnCharacters(characters, state);
    wasHidden_.resize(drawn.size(), false);
    for (std::size_t i = 0; i < drawn.size() && i < placed.size(); ++i) {
        if (!placed[i]) continue;
        if (!drawn[i]) {
            setVisible(scene, *placed[i], false);
        } else if (wasHidden_[i]) {
            setVisible(scene, *placed[i], true);
        }
        wasHidden_[i] = !drawn[i];
    }
}

}
