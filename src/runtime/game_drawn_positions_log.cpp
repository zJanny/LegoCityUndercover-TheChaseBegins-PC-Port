#include "game_level.hpp"

#include <iostream>

namespace game_level {

void LevelSession::logDrawnPositions() {
    for (const std::size_t index : pushableDrawnPending) {
        if (!pushableScene[index]) continue;
        const chase::render::Mat4 drawn = scene.instanceTransform(*pushableScene[index]);
        std::cout << "pushable " << pushables.objects()[index].instance << ": drawn at " << drawn[12] << ' ' << drawn[13] << ' ' << drawn[14] << " at update " << updates << '\n';
    }
    pushableDrawnPending.clear();
    for (const auto& [instance, link] : propAttachments.links()) {
        const std::optional<chase::assets::Matrix> child = placedNodeWorld(instance), parent = placedNodeWorld(link.parent);
        if (!child || !parent) continue;
        if (const auto local = attachedDrawnLog.observe(instance, *child, *parent))
            std::cout << "prop " << instance << ": drawn at " << (*local)[0] << ' ' << (*local)[1] << ' ' << (*local)[2] << " in " << link.parent << " after it moved at update " << updates << '\n';
    }
}

}
