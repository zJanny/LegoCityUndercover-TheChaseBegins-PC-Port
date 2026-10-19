#pragma once
// Draw visibility of the placed characters from the disabled state (docs/re/objects.md, 0x2428c0).
#include "assets/assets.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "runtime/object_visibility.hpp"
#include <optional>
#include <vector>

namespace game_object_visibility {

class CharacterVisibility {
public:
    void apply(chase::render::Scene& scene, const std::vector<chase::assets::CharacterObject>& characters,
               const std::vector<std::optional<chase::render::CharacterInstance>>& placed, const chase::assets::PlacedObjectState& state);

private:
    std::vector<bool> wasHidden_;
};

}
