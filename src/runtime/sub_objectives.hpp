#pragma once
// Per-update work of the sub-objective manager (*0x51a3a8, docs/re/objective-hud.md): the arrow update 0x2ed8d8, the record
// update 0x2be934 (target removal, centroid, the map slot of a group and its fade) and the values the arrow draw 0x29de50 uses.
// No SDL dependency.
#include "runtime/objectives.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace chase::runtime {

// Floats written by the static initialiser 0x34bb18 into 0x375d2c..0x375d38.
constexpr float kObjectiveArrowLift = 1.2f;        // 0x375d2c: above the top of the entity box
constexpr float kObjectiveArrowBobDivisor = 0.2f;  // 0x375d30: sin(clock / 0.2)
constexpr float kObjectiveArrowBobAmount = 0.2f;   // 0x375d34
constexpr float kSubObjectiveSlotFadeSeconds = 1.0f;  // 0x375d38: the fade of a group's map slot
constexpr float kObjectiveArrowUseMarkerDistance = 4.0f;  // 0x2ed8d8: an arrow over a use marker hides within 4.0 of the player
// 0x3589fc: map icon type of a group's slot by record type (go to, enemy, carry).
constexpr std::array<std::uint8_t, 3> kSubObjectiveSlotIcons = {14, 13, 15};

struct SubObjectiveWorld {
    // The node translation of an entity (0x250810 + 0x30).
    std::function<bool(const std::string&, float[3])> position;
    // Entity flag +0x10 bit 0 (disabled) and +0xc bit 4 (destroyed).
    std::function<bool(const std::string&)> hidden;
    std::function<bool(const std::string&)> destroyed;
    // 0x29de40 -> 0x33dccc: the position lies inside the map's inner ellipse.
    std::function<bool(const float[3])> onMap;
    // 0x1f8638 / 0x1f8564: the entity has a use marker in the list 0x5289ec; its world point and the marker's alpha 0x235e2c.
    std::function<bool(const std::string&, float[3], float&)> useMarker;
    std::array<float, 3> player{};
    // 0x230ef4 (*(0x529c18 + 0x24), the carry-it waypoint of WaypointSet) compared with the key of a record: that key is not counted
    // into the centroid of a record with more than one target. Unset: the key is always counted.
    std::function<bool(const std::string&)> keyIsManagerEntity;
};

// The alpha byte of the arrow draw 0x29de50 for an entry: timer t > 0 gives int(255 * 2t) fading out and int(255 * (1 - 2t))
// fading in; t <= 0 gives 0 fading out, else `baseAlpha` (255, or the use marker's alpha byte).
int objectiveArrowAlphaByte(const ObjectiveArrow& arrow, int baseAlpha);

// 0x2ed694 / 0x29de50: the centre of the arrow's 1 x 1 camera-facing quad. Without a use marker it is the node translation
// raised by (box centre y + box half y + 1.2), with a use marker the marker point raised by 1.2; both then rise by
// sin(clock / 0.2) * 0.2.
std::array<float, 3> objectiveArrowAnchor(const float node[3], float boxCentreY, float boxHalfY, float clockSeconds);
std::array<float, 3> objectiveArrowUseMarkerAnchor(const float marker[3], float clockSeconds);

// 0x1589fc with the scale (0.5, 0.5): the quad's corners top left, top right, bottom right, bottom left (uv (0,0), (1,0), (1,1),
// (0,1)) at the anchor -+ 0.5 along the camera's right and up axes (the rotation of the camera node *0x37688c).
std::array<std::array<float, 3>, 4> objectiveArrowQuad(const std::array<float, 3>& anchor, const float right[3], const float up[3]);
constexpr float kObjectiveArrowHalfSize = 0.5f;

}
