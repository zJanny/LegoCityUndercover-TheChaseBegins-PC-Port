#include "runtime/character_respawn.hpp"

namespace chase::runtime {

CharacterAfterDeath characterAfterDeath(const CharacterAfterDeathInput& input) {
    if (!input.respawnFlags && !input.friendly) return CharacterAfterDeath::LeaveEngaged;
    return input.millAbout ? CharacterAfterDeath::MillAbout : CharacterAfterDeath::BackToSpawn;
}

std::uint32_t characterComeBackColour(std::uint8_t counter) {
    if (counter == kComeBackCounterEnd) return 0xffffffffu;
    std::uint32_t n = counter & 15u;
    if (n >= 8) n = 15 - n;
    const std::uint32_t greenBlue = ((n << 5) | 31u) & 0xffu;
    return 0xff000000u | (greenBlue << 16) | (greenBlue << 8) | 0xffu;
}

CharacterComeBackStep characterComeBackStep(CharacterComeBack& state, bool respawnEvent, std::uint8_t kind) {
    CharacterComeBackStep step;
    if (state.counter < kComeBackCounterEnd) {
        ++state.counter;
        if (state.counter > kComeBackShowAfter && !state.dead) {
            step.shown = true;
            if (kind != kNoComeBackColourKind) step.colour = characterComeBackColour(state.counter);
        }
    }
    if (state.dead && state.counter > kComeBackDeadWait) {
        step.raisesEvent = respawnEvent;
        step.leavesDead = true;
        state.dead = false;
    }
    return step;
}

}
