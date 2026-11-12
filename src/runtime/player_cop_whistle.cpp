#include "runtime/player_cop_whistle.hpp"

namespace chase::runtime {

PlayerCopWhistleStep PlayerCopWhistle::update(const PlayerCopWhistleInput& input, TrafficWhistleStop& stop, GameRandom& random) {
    PlayerCopWhistleStep step;
    if (active_) {
        if (updateCopWhistle(elapsed_ >= clipUpdates_)) {
            active_ = false;
            step.finished = true;
            return step;
        }
        ++elapsed_;
        return step;
    }
    CopWhistleGate gate;
    gate.extraActive = (input.extras >> kCopWhistleExtra & 1u) != 0;
    gate.state = input.state;
    gate.downPressed = input.downPressed;
    gate.word13c = input.carrying ? 1u : 0u;
    gate.characterClass = kPlayerCharacterClassStandIn;
    if (!copWhistleRequested(gate)) return step;
    enterCopWhistle(stop, input.clock, random);
    active_ = true;
    elapsed_ = 1;
    step.entered = true;
    return step;
}

}
