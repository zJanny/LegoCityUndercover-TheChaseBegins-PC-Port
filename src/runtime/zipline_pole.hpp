#pragma once
// The pole of a zipline anchor: the anchor's create 0x1a999c loads the model of its meshname (`Ability_Parkour_Zip_Line`) unless the
// parameter "NoMesh" is set, and the model draw puts the pole's mesh on the anchor node with the model's node matrix like every model
// (assets::nodeDrawMatrix). Rules and measurements: docs/re/player-traversal.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/zipline.hpp"

namespace chase::runtime {

// The anchor node the pole is drawn on: the linked node (rows right, up, forward of 0x19b074, translation the node position) when the
// anchor has a target, else the placement.
assets::Matrix ziplineAnchorNode(const ZiplineAnchor& anchor, const assets::Matrix& placement);

}
