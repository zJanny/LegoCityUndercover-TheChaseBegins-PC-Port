#include "runtime/object_visibility.hpp"

namespace chase::runtime {

std::vector<bool> drawnPlacements(const std::vector<assets::Placement>& placements, const assets::PlacedObjectState& state) {
    std::vector<bool> drawn;
    drawn.reserve(placements.size());
    for (const assets::Placement& placement : placements) drawn.push_back(!state.isHidden(placement.instance));
    return drawn;
}

std::vector<bool> drawnCharacters(const std::vector<assets::CharacterObject>& characters, const assets::PlacedObjectState& state) {
    std::vector<bool> drawn;
    drawn.reserve(characters.size());
    for (const assets::CharacterObject& character : characters) drawn.push_back(!state.isHidden(character.placement.instance));
    return drawn;
}

bool nestedObjectDrawn(bool ownerDrawn) { return ownerDrawn; }

}
