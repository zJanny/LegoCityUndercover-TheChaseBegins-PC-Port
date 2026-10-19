#pragma once
// The pole of a zipline anchor: the anchor's create 0x1a999c loads the model of its meshname (`Ability_Parkour_Zip_Line`) unless the
// parameter "NoMesh" is set, and the model's node 0 carries a matrix (model+0x20) that lifts the pole's mesh, stored at its authoring
// height, back onto the anchor. Rules and measurements: docs/re/player-traversal.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/zipline.hpp"

#include <optional>

namespace chase::runtime {

// The matrix of the model's first node when it has a drawn mesh and a valid matrix (w of the translation row non-zero).
std::optional<assets::Matrix> modelRootNodeMatrix(const assets::Model& model);

// TRAV-S33: pole = node matrix * anchor node, row vectors with the node matrix applied first (as doors and the detective cards do; the
// generic model draw of the original is not read). The anchor node is the linked node (rows right, up, forward of 0x19b074, translation
// the node position) when the anchor has a target, else the placement.
assets::Matrix ziplinePoleMatrix(const assets::Matrix& modelNode, const ZiplineAnchor& anchor, const assets::Matrix& placement);

}
