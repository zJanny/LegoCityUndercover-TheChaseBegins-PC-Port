// Rules of the cutscene setup 0x2d1524 and teardown 0x2d135c (docs/re/cutscene-actors.md, llvm-objdump of artifacts/game/game.arm.elf, 2026-10-19):
// an actor entity is enabled and its node shown while the cutscene holds it, and its node matrix goes back to the one saved at the setup.
#include "runtime/cutscene_hold.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::runtime::CutsceneHold;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        CutsceneHold hold;
        require(!hold.holds(4), "nothing is held at the start");
        require(!hold.end(4).held, "ending an entity that was never held does nothing");

        CutsceneHold::Node start{};
        for (std::size_t i = 0; i < 16; ++i) start[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        start[12] = 730.328f, start[13] = 31.599f, start[14] = 645.549f;
        hold.begin(4, start);
        hold.begin(7);
        require(hold.holds(4) && hold.holds(7) && !hold.holds(5), "both actors are held");

        CutsceneHold::Node changed = start;
        changed[12] = 744.5f;
        hold.begin(4, changed);
        const CutsceneHold::Released prop = hold.end(4);
        require(prop.held && prop.node && *prop.node == start, "the matrix saved at the setup comes back, not a later one");
        require(!hold.holds(4), "the entity is released");
        const CutsceneHold::Released character = hold.end(7);
        require(character.held && !character.node, "a character without a saved matrix has nothing to restore");
        require(!hold.end(7).held, "a second end finds nothing");

        require(chase::runtime::drawnWhileHeld(false, true), "a disabled entity is drawn while the cutscene holds it");
        require(!chase::runtime::drawnWhileHeld(false, false), "a disabled entity that is not held stays hidden");
        require(chase::runtime::drawnWhileHeld(true, false) && chase::runtime::drawnWhileHeld(true, true), "an enabled entity is always drawn");
    } catch (const std::exception& error) {
        std::cerr << "cutscene-hold-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
