// Update order (src/runtime/update_order.cpp). Call sites: llvm-objdump of artifacts/game/game.arm.elf, step 0x2e5158:
// runner bl 0x169254 at 0x2e54b4, entity pass bl 0x1a3f80 at 0x2e5530 (sorted by entity+0x19 then type; the character
// create 0x182e28 stores 254 at 0x182ee8), use spot update bl 0x172254 at 0x2e55b0.
#include "runtime/update_order.hpp"

#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    const auto& phases = updatePhases();
    require(phases[0].phase == UpdatePhase::Runner && phases[0].callSite == 0x2e54b4, "runner first");
    require(phases[1].phase == UpdatePhase::Objects && phases[1].callSite == 0x2e5530, "objects");
    require(phases[2].phase == UpdatePhase::Characters && phases[2].callSite == 0x2e5530, "characters");
    require(phases[4].phase == UpdatePhase::UseSpots && phases[4].callSite == 0x2e55b0, "use spots");
    require(phases[3].standIn && !phases[2].standIn, "stand-in flag");

    std::string log;
    UpdateSequence sequence;
    sequence.at(UpdatePhase::UseSpots, [&] { log += 'U'; });
    sequence.at(UpdatePhase::Characters, [&] { log += 'C'; });
    sequence.at(UpdatePhase::AfterCharactersStandIn, [&] { log += 'A'; });
    sequence.at(UpdatePhase::Objects, [&] { log += 'O'; });
    sequence.at(UpdatePhase::Objects, [&] { log += 'o'; });
    sequence.at(UpdatePhase::Runner, [&] { log += 'R'; });
    sequence.run();
    require(log == "ROoCAU", "order " + log);
    log.clear();
    sequence.run();
    require(log.empty(), "steps run once");
    return 0;
}
