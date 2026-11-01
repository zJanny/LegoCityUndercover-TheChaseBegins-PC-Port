#include "game_level.hpp"

#include "runtime/collision.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::landReleasedFlights(float dt) {
    runtime::AiCore& ai = levelScripts.ai();
    for (const auto& [instance, character] : levelCharacters->characters()) {
        const runtime::AiCharacter* flying = ai.character(instance);
        if (!flying || !flying->superJump || !flying->superJump->released || !flying->position) continue;
        runtime::GroundProbe probe;
        probe.x = (*flying->position)[0];
        probe.y = (*flying->position)[1];
        probe.z = (*flying->position)[2];
        probe.dt = dt;
        probe.distance = runtime::kReleasedFlightReach;
        const std::optional<float> ground = runtime::groundHeight(collision, probe);
        if (!ground) continue;
        ai.land(instance, *ground);
        if (std::optional<chase::assets::Matrix> world = levelCharacters->world(instance)) {
            (*world)[13] = *ground;
            levelCharacters->place(instance, *world);
        }
        std::cout << "character " << instance << ": released flight landed at " << (*flying->position)[0] << ' ' << *ground << ' ' << (*flying->position)[2] << " at update " << updates << '\n';
    }
}

void LevelSession::logDrawnFixes() {
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
