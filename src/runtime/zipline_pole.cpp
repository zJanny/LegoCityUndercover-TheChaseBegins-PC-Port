#include "runtime/zipline_pole.hpp"

namespace chase::runtime {

std::optional<assets::Matrix> modelRootNodeMatrix(const assets::Model& model) {
    if (model.nodeMatrices.empty() || model.nodeMesh.empty() || model.nodeMesh[0] < 0) return std::nullopt;
    if (model.nodeMatrices[0][15] == 0.0f) return std::nullopt;
    return model.nodeMatrices[0];
}

assets::Matrix ziplinePoleMatrix(const assets::Matrix& modelNode, const ZiplineAnchor& anchor, const assets::Matrix& placement) {
    if (anchor.target < 0) return assets::multiply(modelNode, placement);
    const assets::Matrix linked = {anchor.right[0],    anchor.right[1],    anchor.right[2],    0.0f,
                                   anchor.up[0],       anchor.up[1],       anchor.up[2],       0.0f,
                                   anchor.forward[0],  anchor.forward[1],  anchor.forward[2],  0.0f,
                                   anchor.position[0], anchor.position[1], anchor.position[2], 1.0f};
    return assets::multiply(modelNode, linked);
}

}
