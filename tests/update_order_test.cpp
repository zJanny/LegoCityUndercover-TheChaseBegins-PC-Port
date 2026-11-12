// Update order (src/runtime/update_order.cpp). Call sites: llvm-objdump of artifacts/game/game.arm.elf, step 0x2e5158:
// system list bl 0x169374 at 0x2e5488, runner bl 0x169254 at 0x2e54b4, entity pass bl 0x1a3f80 at 0x2e5530 (sorted by entity+0x19 then type; the character
// create 0x182e28 stores 254 at 0x182ee8), use spot update bl 0x172254 at 0x2e55b0.
// Entity pass order (20261112-traffic-F, 2026-11-12): the comparator 0x1cf7e4 orders by priority byte +0x19, then class type +0x12,
// both ascending; the object loader stores 128 at 0x1a4214 for every object; the pedestrian registration 0x1786dc is `mov r0, #197`.
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
    require(phases[0].phase == UpdatePhase::Systems && phases[0].callSite == 0x2e5488, "systems first");
    require(phases[1].phase == UpdatePhase::Runner && phases[1].callSite == 0x2e54b4, "runner");
    require(phases[2].phase == UpdatePhase::Objects && phases[2].callSite == 0x2e5530, "objects");
    require(phases[3].phase == UpdatePhase::Characters && phases[3].callSite == 0x2e5530, "characters");
    require(phases[5].phase == UpdatePhase::UseSpots && phases[5].callSite == 0x2e55b0, "use spots");
    require(phases[4].standIn && !phases[3].standIn, "stand-in flag");

    std::string log;
    UpdateSequence sequence;
    sequence.at(UpdatePhase::UseSpots, [&] { log += 'U'; });
    sequence.at(UpdatePhase::Characters, [&] { log += 'C'; });
    sequence.at(UpdatePhase::AfterCharactersStandIn, [&] { log += 'A'; });
    sequence.at(UpdatePhase::Objects, [&] { log += 'O'; });
    sequence.at(UpdatePhase::Objects, [&] { log += 'o'; });
    sequence.at(UpdatePhase::Runner, [&] { log += 'R'; });
    sequence.at(UpdatePhase::Systems, [&] { log += 'S'; });
    sequence.run();
    require(log == "SROoCAU", "order " + log);
    log.clear();
    sequence.run();
    require(log.empty(), "steps run once");

    sequence.atObjectKind(kPedestrianObjectKind, [&] { log += 'P'; });
    sequence.at(UpdatePhase::Objects, [&] { log += 'a'; });
    sequence.atObjectKind(200, [&] { log += 'Z'; });
    sequence.at(UpdatePhase::Objects, [&] { log += 'b'; });
    sequence.atObjectKind(kPedestrianObjectKind, [&] { log += 'Q'; });
    sequence.at(UpdatePhase::Characters, [&] { log += 'C'; });
    sequence.run();
    require(log == "abPQZC", "objects sorted by kind, equal kinds in call order: " + log);
    require(kPedestrianObjectKind == 197, "chars/pedestrian is kind 197");
    return 0;
}
