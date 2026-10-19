#include "runtime/zipline_pole.hpp"

namespace chase::runtime {

assets::Matrix ziplineAnchorNode(const ZiplineAnchor& anchor, const assets::Matrix& placement) {
    if (anchor.target < 0) return placement;
    return {anchor.right[0],    anchor.right[1],    anchor.right[2],    0.0f,
            anchor.up[0],       anchor.up[1],       anchor.up[2],       0.0f,
            anchor.forward[0],  anchor.forward[1],  anchor.forward[2],  0.0f,
            anchor.position[0], anchor.position[1], anchor.position[2], 1.0f};
}

}
