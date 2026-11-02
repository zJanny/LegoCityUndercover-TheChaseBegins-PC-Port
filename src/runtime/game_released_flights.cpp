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

}
