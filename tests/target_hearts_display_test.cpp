// The hearts display of a chased target (src/runtime/target_hearts_display.hpp). The rules are transcribed from 0x230f04 (llvm-objdump of game.arm.elf,
// 20261112-traffic-D, 2026-11-12, docs/re/traffic-driver-states.md); the targets are made up.
#include "runtime/target_hearts_display.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

TargetHeartsSource car(std::uint8_t hearts, std::uint8_t most) {
    TargetHeartsSource source;
    source.kind = TargetHeartsKind::Vehicle;
    source.entity = "stolen_car";
    source.hearts = hearts;
    source.most = most;
    source.portraitName = "Vehicle_Sports";
    return source;
}

void run() {
    TargetHeartsDisplay display;
    setTargetHearts(display, car(3, 3));
    require(display.shown && display.most == 3 && display.hearts == 3 && display.previous == 3, "a new car: the most, the hearts and the previous hearts are its most");
    require(display.changed == 0.0f && display.unchanged == 1.0f, "no change of the hearts: +0x84 = 0.0, +0x88 = 1.0");
    require(display.portrait == "Sprites/portraits/Vehicle_Sports.png" && display.target == "stolen_car", "the portrait path 0x2310d4 and the target 0x375734");

    setTargetHearts(display, car(2, 3));
    require(display.shown && display.previous == 3 && display.hearts == 2 && display.changed == 1.0f && display.unchanged == 0.0f,
            "one heart less: previous 3, hearts 2, +0x84 = 1.0, +0x88 = 0.0");
    setTargetHearts(display, car(2, 3));
    require(display.previous == 2 && display.hearts == 2 && display.changed == 0.0f, "the same hearts again count as unchanged");

    setTargetHearts(display, car(5, 5));
    require(display.previous == 5 && display.hearts == 5 && display.most == 5 && display.changed == 0.0f, "another most resets the previous hearts to it first");

    setTargetHearts(display, car(255, 255));
    require(!display.shown && display.changed == 1.0f, "a most of 255 is not shown and always counts as changed");

    TargetHeartsSource character;
    character.kind = TargetHeartsKind::Character;
    character.entity = "robber";
    character.hearts = 1;
    character.most = 4;
    character.portraitName = "Robber";
    setTargetHearts(display, character);
    require(display.shown && display.most == 4 && display.hearts == 1 && display.portrait == "Sprites/portraits/Robber.png", "a character shows its arrest health");

    TargetHeartsSource prop;
    prop.kind = TargetHeartsKind::Other;
    prop.entity = "crate";
    prop.portraitName = "Crate";
    setTargetHearts(display, prop);
    require(!display.shown && display.most == 0 && display.hearts == 0 && display.portrait == "Sprites/portraits/Robber.png" && display.target == "crate",
            "another kind: off, no portrait load, but still the target");

    setTargetHearts(display, TargetHeartsSource{});
    require(!display.shown && display.target.empty(), "0x230f04(0) switches the display off and clears the target");
}

}

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "target-hearts-display-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "target-hearts-display-test: ok\n";
    return 0;
}
