// The allegiance lists 0x58deb0 / 0x58dec8 and the FallArrestBoundSystem update on them, on made-up characters and bounds. The rules were read by
// 20261103-large-D (2026-11-03) with llvm-objdump of game.arm.elf: 0x206d18 (enter into the list), 0x21decc (leave), 0x2b6b5c (SwapAllegiance's move),
// 0x307980 (the update); docs/re/scripts-missions-1-4.md. Every number below is made up and checked by hand against those rules. No game data needed.
#include "runtime/fall_arrest_engaged.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void membership() {
    runtime::EngagedTable table;
    for (int i = 0; i < 6; ++i) {
        const runtime::EngageResult result = table.engage("goon" + std::to_string(i), true);
        require(result.joined && result.counterCleared && !result.listFull, "the first free slot of the bad guys' list (0x206d18)");
    }
    require(table.list(true)[0] == "goon0" && table.list(true)[5] == "goon5" && table.count(true) == 6, "slots in order of joining");
    runtime::EngageResult result = table.engage("goon6", true);
    require(!result.joined && result.listFull && result.counterCleared && !table.engaged("goon6"), "a seventh bad guy gets no slot");
    result = table.engage("goon0", true);
    require(!result.joined && !result.counterCleared && result.alreadyEngaged, "ctrl+0xd8 bit 5 set: 0x206d18 returns at the first test");
    require(!table.playerSlot().size(), "the player slot 0x58dee0 starts empty");
    table.setPlayerSlot("player1");
    require(table.playerSlot() == "player1" && table.count(false) == 0 && table.count(true) == 6, "the player slot is not one of the six (0x1f8fb0 writes +0x18 of 0x58dec8)");
    result = table.engage("friend", false);
    require(result.joined && table.count(false) == 1 && table.list(false)[0] == "friend", "the other side has its own list");
    require(table.release("goon2", true, false, false) && table.list(true)[2].empty() && !table.engaged("goon2"), "0x21decc empties the slot");
    require(table.engage("goon7", true).joined && table.list(true)[2] == "goon7", "the first free slot is reused");
    require(!table.release("boss", true, true, false), "a character that was never engaged is untouched");
    table.engage("boss", false);
    require(!table.release("boss", false, true, false) && table.engaged("boss"), "a boss with an AI request other than 1 stays (0x21decc)");
    require(table.release("boss", false, true, true) && !table.engaged("boss"), "a boss with AI request 1 leaves");
}

assets::LevelBound box(const std::string& name, float cx, float half) {
    assets::LevelBound bound;
    bound.name = name;
    bound.centre[0] = cx;
    bound.half[0] = bound.half[1] = bound.half[2] = half;
    bound.shape = 3;
    return bound;
}

void fallArrest() {
    assets::Level level;
    level.bounds = {box("FallArrestBound01", 0, 1), box("elsewhere", 0, 50)};
    const std::vector<std::size_t> bounds = runtime::fallArrestBounds(level);
    const std::vector<std::uint8_t> enabled = {1, 1};
    runtime::EngagedTable table;
    table.engage("thrown", true);
    table.engage("walker", true);
    table.engage("friend", false);
    table.setOnGround("thrown", false);
    table.setOnGround("friend", false);
    const runtime::EngagedTable::Describe describe = [](const std::string&) {
        runtime::FallArrestCharacter character;
        character.position = {0.5f, 0, 0};
        return std::optional<runtime::FallArrestCharacter>(character);
    };
    std::vector<runtime::FallArrestHit> hits = table.fallArrestUpdate(level, bounds, enabled, describe);
    require(hits.size() == 1 && hits[0].character == "thrown" && hits[0].bound == 0, "only the bad guys' list in the air inside a FallArrest bound (0x307980)");
    require(table.autoArrested("thrown") && !table.autoArrested("walker") && !table.autoArrested("friend"), "the mark 2 (sub +0x131 bits 6..7)");
    hits = table.fallArrestUpdate(level, bounds, enabled, describe);
    require(hits.empty(), "a marked character is not marked again");
    table.release("thrown", true, false, false);
    require(table.autoArrested("thrown"), "the mark stays with the character after it leaves the list");
    table.setOnGround("walker", false);
    const runtime::EngagedTable::Describe gone = [](const std::string&) { return std::optional<runtime::FallArrestCharacter>(); };
    require(table.fallArrestUpdate(level, bounds, enabled, gone).empty(), "a character the glue cannot describe is skipped");
}

void sound() {
    std::array<std::uint16_t, assets::kCharacterSoundSlots> row{};
    row[19] = 7;
    row[20] = 321;
    row[21] = 9;
    require(runtime::fallArrestSoundId(row) == 321, "slot 20 of the sound row (u16 +0x28)");
}

}

int main() {
    try {
        membership();
        fallArrest();
        sound();
    } catch (const std::exception& error) {
        std::cerr << "fall-arrest-engaged: " << error.what() << '\n';
        return 1;
    }
    std::cout << "fall-arrest-engaged: ok\n";
    return 0;
}
