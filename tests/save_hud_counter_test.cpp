// Tests of src/runtime/save_hud_counter.*: the HUD stud counter after a level load.
// Read with llvm-objdump of artifacts/game/game.arm.elf: the level load 0x2e5fac..0x2e5fdc (studs from slot +0x5148 to 0x593418, type =
// (slot[+0x520b] << 27) >> 29 into 0x22ddf4(0x519760, type, studs)), the Studs bar creation 0x15b350..0x15b35c and the HUD page 0x2cf064..0x2cf094
// (type and value reset to 0 when the counter is 0, then 0x22ddf4 again), 0x22ddf4 (type 4 keeps, the others store at +0x40 and into the slot bits).
#include "runtime/save_hud_counter.hpp"
#include "save/save_writer.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void loadedStuds() {
    for (unsigned type = 0; type < 4; ++type) {
        const runtime::HudStudCounter counter = runtime::hudStudCounterAtLoad(type, 1130);
        require(counter.type == type && counter.total == 1130, "a loaded counter keeps the type of the slot and shows the studs");
    }
}

void emptyCounter() {
    const runtime::HudStudCounter counter = runtime::hudStudCounterAtLoad(3, 0);
    require(counter.type == 0 && counter.total == 0, "a counter of 0 resets the type");
    std::vector<std::uint8_t> slot(save::kSlotSize, 0);
    save::setSlotStudType(slot.data(), 3);
    save::setSlotStudType(slot.data(), counter.type);
    require(save::slotStudType({slot.data(), slot.size()}) == 0, "0x22ddf4 clears the type bits of the slot");
}

void writeBack() {
    std::vector<std::uint8_t> slot(save::kSlotSize, 0);
    slot[save::kSlotStudFlagsOffset] = 0x20 | (2 << 2);
    const runtime::HudStudCounter counter = runtime::hudStudCounterAtLoad(save::slotStudType({slot.data(), slot.size()}), 52);
    save::setSlotStudType(slot.data(), counter.type);
    require(slot[save::kSlotStudFlagsOffset] == (0x20 | (2 << 2)) && counter.type == 2, "a loaded type is written back unchanged, other bits stay");
}

}

int main() {
    try {
        loadedStuds();
        emptyCounter();
        writeBack();
    } catch (const std::exception& error) {
        std::cerr << "save-hud-counter test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "save-hud-counter test passed\n";
    return 0;
}
